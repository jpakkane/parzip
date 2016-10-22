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

#if defined(_WIN32)
#include<winsock2.h>
#include<windows.h>
#include<io.h>
#else
#include<sys/mman.h>
#include<fcntl.h>
#endif

#include"mmapper.h"
#include"file.h"
#include"utils.h"

#if defined(_WIN32)
MMapper::MMapper(const File &f) {
    map_size = f.size();
    h = CreateFileMapping((HANDLE)_get_osfhandle(f.fileno()), nullptr, PAGE_READONLY, 0, 0, nullptr);
    addr = MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
}

#else
MMapper::MMapper(const File &f) {
    map_size = f.size();
    auto fdnum = f.fileno();
    if(map_size == 0) {
        addr = nullptr;
    } else {
        addr = ::mmap(nullptr, map_size, PROT_READ, MAP_PRIVATE, fdnum, 0);
        if(addr == MAP_FAILED) {
            throw_system("Could not mmap file:");
        }
    }
}
#endif

MMapper::MMapper(MMapper && other) {
    if(&other == this) {
        return;
    }
    this->addr = other.addr;
    this->map_size = other.map_size;
    other.addr = nullptr;
}

MMapper& MMapper::operator=(MMapper &&other) {
    if(&other != this) {
        this->addr = other.addr;
        this->map_size = other.map_size;
        other.addr = nullptr;
    }
    return *this;
}

MMapper::~MMapper() {
#if defined(_WIN32)
    UnmapViewOfFile(addr);
    CloseHandle(h);
#else
  if(addr) {
        munmap(addr, map_size);
    }
#endif
}
