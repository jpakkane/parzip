/*
 * Copyright (C) 2016 Jussi Pakkanen.
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

#include"zipfile.h"
#include"utils.h"
#include"fileutils.h"
#include"mmapper.h"
#include"naturalorder.h"
#include<portable_endian.h>
#ifdef _WIN32
#include<winsock2.h>
#include<windows.h>
#else
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#endif
#include<cstdio>
#include<cerrno>
#include<cstring>
#include<stdexcept>
#include<future>
#include<thread>
#include<algorithm>
#include "decompress.h"

#ifndef _WIN32
using std::max;
#endif

namespace {

void unpack_zip64_sizes(const std::string &extra_field, uint64_t &compressed_size, uint64_t &uncompressed_size) {
    size_t offset = 0;
    while(offset < extra_field.size()) {
        uint16_t header_id = le16toh(*reinterpret_cast<const uint16_t*>(&extra_field[offset]));
        offset+=2;
        uint16_t data_size = le16toh(*reinterpret_cast<const uint16_t*>(&extra_field[offset]));
        offset+=2;
        if(header_id == ZIP_EXTRA_ZIP64) {
            uncompressed_size = le64toh(*reinterpret_cast<const uint64_t*>(&extra_field[offset]));
            offset += 8;
            compressed_size = le64toh(*reinterpret_cast<const uint64_t*>(&extra_field[offset]));
            return;
        }
        offset += data_size;
    }
    throw std::runtime_error("Entry extra field did not contain ZIP64 extension, file can not be parsed.");
}

void unpack_unix(const std::string &extra, unixextra &unix) {
    size_t offset = 0;
    while(offset < extra.size()) {
        if(offset + 4 >= extra.size()) {
            throw std::runtime_error("Malformed extra data.");
        }
        uint16_t header_id = le16toh(*reinterpret_cast<const uint16_t*>(&extra.at(offset)));
        offset+=2;
        uint16_t data_size = le16toh(*reinterpret_cast<const uint16_t*>(&extra.at(offset)));
        offset+=2;
        auto extra_end = offset + data_size;
        if(header_id == ZIP_EXTRA_UNIX) {
            unix.atime = le32toh(*reinterpret_cast<const uint32_t*>(&extra.at(offset)));
            offset += 4;
            unix.mtime = le32toh(*reinterpret_cast<const uint32_t*>(&extra.at(offset)));
            offset += 4;
            unix.uid = le16toh(*reinterpret_cast<const uint16_t*>(&extra.at(offset)));
            offset += 2;
            unix.gid = le16toh(*reinterpret_cast<const uint16_t*>(&extra.at(offset)));
            offset += 2;
            if(offset > extra_end || extra_end > extra.size()) {
                throw std::runtime_error("Malformed Unix extra data.");
            }
            unix.data = std::string(&extra[offset], extra[extra_end]);
            return;
        }
        offset += data_size;
    }
    unix.atime = 0;
}

void check_filename(const std::string &fname) {
    if(fname.size() == 0) {
        throw std::runtime_error("Empty filename in directory");
    }
    if(is_absolute_path(fname)) {
        throw std::runtime_error("Archive has an absolute filename which is forbidden.");
    }
}

localheader read_local_entry(File &f) {
    localheader h;
    uint16_t fname_length, extra_length;
    h.needed_version = f.read16le();
    h.gp_bitflag = f.read16le();
    h.compression = f.read16le();
    h.last_mod_time = f.read16le();
    h.last_mod_date = f.read16le();
    h.crc32 = f.read32le();
    h.compressed_size = f.read32le();
    h.uncompressed_size = f.read32le();
    fname_length = f.read16le();
    extra_length = f.read16le();
    h.fname = f.read(fname_length);
    h.extra = f.read(extra_length);
    if(h.compressed_size == 0xFFFFFFFF || h.uncompressed_size == 0xFFFFFFFF) {
        unpack_zip64_sizes(h.extra, h.compressed_size, h.uncompressed_size);
    }
    unpack_unix(h.extra, h.unix);
    check_filename(h.fname);
    return h;
}

centralheader read_central_entry(File &f) {
    centralheader c;
    uint16_t fname_length, extra_length, comment_length;
    c.version_made_by = f.read16le();
    c.version_needed = f.read16le();
    c.bit_flag = f.read16le();
    c.compression_method = f.read16le();
    c.last_mod_time = f.read16le();
    c.last_mod_date = f.read16le();
    c.crc32 = f.read32le();
    c.compressed_size = f.read32le();
    c.uncompressed_size = f.read32le();
    fname_length = f.read16le();
    extra_length = f.read16le();
    comment_length = f.read16le();
    c.disk_number_start = f.read16le();
    c.internal_file_attributes = f.read16le();
    c.external_file_attributes = f.read32le();
    c.local_header_rel_offset = f.read32le();

    c.fname = f.read(fname_length);
    c.extra_field = f.read(extra_length);
    c.comment = f.read(comment_length);
    return c;
}

zip64endrecord read_z64_central_end(File &f) {
    zip64endrecord er;
    er.recordsize = f.read64le();
    er.version_made_by = f.read16le();
    er.version_needed = f.read16le();
    er.disk_number = f.read32le();
    er.dir_start_disk_number = f.read32le();
    er.this_disk_num_entries = f.read64le();
    er.total_entries = f.read64le();
    er.dir_size = f.read64le();
    er.dir_offset = f.read64le();
    auto ext_size = er.recordsize - 2 - 2 - 4 - 4 - 8 - 8 - 8 - 8;
    er.extensible = f.read(ext_size);
    return er;
}

zip64locator read_z64_locator(File &f) {
    zip64locator loc;
    loc.central_dir_disk_number = f.read32le();
    loc.central_dir_offset = f.read64le();
    loc.num_disks = f.read32le();
    return loc;
}

endrecord read_end_record(File &f) {
    endrecord el;
    el.disk_number = f.read16le();
    el.central_dir_disk_number = f.read16le();
    el.this_disk_num_entries = f.read16le();
    el.total_entries = f.read16le();
    el.dir_size = f.read32le();
    el.dir_offset_start_disk = f.read32le();
    auto csize = f.read16le();
    el.comment = f.read(csize);
    return el;
}

void wait_for_slot(std::vector<std::future<UnpackResult>> &entries,
                   const int num_threads,
                   TaskControl &tc) {
    if((int)entries.size() < num_threads)
        return;
    while(true) {
        auto finished = std::find_if(entries.begin(), entries.end(), [](const std::future<UnpackResult> &e) {
            return e.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
        });
        if(finished != entries.end()) {
            auto r = finished->get();
            if(r.success) {
                tc.add_success(r.msg);
            } else {
                tc.add_failure(r.msg);
            }
            entries.erase(finished);
            return;
        }
        if(tc.should_stop()) {
            return;
        }
        std::this_thread::yield();
    }
}

void order_entries(DirectoryDisplayInfo &d) {
    std::sort(d.dirs.begin(), d.dirs.end(), [](const DirectoryDisplayInfo &d1, const DirectoryDisplayInfo &d2) {
        return natural_less(d1.dirname, d2.dirname);
    });
    std::sort(d.files.begin(), d.files.end(), [](const FileDisplayInfo &f1, const FileDisplayInfo &f2) {
        return natural_less(f1.fname, f2.fname);
    });
    for(auto &de : d.dirs) {
        order_entries(de);
    }
}

}

ZipFile::ZipFile(const char *fname) : zipfile(fname, "rb") {
    readLocalFileHeaders();
    readCentralDirectory();
    if(entries.size() != centrals.size()) {
        std::string msg("Mismatch. File has ");
        msg += std::to_string(entries.size());
        msg += " local entries but ";
        msg += std::to_string(centrals.size());
        msg += " central entries.";
        throw std::runtime_error(msg);
    }
    auto id = zipfile.read32le();
    if(id == ZIP64_CENTRAL_END_SIG) {
        z64end = read_z64_central_end(zipfile);
        if(z64end.total_entries != entries.size()) {
            throw std::runtime_error("File is broken, zip64 directory has incorrect number of entries.");
        }
        id = zipfile.read32le();
        if(id == ZIP64_CENTRAL_LOCATOR_SIG) {
            z64loc = read_z64_locator(zipfile);
            id = zipfile.read32le();
        }
    }
    if(id != CENTRAL_END_SIG) {
        throw std::runtime_error("Zip file broken, missing end of central directory.");
    }
    endloc = read_end_record(zipfile);
    if(endloc.total_entries != 0xFFFF && endloc.total_entries != entries.size()) {
        throw std::runtime_error("Zip file broken, end record has incorrect directory size.");
    }
    zipfile.seek(0, SEEK_END);
    fsize = zipfile.tell();
}

ZipFile::~ZipFile() {
    if(t) {
        t->join();
    }
}

void ZipFile::readLocalFileHeaders() {
    while(true) {
        auto curloc = zipfile.tell();
        uint32_t head = zipfile.read32le();
        if(head != LOCAL_SIG) {
            zipfile.seek(curloc);
            break;
        }
        entries.emplace_back(read_local_entry(zipfile));
        if(entries.back().gp_bitflag & 1) {
            throw std::runtime_error("This file is encrypted. Encrypted ZIP archives are not supported.");
        }
        data_offsets.push_back(zipfile.tell());
        zipfile.seek(entries.back().compressed_size, SEEK_CUR);
        if(entries.back().gp_bitflag & (1<<2)) {
            zipfile.seek(3*4, SEEK_CUR);
        }
    }
}

void ZipFile::readCentralDirectory() {
    while(true) {
        auto curloc = zipfile.tell();
        uint32_t head = zipfile.read32le();
        if(head != CENTRAL_SIG) {
            zipfile.seek(curloc);
            break;
        }
        centrals.push_back(read_central_entry(zipfile));
    }
}

TaskControl* ZipFile::unzip(const std::string &prefix, int num_threads) const {
    if(num_threads < 0) {
        num_threads = max((int)std::thread::hardware_concurrency(), 1);
    }
    if(tc.state() != TASK_NOT_STARTED) {
        throw std::logic_error("Tried to start an already used packing process.");
    }
    int fd = zipfile.fileno();
    if(fd < 0) {
        throw_system("Could not open zip file:");
    }

    tc.reserve(entries.size());
    tc.set_state(TASK_RUNNING);
    t.reset(new std::thread([this](const std::string prefix, int num_threads) {
        try {
            this->run(prefix, num_threads);
        } catch(const std::exception &e) {
            printf("Fail: %s\n", e.what());
        } catch(...) {
            printf("Unknown fail.\n");
        }
    }, prefix, num_threads));
    return &tc;
}

void ZipFile::run(const std::string &prefix, int num_threads) const {
    MMapper map(zipfile);

    unsigned char *file_start = map;
    std::vector<std::future<UnpackResult>> futures;
    futures.reserve(num_threads);
    for(size_t i=0; i<entries.size(); i++) {
        wait_for_slot(futures, num_threads, tc);
        if(tc.should_stop()) {
            break;
        }
        auto unstoretask = [this, file_start, i, &prefix](){
            return unpack_entry(prefix, entries[i],
                                centrals[i],
                                file_start + data_offsets[i],
                                entries[i].compressed_size,
                                tc);
        };
        futures.emplace_back(std::async(std::launch::async, unstoretask));
    }
    for(auto &f : futures) {
        auto r = f.get();
        if(r.success) {
            tc.add_success(r.msg);
        } else {
            tc.add_failure(r.msg);
        }
    }
    tc.set_state(TASK_FINISHED);
}


DirectoryDisplayInfo ZipFile::build_tree() const {
    DirectoryDisplayInfo root;
    for(const auto &e : entries) {
        DirectoryDisplayInfo *current = &root;
        std::string fname = e.fname;
        auto index = fname.find('/');
        while(index != std::string::npos) {
            std::string dirpart = fname.substr(0, index);
            fname = fname.substr(index+1);
            auto dir = std::find_if(current->dirs.begin(), current->dirs.end(),
                    [&dirpart] (const DirectoryDisplayInfo &d) {return dirpart == d.dirname;});
            if(dir != current->dirs.end()) {
                current = &(*dir);
            } else {
                DirectoryDisplayInfo tmp;
                tmp.dirname = dirpart;
                current->dirs.emplace_back(std::move(tmp));
                current = &current->dirs.back();
            }
            index = fname.find('/');
        }
        if(fname.empty()) {
            // Zip files mark directory entries by ending them with a slash,
            // don't create a file entry.
            continue;
        }
        FileDisplayInfo tmp{fname, e.compressed_size, e.uncompressed_size};
        current->files.emplace_back(std::move(tmp));
    }
    order_entries(root);
    return root;
}

