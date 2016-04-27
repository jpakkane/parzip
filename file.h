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

#include<cstdio>
#include<string>

class File final {
private:

    FILE *f;

public:

    File() = delete;
    File(const File &) = delete;
    File(File &&other) { f = other.f; other.f = nullptr; }
    ~File();

    File(const std::string &fname, const char *mode);
    File(FILE *opened);

    long tell();
    int seek(long offset, int whence=SEEK_SET);
    int fileno() const;

    uint8_t read8();
    uint16_t read16();
    uint32_t read32();
    void read(void *buf, size_t bufsize);
};
