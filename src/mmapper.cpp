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

#include<sys/mman.h>
#include<fcntl.h>

#include"mmapper.h"
#include"utils.h"

MMapper::MMapper(int fd, uint64_t size) : map_size(size) {
    if(size == 0) {
        addr = nullptr;
    } else {
        addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if(addr == MAP_FAILED) {
            throw_system("Could not mmap file:");
        }
    }
}

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
    if(addr) {
        munmap(addr, map_size);
    }
}
