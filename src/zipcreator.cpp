/*
 * Copyright (C) 2016-2019 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3, or (at your option) any later version,
 * of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "zipcreator.h"
#include "bytequeue.hpp"
#include "compress.h"
#include "file.h"
#include "fileutils.h"
#include "mmapper.h"
#include "utils.h"
#include "zipdefs.h"

#include <portable_endian.h>
#if defined(_WIN32)
#include <windows.h>
#include <winsock2.h>
#else
#include <pthread.h>
#include <sys/stat.h>
#endif

#include <cassert>
#include <future>
#include <stdexcept>
#include <thread>
#include <algorithm>

namespace {

struct CompressionTask {
    fileinfo fi;
    ByteQueue queue;
    std::future<compressresult> result;

    explicit CompressionTask(const fileinfo fi, const int64_t queue_size)
        : fi(fi), queue(queue_size) {}
};

typedef std::vector<std::unique_ptr<CompressionTask>> task_array;

void write_localheader(File &ofile, const localheader &lh) {
    ofile.write32le(LOCAL_SIG);
    ofile.write16le(lh.needed_version);
    ofile.write16le(lh.gp_bitflag);
    ofile.write16le(lh.compression);
    ofile.write16le(lh.last_mod_time);
    ofile.write16le(lh.last_mod_date);
    ofile.write32le(lh.crc32);
    ofile.write32le(lh.compressed_size);
    ofile.write32le(lh.uncompressed_size);
    ofile.write16le(lh.fname.size());
    ofile.write16le(lh.extra.size());
    ofile.write(lh.fname);
    ofile.write(lh.extra);
}

void write_file(ByteQueue &q, File &ofile) {
    do {
        auto buf = q.pop();
        ofile.write(buf.data(), buf.size());
    } while(q.state() != QueueState::SHUTDOWN);
    auto final_buf = q.pop();
    ofile.write(final_buf.data(), final_buf.size());
}

void write_central_header(File &ofile, const centralheader &ch) {
    ofile.write32le(CENTRAL_SIG);
    ofile.write16le(ch.version_made_by);
    ofile.write16le(ch.version_needed);
    ofile.write16le(ch.bit_flag);
    ofile.write16le(ch.compression_method);
    ofile.write16le(ch.last_mod_time);
    ofile.write16le(ch.last_mod_date);
    ofile.write32le(ch.crc32);
    ofile.write32le(ch.compressed_size);
    ofile.write32le(ch.uncompressed_size);
    ofile.write16le(ch.fname.size());
    ofile.write16le(ch.extra_field.size());
    ofile.write16le(ch.comment.size());
    ofile.write16le(ch.disk_number_start);
    ofile.write16le(ch.internal_file_attributes);
    ofile.write32le(ch.external_file_attributes);
    ofile.write32le(ch.local_header_rel_offset);
    ofile.write(ch.fname);
    ofile.write(ch.extra_field);
    ofile.write(ch.comment);
}

void write_end_record(File &ofile, const endrecord &ed) {
    ofile.write32le(CENTRAL_END_SIG);
    ofile.write16le(ed.disk_number);
    ofile.write16le(ed.central_dir_disk_number);
    ofile.write16le(ed.this_disk_num_entries);
    ofile.write16le(ed.total_entries);
    ofile.write32le(ed.dir_size);
    ofile.write32le(ed.dir_offset_start_disk);
    ofile.write16le(ed.comment.size());
    ofile.write(ed.comment);
}

void write_z64_eod_record(File &ofile, const zip64endrecord &r) {
    ofile.write32le(ZIP64_CENTRAL_END_SIG);
    ofile.write64le(r.recordsize);
    ofile.write16le(r.version_made_by);
    ofile.write16le(r.version_needed);
    ofile.write32le(r.disk_number);
    ofile.write32le(r.dir_start_disk_number);
    ofile.write64le(r.this_disk_num_entries);
    ofile.write64le(r.total_entries);
    ofile.write64le(r.dir_size);
    ofile.write64le(r.dir_offset);
    ofile.write(r.extensible);
}

void write_z64_eod_locator(File &ofile, const zip64locator &l) {
    ofile.write32le(ZIP64_CENTRAL_LOCATOR_SIG);
    ofile.write32le(l.central_dir_disk_number);
    ofile.write64le(l.central_dir_offset);
    ofile.write32le(l.num_disks);
}

template<typename C> void append_data(std::string &s, const C &c) {
    s.append(reinterpret_cast<const char *>(&c), sizeof(C));
}

std::string pack_zip64(uint64_t uncompressed_size, uint64_t compressed_size, uint64_t offset) {
    const uint16_t tag = 0x01;
    const uint16_t size = 8 + 8 + 8 + 4;
    std::string result;
    append_data(result, htole16(tag));
    append_data(result, htole16(size));
    append_data(result, htole64(uncompressed_size));
    append_data(result, htole64(compressed_size));
    append_data(result, htole64(offset));
    append_data(result, htole32(0));
    assert(result.size() == size + 2 * 2);
    return result;
}

std::string pack_unix_extra(unixextra ue) {
    const uint16_t tag = 0x0d;
    const uint16_t size = 4 + 4 + 2 + 2 + ue.data.size();
    std::string result;
    append_data(result, htole16(tag));
    append_data(result, htole16(size));
    append_data(result, htole32(ue.atime));
    append_data(result, htole32(ue.mtime));
    append_data(result, htole16(ue.uid));
    append_data(result, htole16(ue.gid));
    result += ue.data;
    return result;
}

centralheader write_entry(File &ofile, CompressionTask &t) {
    localheader lh;
    centralheader ch;
    const fileinfo &i = t.fi;
    uint64_t local_header_offset = ofile.tell();
    uint64_t uncompressed_size = i.fsize;
    uint64_t compressed_size = 0xFFFFFFFF;
    lh.fname = i.fname;
    const auto compression_result = t.result.get();
    if(!compression_result.additional_unix_extra_data.empty()) {
        t.fi.ue.data.insert(0, compression_result.additional_unix_extra_data.c_str());
    }
    if(compression_result.entrytype == DIRECTORY_ENTRY) {
        if(lh.fname.back() != '/') {
            lh.fname += '/';
        }
    }

    lh.needed_version = NEEDED_VERSION;
    lh.gp_bitflag = 0x02; // LZMA EOS marker.
    lh.compression = compression_result.cformat;
    lh.last_mod_date = 0;
    lh.last_mod_time = 0;
    lh.crc32 = compression_result.crc32;
    lh.compressed_size = lh.uncompressed_size = 0xFFFFFFFF;
    // Write fake data because the local header must be written before the data.
    // But we don't know the final data size until all data has been read from the
    // ByteQueue.
    lh.extra = pack_zip64(uncompressed_size, compressed_size, local_header_offset);
    lh.extra += pack_unix_extra(t.fi.ue);
    write_localheader(ofile, lh);
    auto data_start_loc = ofile.tell();
    write_file(t.queue, ofile);
    auto data_end_loc = ofile.tell();

    // Fix the header by rewriting it.
    lh.extra = pack_zip64(uncompressed_size, data_end_loc - data_start_loc, local_header_offset);
    lh.extra += pack_unix_extra(t.fi.ue);
    ofile.seek(local_header_offset, SEEK_SET);
    write_localheader(ofile, lh);
    ofile.seek(data_end_loc, SEEK_SET);

    ch.version_made_by = MADE_BY_UNIX << 8 | NEEDED_VERSION;
    ch.version_needed = lh.needed_version;
    ch.bit_flag = lh.gp_bitflag;
    ch.compression_method = lh.compression;
    ch.last_mod_time = lh.last_mod_time;
    ch.last_mod_date = lh.last_mod_date;
    ch.crc32 = lh.crc32;
    ch.compressed_size = lh.compressed_size;
    ch.uncompressed_size = lh.uncompressed_size;
    ch.fname = lh.fname;
    ch.disk_number_start = 0;
    ch.internal_file_attributes = 0;
    ch.external_file_attributes = i.mode << 16;
    ch.local_header_rel_offset = local_header_offset;
    ch.extra_field = lh.extra;
    return ch;
}

void handle_future(File &ofile,
                   CompressionTask &t,
                   std::vector<centralheader> &chs,
                   TaskControl &tc) {
    try {
        chs.push_back(write_entry(ofile, t));
        tc.add_success("OK: " + t.fi.fname);
    } catch(const std::exception &e) {
        std::string msg("FAIL: ");
        msg += e.what();
        tc.add_failure(msg);
    } catch(...) {
        tc.add_failure("FAIL: unknown reason.");
    }
}

bool pop_with_state(File &ofile,
                    task_array &tasks,
                    std::vector<centralheader> &chs,
                    TaskControl &tc,
                    const QueueState state) {
    auto full_entry = std::find_if(
        tasks.begin(), tasks.end(), [state](const auto &up) { return up->queue.state() == state; });
    if(full_entry != tasks.end()) {
        handle_future(ofile, **full_entry, chs, tc);
        tasks.erase(full_entry);
        return true;
    }
    return false;
}

void pop_future(File &ofile, task_array &tasks, std::vector<centralheader> &chs, TaskControl &tc) {
    while(true) {
        if(tc.should_stop()) {
            return;
        }
        if(pop_with_state(ofile, tasks, chs, tc, QueueState::FULL))
            return;
        if(pop_with_state(ofile, tasks, chs, tc, QueueState::SHUTDOWN))
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace

ZipCreator::ZipCreator(const std::string fname) : fname(fname) {}

ZipCreator::~ZipCreator() {
    if(t) {
        t->join();
    }
}

TaskControl *ZipCreator::create(const std::vector<fileinfo> &files, int num_threads) {
    if(tc.state() != TASK_NOT_STARTED) {
        throw std::logic_error("Tried to start an already used packing process.");
    }

    tc.reserve(files.size());
    tc.set_state(TASK_RUNNING);
    t.reset(new std::thread(
        [this](const std::vector<fileinfo> &files, int num_threads) {
            try {
                this->run(files, num_threads);
            } catch(const std::exception &e) {
                printf("Fail: %s\n", e.what());
            } catch(...) {
                printf("Unknown fail.\n");
            }
        },
        files,
        num_threads));
    return &tc;
}

void launch_task(task_array &tasks,
                 const fileinfo &f,
                 const int64_t buffer_size,
                 bool use_lzma,
                 TaskControl &tc) {
    auto t = std::make_unique<CompressionTask>(f, buffer_size);
    ByteQueue *bq_ptr = &t->queue;
    t->result = std::async(std::launch::async, [&f, bq_ptr, use_lzma, &tc]() -> compressresult {
        try {
            compressresult result = compress_entry(f, *bq_ptr, use_lzma, tc);
            bq_ptr->shutdown();
            return result;
        } catch(...) {
            bq_ptr->shutdown();
            throw;
        }
    });
    tasks.push_back(std::move(t));
}

void ZipCreator::run(const std::vector<fileinfo> &files, const int num_threads) {
#ifdef __linux__
    const bool use_lzma = true; // Temporary hack until lzma is fixed on OSX and Windows.
#else
    const bool use_lzma = false;
#endif
    const int64_t queue_size = sizeof(void *) > 4 ? 1024 * 1024 * 1024 : 10 * 1024 * 2014;
    File ofile(fname, "wb");
    endrecord ed;
    std::vector<centralheader> chs;
    task_array tasks;
    assert(num_threads > 0);
    tasks.reserve(num_threads);
    /*
     * Try to always keep as many compression jobs running as there are processors.
     *
     * - a task that is finished is written to the output file
     *
     * - a task that has filled its buffer is written to the output file in a streaming fashion
     *
     * - only one file is being written to the output file at a time
     *
     * The problem comes when a huge file blocks and must be written to the result file.
     * In this case ongoing tasks either finish or fill their buffer and wait. New tasks
     * can not be launched until the big file is fully written.
     */
    for(const auto &f : files) {
        if(tc.should_stop()) {
            break;
        }
        while((int)tasks.size() >= num_threads) {
            pop_future(ofile, tasks, chs, tc);
        }
        launch_task(tasks, f, queue_size, use_lzma, tc);
    }
    while(!tasks.empty()) {
        pop_future(ofile, tasks, chs, tc);
    }
    if(chs.empty()) {
        throw std::runtime_error("All files failed to compress.");
    }
    if(!tc.should_stop()) {
        uint64_t ch_offset = ofile.tell();
        for(const auto &ch : chs) {
            write_central_header(ofile, ch);
        }
        uint64_t ch_end_offset = ofile.tell();

        // ZIP64 eod record
        zip64endrecord z64r;
        z64r.recordsize = 2 + 2 + 4 + 4 + 8 + 8 + 8 + 8;
        z64r.version_made_by = chs[0].version_made_by;
        z64r.version_needed = NEEDED_VERSION;
        z64r.disk_number = 0;
        z64r.dir_start_disk_number = 0;
        z64r.this_disk_num_entries = chs.size();
        z64r.total_entries = chs.size();
        z64r.dir_size = ch_end_offset - ch_offset;
        z64r.dir_offset = ch_offset;
        write_z64_eod_record(ofile, z64r);

        // ZIP64 eod locator
        zip64locator z64l;
        z64l.central_dir_disk_number = 0;
        z64l.central_dir_offset = ch_end_offset;
        z64l.num_disks = 1;
        write_z64_eod_locator(ofile, z64l);

        ed.disk_number = 0;
        ed.central_dir_disk_number = 0;
        ed.this_disk_num_entries = 0xFFFF;
        ed.total_entries = 0XFFFF;
        ed.dir_size = 0xFFFFFFFF;
        ed.dir_offset_start_disk = 0xFFFFFFFF;
        write_end_record(ofile, ed);
    }
    tc.set_state(TASK_FINISHED);
}
