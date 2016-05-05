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


/* Zlib decompression is based on:
 *
 *  zpipe.c: example of proper use of zlib's inflate() and deflate()
 *  Not copyrighted -- provided to the public domain
 *  Version 1.4  11 December 2005  Mark Adler */

#include"zimp.h"
#include"zipdefs.h"
#include"utils.h"
#include"fileutils.h"
#include"file.h"

#include<lzma.h>
#include<zlib.h>

#include<utime.h>
#include<sys/stat.h>
#include<algorithm>
#include<cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include<cstdio>

#include<memory>

#define CHUNK 16384

namespace {

void inflate_to_file(const unsigned char *data_start, uint32_t data_size, FILE *ofile);
void lzma_to_file(const unsigned char *data_start, uint32_t data_size, FILE *ofile);
void unstore_to_file(const unsigned char *data_start, uint32_t data_size, FILE *ofile);

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
void inflate_to_file(const unsigned char *data_start, uint32_t data_size, FILE *ofile) {
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
        strm.avail_in = std::min((size_t)CHUNK, (size_t)(data_size - (current-data_start)));
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
            if (fwrite(out, 1, have, ofile) != have || ferror(ofile)) {
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

void lzma_to_file(const unsigned char *data_start, uint32_t data_size, FILE *ofile) {
    unsigned char out[CHUNK];
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_filter filter[2];
    unsigned int have;

    size_t offset = 2;
    uint16_t properties_size = le16toh(*reinterpret_cast<const uint16_t*>(data_start + offset));
    offset+=2;
    filter[0].id = LZMA_FILTER_LZMA1;
    filter[1].id = LZMA_VLI_UNKNOWN;
    lzma_ret ret = lzma_properties_decode(&filter[0], nullptr, data_start + offset, properties_size);
    offset += properties_size;
    if(ret != LZMA_OK) {
        throw std::runtime_error("Could not decode LZMA properties.");
    }
    ret = lzma_raw_decoder(&strm, &filter[0]);
    free(filter[0].options);
    if(ret != LZMA_OK) {
        throw std::runtime_error("Could not initialize LZMA decoder.");
    }
    std::unique_ptr<lzma_stream, void(*)(lzma_stream*)> lcloser(&strm, lzma_end);

    const unsigned char *current = data_start + offset;
    /* decompress until data ends */
    do {
        if(current >= data_start + data_size) {
            break;
        }
        strm.avail_in = std::min((size_t)CHUNK, (size_t)(data_size - (current-data_start)));
        if (strm.avail_in == 0)
            break;
        strm.next_in = current;
        strm.next_out = out;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = lzma_code(&strm, LZMA_RUN);
            if(ret != LZMA_OK && ret != LZMA_STREAM_END) {
                throw std::runtime_error("Decompression failed.");
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, ofile) != have || ferror(ofile)) {
                throw_system("Could not write to file:");
            }
        } while (strm.avail_out == 0);
        current += CHUNK;
    } while (true);

}


void unstore_to_file(const unsigned char *data_start, uint32_t data_size, FILE *ofile) {
    auto bytes_written = fwrite(data_start, 1, data_size, ofile);
    if(bytes_written != data_size) {
        throw_system("Could not write file fully:");
    }
}

void create_symlink(const unsigned char *data_start, uint32_t data_size, const std::string &outname) {
    std::string symlink_target(data_start, data_start + data_size);
    if(symlink(symlink_target.c_str(), outname.c_str()) != 0) {
        throw_system("Symlink creation failed:");
    }
}

void create_file(const localheader &lh, const centralheader &ch, const unsigned char *data_start, uint32_t data_size, const std::string &outname) {
    decltype(unstore_to_file) *f;
    if(ch.compression_method == ZIP_NO_COMPRESSION) {
        f = unstore_to_file;
    } else if(ch.compression_method == ZIP_DEFLATE) {
        f = inflate_to_file;
    } else if(ch.compression_method == ZIP_LZMA) {
        f = lzma_to_file;
    } else {
        throw std::runtime_error("Unsupported compression format.");
    }
    if(exists_on_fs(outname)) {
        throw std::runtime_error("Already exists, will not overwrite.");
    }
    create_dirs_for_file(outname);
    std::string extraction_name = outname + "$ZIPTMP";
    File ofile(extraction_name.c_str(), "w+b");
    try {
        (*f)(data_start, data_size, ofile.get());
    } catch(...) {
        unlink(extraction_name.c_str());
        throw;
    }
    ofile.flush();

    const std::string &original = lh.gp_bitflag&(1<<2) ? ch.crc32 : lh.crc32;
    auto crc32 = CRC32(ofile);
    if(crc32 != original) {
        printf("%d %d\n", *reinterpret_cast<const uint32_t*>(original.data()),
                *reinterpret_cast<const uint32_t*>(crc32.data()));
        unlink(extraction_name.c_str());
        throw std::runtime_error("XXX CRC32 checksum is invalid: " + lh.fname);
    }
    if(rename(extraction_name.c_str(), outname.c_str()) != 0) {
        unlink(extraction_name.c_str());
        throw_system("Could not rename tmp file to target file.");
    }
}

void create_device(const localheader &lh, const std::string &outname) {
    const std::string &d = lh.unix.data;
    if(d.size() != 8) {
        throw std::runtime_error("Incorrect extra data for character device.");
    }
    uint32_t major = le32toh(*reinterpret_cast<const uint32_t*>(&d[0]));
    uint32_t minor = le32toh(*reinterpret_cast<const uint32_t*>(&d[4]));
    if(mknod(outname.c_str(), S_IFCHR, makedev(major, minor)) != 0) {
        throw_system("Could not create device node:");
    }
}

void do_unpack(const localheader &lh, const centralheader &ch, const unsigned char *data_start, uint32_t data_size, const std::string &outname) {
    uint16_t extattrs = ch.external_file_attributes >> 16;
    if(S_ISDIR(extattrs)) {
        mkdirp(outname);
    } else if(S_ISLNK(extattrs)) {
        if(ch.compression_method != ZIP_NO_COMPRESSION) {
            throw std::runtime_error("Symbolic link stored compressed. Not supported.");
        }
        create_symlink(data_start, data_size, outname);
    } else if(S_ISCHR(extattrs)) {
        create_device(lh, outname);
    } else if(S_ISREG(extattrs)) {
        create_file(lh, ch, data_start, data_size, outname);
    } else {
        throw std::runtime_error("Unknown file type.");
    }
}

void set_permissions(const localheader &lh, const centralheader &ch, const std::string &fname) {
    // This part of the zip spec is poorly documented. :(
    // https://trac.edgewall.org/attachment/ticket/8919/ZipDownload.patch
    chmod(fname.c_str(), ch.external_file_attributes >> 16);
    // Only support mtime if it is in zip64 info.
    // FIXME add support for crazy zip dos format.
    // http://mindprod.com/jgloss/zip.html
    if(lh.unix.atime != 0) {
        // These can fail for various reasons (i.e. no chown privilegde), so ignore return values.
        struct utimbuf tb;
        tb.actime = lh.unix.atime;
        tb.modtime = lh.unix.mtime;
        utime(fname.c_str(), &tb);
        chown(fname.c_str(), lh.unix.uid, lh.unix.gid);
    }
}

}

void unpack_entry(const localheader &lh,
        const centralheader &ch,
        const unsigned char *data_start, uint32_t data_size) {
    try {
        do_unpack(lh, ch, data_start, data_size, lh.fname);
        set_permissions(lh, ch, lh.fname);
        printf("OK: %s\n", lh.fname.c_str());
    } catch(const std::exception &e) {
        printf("FAIL: %s\n", lh.fname.c_str());
        printf("  %s\n", e.what());
    }
}
