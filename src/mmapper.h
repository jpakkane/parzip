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

#pragma once

#ifdef _WIN32
#include<winsock2.h>
#include<windows.h>
#endif
#include<cstdint>

class File;

class MMapper final {
public:
    explicit MMapper(const File &file);
    MMapper(const MMapper&) = delete;
    MMapper(MMapper && other);
    MMapper& operator=(const MMapper &) = delete;
    MMapper& operator=(MMapper &&other);
    ~MMapper();

    uint64_t size() const { return map_size; }

    operator unsigned char*() { return reinterpret_cast<unsigned char*>(addr); }

private:
    void *addr;
    uint64_t map_size;
#if defined(_WIN32)
    HANDLE h;
#endif
};
