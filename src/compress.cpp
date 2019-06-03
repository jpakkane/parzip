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

#include "compress.h"
#include "file.h"
#include "fileutils.h"
#include "mmapper.h"
#include "taskcontrol.h"
#include "utils.h"
#include <portable_endian.h>

#include <cstring>
#include <sys/stat.h>
#ifdef _WIN32
#else
#include <lzma.h> // libxz does not work on MSVC.
#include <unistd.h>
#endif

#if defined(__linux__)
#include <sys/sysmacros.h>
#endif

#include <cassert>

#include <memory>
#include <stdexcept>

#include <cstdio>
#include <zlib.h>

#ifndef _WIN32
using std::min;
#endif

namespace {

compressresult store_file(const fileinfo &fi, ByteQueue &queue);

bool is_compressible(const unsigned char *buf, const size_t bufsize) {
    assert(bufsize > 0);
    const int blocksize = min((size_t)32 * 1024, bufsize / 2);
    const double required_ratio = 0.92; // Stetson-Harrison constant
    if(blocksize < 16) {
        return false;
    }
    std::unique_ptr<unsigned char[]> out(new unsigned char[blocksize * 2]);
    // Use zlib for compression test because it is a lot faster than LZMA.
    auto checkpoint =
        buf + bufsize / 2; // Files usually start with some compressible data like an index.
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    auto ret = deflateInit(&strm, -1);
    if(ret != Z_OK) {
        throw std::runtime_error("Zlib init failed.");
    }
    std::unique_ptr<z_stream, int (*)(z_stream *)> zcloser(&strm, deflateEnd);
    strm.next_in = (unsigned char *)checkpoint; // Zlib is const-broken.
    strm.avail_in = blocksize;
    strm.next_out = out.get();
    strm.avail_out = 2 * blocksize;
    ret = deflate(&strm, Z_FINISH);
    if(ret != Z_STREAM_END) {
        throw std::runtime_error("Zlib compression test failed.");
    }
    return ((double)strm.total_out) / blocksize < required_ratio;
}

compressresult compress_zlib(const fileinfo &fi, ByteQueue &queue, const TaskControl &tc) {
    const int CHUNK = 1024 * 1024;
    std::unique_ptr<unsigned char[]> out(new unsigned char[CHUNK]);
    File infile(fi.fname, "rb");
    MMapper buf = infile.mmap();
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    auto ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if(ret != Z_OK) {
        throw std::runtime_error("Zlib init failed.");
    }
    std::unique_ptr<z_stream, int (*)(z_stream *)> zcloser(&strm, deflateEnd);
    compressresult result{FILE_ENTRY, CRC32(buf, buf.size()), ZIP_DEFLATE, ""};
    strm.avail_in = buf.size();
    strm.next_in = buf;

    do {
        strm.avail_out = CHUNK;
        strm.next_out = out.get();
        ret = deflate(&strm, Z_FINISH); /* no bad return value */
        tc.throw_if_stopped();
        assert(ret != Z_STREAM_ERROR); /* state not clobbered */
        int write_size = CHUNK - strm.avail_out;
        queue.push(out.get(), write_size);
    } while(strm.avail_out == 0);
    assert(strm.avail_in == 0); /* all input will be used */

    /* done when last data in file processed */
    assert(ret == Z_STREAM_END); /* stream will be complete */

    return result;
}

#ifdef _WIN32
compressresult compress_lzma(const fileinfo &fi, ByteQueue &queue, const TaskControl &tc) {
    throw std::runtime_error("Liblzma does not work with VS.");
}

#else

compressresult compress_lzma(const fileinfo &fi, ByteQueue &queue, const TaskControl &tc) {
    const int CHUNK = 1024 * 1024;
    File infile(fi.fname, "rb");
    MMapper buf = infile.mmap();
    if(!is_compressible(buf, buf.size())) {
        return store_file(fi, queue);
    }
    std::unique_ptr<unsigned char[]> out(new unsigned char[CHUNK]);
    uint32_t filter_size;
    compressresult result{FILE_ENTRY, CRC32(buf, buf.size()), ZIP_LZMA, ""};
    lzma_options_lzma opt_lzma;
    lzma_stream strm = LZMA_STREAM_INIT;
    if(lzma_lzma_preset(&opt_lzma, LZMA_PRESET_DEFAULT)) {
        throw std::runtime_error("Unsupported LZMA preset.");
    }
    lzma_filter filter[2];
    filter[0].id = LZMA_FILTER_LZMA1;
    filter[0].options = &opt_lzma;
    filter[1].id = LZMA_VLI_UNKNOWN;

    lzma_ret ret = lzma_raw_encoder(&strm, filter);
    if(ret != LZMA_OK) {
        throw std::runtime_error("Could not create LZMA encoder.");
    }
    if(lzma_properties_size(&filter_size, filter) != LZMA_OK) {
        throw std::runtime_error("Could not determine LZMA properties size.");
    } else {
        std::string x(filter_size, 'X');
        if(lzma_properties_encode(filter, (unsigned char *)x.data()) != LZMA_OK) {
            throw std::runtime_error("Could not encode filter properties.");
        }
        uint8_t write_byte;
        // This is what Python's lzma lib does. Copy it
        // without understanding.
        write_byte = 9;
        queue.push((char *)&write_byte, sizeof(write_byte));
        write_byte = 4;
        queue.push((char *)&write_byte, sizeof(write_byte));
        uint16_t lefilter = htole16(filter_size);
        queue.push((char *)&lefilter, sizeof(lefilter));
        queue.push(x.data(), x.size());
    }
    std::unique_ptr<lzma_stream, void (*)(lzma_stream *)> lcloser(&strm, lzma_end);

    strm.avail_in = buf.size();
    strm.next_in = buf;
    /* compress until data ends */
    lzma_action action = LZMA_RUN;
    while(true) {
        if(strm.total_in >= buf.size()) {
            action = LZMA_FINISH;
        } else {
            strm.avail_out = CHUNK;
            strm.next_out = out.get();
        }
        ret = lzma_code(&strm, action);
        tc.throw_if_stopped();
        if(strm.avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = CHUNK - strm.avail_out;
            queue.push(out.get(), write_size);
            strm.next_out = out.get();
            strm.avail_out = CHUNK;
        }

        if(ret != LZMA_OK) {
            if(ret == LZMA_STREAM_END) {
                break;
            }
            throw std::runtime_error("Compression failed.");
        }
    }

    return result;
}

#endif

compressresult store_file(const fileinfo &fi, ByteQueue &queue) {
    FILE *f = fopen(fi.fname.c_str(), "r");
    if(!f) {
        throw_system("Could not open input file: ");
    }
    File infile(f);

    compressresult result{FILE_ENTRY, (uint32_t)-1, ZIP_NO_COMPRESSION, ""};
    auto mmap = infile.mmap();
    result.crc32 = CRC32(mmap, mmap.size());
    queue.push(mmap, mmap.size());
    return result;
}

compressresult create_dir(const fileinfo &, ByteQueue &) {
    compressresult r{DIRECTORY_ENTRY, CRC32(nullptr, 0), ZIP_NO_COMPRESSION, ""};
    return r;
}

compressresult create_symlink(const fileinfo &fi, ByteQueue &queue) {
#ifdef _WIN32
    throw std::runtime_error("Symlinks not supported on Windows.");
#else
    std::unique_ptr<unsigned char[]> buf(new unsigned char[fi.fsize + 1]);
    auto r = readlink(fi.fname.c_str(), (char *)buf.get(), fi.fsize + 1);
    if(r < 0) {
        throw_system("Could not read symlink contents: ");
    }
    if((uint64_t)r > fi.fsize) {
        throw std::runtime_error("Symlink size changed while packing.");
    }
    buf[r] = '\0';

    FILE *tf = tmpfile();
    if(!tf) {
        throw_system("Could not create temp file: ");
    }
    // Zip spec says that symlink target should be in Unix extra data but other
    // programs put it in the file data. Let's do both to be sure.
    auto final_fi = fi;
    final_fi.ue.data.insert(0, (const char *)buf.get());
    std::string edata{(char *)buf.get()};
    compressresult result{FILE_ENTRY, CRC32(buf.get(), r), ZIP_NO_COMPRESSION, edata};
    queue.push(edata.data(), edata.size());
    return result;
#endif
}

#ifndef _WIN32
compressresult create_chrdev(const fileinfo &fi, ByteQueue &) {
    std::string buf(8, 'x');
    FILE *tf = tmpfile();
    if(!tf) {
        throw_system("Could not create temp file: ");
    }
    auto native_major_byte = major(fi.device_id);
    auto native_minor_byte = minor(fi.device_id);
    auto major_byte = htole32(native_major_byte);
    auto minor_byte = htole32(native_minor_byte);

    memcpy(&buf[0], &major_byte, 4);
    memcpy(&buf[0] + 4, &minor_byte, 4);
    auto final_fi = fi;
    std::string ue_data{buf};
    compressresult result{
        CHARDEV_ENTRY, CRC32((const unsigned char *)&buf[0], 0), ZIP_NO_COMPRESSION, ue_data};
    return result;
}
#endif

} // namespace

compressresult
compress_entry(const fileinfo &f, ByteQueue &queue, bool use_lzma, const TaskControl &tc) {
    if(S_ISREG(f.mode)) {
        if(f.fsize < TOO_SMALL_FOR_LZMA) {
            return store_file(f, queue);
        }
        return use_lzma ? compress_lzma(f, queue, tc) : compress_zlib(f, queue, tc);
    }
    if(S_ISDIR(f.mode)) {
        return create_dir(f, queue);
    }
    if(S_ISLNK(f.mode)) {
        return create_symlink(f, queue);
    }
#ifndef _WIN32
    if(S_ISCHR(f.mode)) {
        return create_chrdev(f, queue);
    }
#endif
    std::string error("Unknown file type: ");
    error += f.fname;
    throw std::runtime_error(error);
}
