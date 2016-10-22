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
#include"fileutils.h"
#include"mmapper.h"

#include<sys/stat.h>
#ifdef _WIN32
#else
#include<unistd.h>
#include<lzma.h> // libxz does not work on MSVC.
#endif
#include<cassert>

#include<memory>
#include<stdexcept>

#include<zlib.h>
#include<cstdio>


namespace {

compressresult store_file(const fileinfo &fi);

bool is_compressible(const unsigned char *buf, const size_t bufsize) {
    assert(bufsize > 0);
    const int blocksize = min((size_t)32*1024, bufsize/2);
    const double required_ratio = 0.92; // Stetson-Harrison constant
    if(blocksize < 16) {
        return false;
    }
    std::unique_ptr<unsigned char[]> out(new unsigned char [blocksize*2]);
    // Use zlib for compression test because it is a lot faster than LZMA.
    auto checkpoint = buf + bufsize/2; // Files usually start with some compressible data like an index.
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    auto ret = deflateInit(&strm, -1);
    if(ret != Z_OK) {
        throw std::runtime_error("Zlib init failed.");
    }
    std::unique_ptr<z_stream, int(*)(z_stream*)> zcloser(&strm, deflateEnd);
    strm.next_in = (unsigned char*)checkpoint; // Zlib is const-broken.
    strm.avail_in = blocksize;
    strm.next_out = out.get();
    strm.avail_out = 2*blocksize;
    ret = deflate(&strm, Z_FINISH);
    if(ret != Z_STREAM_END) {
        throw std::runtime_error("Zlib compression test failed.");
    }
    return ((double)strm.total_out)/blocksize < required_ratio;
}

#ifdef _WIN32
compressresult compress_lzma(const fileinfo &fi) {
    throw std::runtime_error("Liblzma does not work with VS.");
}

#else

compressresult compress_lzma(const fileinfo &fi) {
    const int CHUNK=1024*1024;
    File infile(fi.fname, "rb");
    MMapper buf = infile.mmap();
    if(!is_compressible(buf, buf.size())) {
        return store_file(fi);
    }
    FILE *f = tmpfile();
    std::unique_ptr<unsigned char[]> out(new unsigned char [CHUNK]);
    uint32_t filter_size;
    if(!f) {
        throw_system("Could not create temp file: ");
    }
    compressresult result{File(f), FILE_ENTRY, CRC32(buf, buf.size()), ZIP_LZMA, fi};
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
            strm.next_out = out.get();
        }
        ret = lzma_code(&strm, action);
        if(strm.avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = CHUNK - strm.avail_out;
            if (fwrite(out.get(), 1, write_size, result.f) != write_size || ferror(result.f)) {
                throw_system("Could not write to file:");
            }
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

    fflush(result.f);
    return result;
}

#endif

compressresult store_file(const fileinfo &fi) {
    FILE *f = fopen(fi.fname.c_str(), "r");
    if(!f) {
        throw_system("Could not open input file: ");
    }

    compressresult result{File(f), FILE_ENTRY, (uint32_t)-1, ZIP_NO_COMPRESSION, fi};
    auto mmap = result.f.mmap();
    result.crc32 = CRC32(mmap, mmap.size());
    // Status writer expects location to be at the end of the file.
    result.f.seek(0, SEEK_END);
    return result;
}

compressresult create_dir(const fileinfo &fi) {
    compressresult r{nullptr, DIRECTORY_ENTRY, CRC32(nullptr, 0), ZIP_NO_COMPRESSION, fi};
    return r;
}

compressresult create_symlink(const fileinfo &fi) {
#ifdef _WIN32
    throw std::runtime_error("Symlinks not supported on Windows.");
#else
    std::unique_ptr<unsigned char[]> buf(new unsigned char[fi.fsize+1]);
    auto r = readlink(fi.fname.c_str(), (char*)buf.get(), fi.fsize+1);
    if(r<0) {
        throw_system("Could not read symlink contents: ");
    }
    if((uint64_t)r > fi.fsize) {
        throw std::runtime_error("Symlink size changed while packing.");
    }

    FILE *tf = tmpfile();
    if(!tf) {
        throw_system("Could not create temp file: ");
    }
    compressresult result{File(tf), FILE_ENTRY, CRC32(buf.get(), r), ZIP_NO_COMPRESSION, fi};
    result.f.write(buf.get(), r);
    result.f.flush();
    return result;
#endif
}

}

compressresult compress_entry(const fileinfo &f) {
    if(S_ISREG(f.mode)) {
        if(f.fsize < TOO_SMALL_FOR_LZMA) {
            return store_file(f);
        }
        return compress_lzma(f);
    }
    if(S_ISDIR(f.mode)) {
        return create_dir(f);
    }
    if(S_ISLNK(f.mode)) {
        return create_symlink(f);
    }
    throw std::runtime_error("Unknown file type.");
}
