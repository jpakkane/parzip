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

#include"utils.h"
#include"mmapper.h"

#if _WIN32
#include<winsock2.h>
#include<windows.h>
#endif

#include<zlib.h>

#include<cerrno>
#include<cassert>
#include<cstring>

#include<stdexcept>
#include<string>

#ifndef _WIN32
using std::min;
#endif

void throw_system(const char *msg) {
    std::string error(msg);
    assert(errno != 0);
    if(error.back() != ' ') {
        error += ' ';
    }
    error += strerror(errno);
    throw std::runtime_error(error);
}

uint32_t CRC32(const unsigned char *buf, uint64_t bufsize) {
    uint32_t crcvalue = crc32(0, Z_NULL, 0);
    const uint64_t blocksize = 1024*1024;
    for(uint64_t offset=0; offset < bufsize; offset+=blocksize) {
        crcvalue = crc32(crcvalue, buf+offset, min(blocksize, bufsize-offset));
    }
    return crcvalue;
}

uint32_t CRC32(File &f) {
    MMapper mmap = f.mmap();
    return CRC32(mmap, mmap.size());
}
