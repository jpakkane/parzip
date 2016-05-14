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

#include"compress.h"
#include"file.h"
#include"utils.h"
#include"mmapper.h"

#include<sys/stat.h>
#include<unistd.h>

#include<memory>
#include<stdexcept>

#include<lzma.h>
#include<cstdio>

#define CHUNK 16384

namespace {

compressresult compress_lzma(const std::string &s) {
    File infile(s, "rb");
    MMapper buf = infile.mmap();
    FILE *f = tmpfile();
    unsigned char out[CHUNK];
    uint32_t filter_size;
    if(!f) {
        throw_system("Could not create temp file: ");
    }
    compressresult result{File(f), FILE_ENTRY, CRC32(buf, buf.size()), ZIP_LZMA};
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
        if(lzma_properties_encode(filter, (unsigned char*)x.data()) != LZMA_OK) {
            throw std::runtime_error("Could not encode filter properties.");
        }
        result.f.write8(9); // This is what Python's lzma lib does. Copy it without understanding.
        result.f.write8(4);
        result.f.write16le(filter_size);
        result.f.write(x);
    }
    std::unique_ptr<lzma_stream, void(*)(lzma_stream*)> lcloser(&strm, lzma_end);

    strm.avail_in = buf.size();
    strm.next_in = buf;
    /* compress until data ends */
    lzma_action action = LZMA_RUN;
    while(true) {
        if(strm.total_in >= buf.size()) {
            action = LZMA_FINISH;
        } else {
            strm.avail_out = CHUNK;
            strm.next_out = out;
        }
        ret = lzma_code(&strm, action);
        if(strm.avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = CHUNK - strm.avail_out;
            if (fwrite(out, 1, write_size, result.f) != write_size || ferror(result.f)) {
                throw_system("Could not write to file:");
            }
            strm.next_out = out;
            strm.avail_out = CHUNK;
        }

        if(ret != LZMA_OK) {
            if(ret == LZMA_STREAM_END) {
                break;
            }
            throw std::runtime_error("Compression failed.");
        }
    }

    fflush(result.f);
    return result;
}

compressresult create_dir(const std::string &f) {
    compressresult r{nullptr, DIRECTORY_ENTRY, CRC32(reinterpret_cast<const unsigned char*>(&f), 0), ZIP_NO_COMPRESSION};
    return r;
}

compressresult create_symlink(const fileinfo &f) {
    std::unique_ptr<unsigned char[]> buf(new unsigned char[f.fsize+1]);
    auto r = readlink(f.fname.c_str(), (char*)buf.get(), f.fsize+1);
    if(r<0) {
        throw_system("Could not read symlink contents: ");
    }
    if((uint64_t)r > f.fsize) {
        throw std::runtime_error("Symlink size changed while packing.");
    }

    FILE *tf = tmpfile();
    if(!tf) {
        throw_system("Could not create temp file: ");
    }
    compressresult result{File(tf), FILE_ENTRY, CRC32(buf.get(), r), ZIP_NO_COMPRESSION};
    result.f.write(buf.get(), r);
    result.f.flush();
    return result;
}

}

compressresult compress_entry(const fileinfo &f) {
    if(S_ISREG(f.mode)) {
        return compress_lzma(f.fname);
    }
    if(S_ISDIR(f.mode)) {
        return create_dir(f.fname);
    }
    if(S_ISLNK(f.mode)) {
        return create_symlink(f);
    }
    throw std::runtime_error("Unknown file type.");
}
