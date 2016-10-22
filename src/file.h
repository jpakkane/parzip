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

class MMapper;

class File final {
private:

    FILE *f;
    void read(void *buf, size_t bufsize);

public:

    File() : f(nullptr) {};
    File(const File &) = delete;
    File(File &&other) { f = other.f; other.f = nullptr; }
    File& operator=(File &&other) { f = other.f; other.f = nullptr; return *this; }
    ~File();

    File(const std::string &fname, const char *mode);
    File(FILE *opened);

    operator FILE*() { return f; }

    FILE* get() const { return f; }
    int64_t tell() const;
    int seek(int64_t offset, int whence=SEEK_SET);
    int fileno() const;

    MMapper mmap() const;

    uint64_t size() const;
    void flush();
    void close();

    uint8_t read8();
    uint16_t read16le();
    uint32_t read32le();
    uint64_t read64le();
    uint16_t read16be();
    uint32_t read32be();
    uint64_t read64be();
    std::string read(size_t bufsize);

    void write8(uint8_t i);
    void write16le(uint16_t i);
    void write32le(uint32_t i);
    void write64le(uint64_t i);
    void write16be(uint16_t i);
    void write32be(uint32_t i);
    void write64be(uint64_t);
    void write(const std::string &s);
    void write(const unsigned char *s, uint64_t size);
};
