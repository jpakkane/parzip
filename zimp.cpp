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


/* This file is based on:
 *
 *  zpipe.c: example of proper use of zlib's inflate() and deflate()
 *  Not copyrighted -- provided to the public domain
 *  Version 1.4  11 December 2005  Mark Adler */

#include"zimp.h"
#include"zipdefs.h"
#include"utils.h"
#include"fileutils.h"
#include "zlib.h"

#include<algorithm>
#include<cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include<cstdio>

#include<memory>

#define CHUNK 16384

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
void inflate_to_file(const unsigned char *data_start, uint32_t data_size, const std::string &outname) {
    std::unique_ptr<FILE, int(*)(FILE *f)> ofile(fopen(outname.c_str(), "wb"), fclose);
    if(!ofile) {
        throw_system("Could not open input file:");
    }
    FILE *dest = ofile.get();
    int ret;
    unsigned have;
    z_stream strm;
    const unsigned char *current = data_start;
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, -15);
    if (ret != Z_OK)
        throw std::runtime_error("Could not init zlib.");
    std::unique_ptr<z_stream, int (*)(z_stream_s*)> zcloser(&strm, inflateEnd);

    /* decompress until deflate stream ends or end of file */
    do {
        if(current >= data_start + data_size) {
            break;
        }
        strm.avail_in = std::min((long)CHUNK, data_size - (current-data_start));
        if (strm.avail_in == 0)
            break;
        strm.next_in = const_cast<unsigned char*>(current); // zlib header is const-broken

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                throw std::runtime_error(strm.msg);
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                throw_system("Could not write to file:");
            }
        } while (strm.avail_out == 0);
        current += CHUNK;
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);
/*
    if(Z_STREAM_END != Z_OK) {
        throw std::runtime_error("Decompression failed.");
    }
*/
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

void unstore_to_file(const unsigned char *data_start, uint32_t data_size, const std::string &outname) {
    std::unique_ptr<FILE, int(*)(FILE *f)> ofile(fopen(outname.c_str(), "wb"), fclose);
    if(!ofile) {
        throw_system("Could not open input file:");
    }
    auto bytes_written = fwrite(data_start, 1, data_size, ofile.get());
    if(bytes_written != data_size) {
        throw_system("Could not write file fully:");
    }
}

void unpack_entry(int compression_method, const unsigned char *data_start, uint32_t data_size, const std::string &outname) {
    decltype(unstore_to_file) *f;
    if(compression_method == ZIP_NO_COMPRESSION) {
        f = unstore_to_file;
    } else if(compression_method == ZIP_DEFLATE) {
        f = inflate_to_file;
    } else {
        throw std::runtime_error("Unsupported compression format.");
    }
    if(exists_on_fs(outname)) {
        throw std::runtime_error("Already exists, will not overwrite.");
    }
    create_dirs_for_file(outname);
    (*f)(data_start, data_size, outname);
}
