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

#include"file.h"
#include"utils.h"
#include"mmapper.h"
#include<portable_endian.h>
#include<sys/stat.h>

File::File(const std::string &fname, const char *mode) {
    f = fopen(fname.c_str(), mode);
    if(!f) {
        std::string msg("Could not open file ");
        msg += fname;
        msg += ":";
        throw_system(msg.c_str());
    }

}

File::File(FILE *opened) : f(opened) {
}

File::~File() {
    if(f) {
        fclose(f);
    }
}

void File::close() {
    if (f) {
        fclose(f);
        f = nullptr;
    }
}

int64_t File::tell() const {
#ifdef _WIN32
    return _ftelli64(f);
#else
    return ftell(f);
#endif
}
int File::seek(int64_t offset, int whence) {
#ifdef _WIN32
    return _fseeki64(f, offset, whence);
#else
    return fseek(f, offset, whence);
#endif
}

int File::fileno() const {
    return ::fileno(f);
}

MMapper File::mmap() const {
    return MMapper(*this);
}

void File::read(void *buf, size_t bufsize) {
    if(fread(buf, 1, bufsize, f) != bufsize) {
        throw_system("Could not read data:");
    }
}

uint8_t File::read8() {
    uint8_t r;
    read(&r, sizeof(r));
    return r;
}

uint16_t File::read16le() {
    uint16_t r;
    read(&r, sizeof(r));
    return le16toh(r);

}

uint32_t File::read32le() {
    uint32_t r;
    read(&r, sizeof(r));
    return le32toh(r);
}

uint64_t File::read64le() {
    uint64_t r;
    read(&r, sizeof(r));
    return le64toh(r);
}

uint16_t File::read16be() {
    uint16_t r;
    read(&r, sizeof(r));
    return be16toh(r);

}

uint32_t File::read32be() {
    uint32_t r;
    read(&r, sizeof(r));
    return be32toh(r);
}

uint64_t File::read64be() {
    uint64_t r;
    read(&r, sizeof(r));
    return be64toh(r);
}

std::string File::read(size_t bufsize) {
    std::string buf(bufsize, 'X');
    read(&buf[0], bufsize);
    return buf;
}

uint64_t File::size() const {
    struct stat buf;
    if(fstat(fileno(), &buf) != 0) {
        throw_system("Statting self failed:");
    }
    return buf.st_size;
}

void File::flush() {
    if(fflush(f) != 0) {
        throw_system("Flushing data failed:");
    }
}

void File::write8(uint8_t i) {
    write(reinterpret_cast<const unsigned char*>(&i), sizeof(i));
}

void File::write16le(uint16_t i) {
    uint16_t c = htole16(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c));
}

void File::write32le(uint32_t i) {
    uint32_t c = htole32(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c));

}

void File::write64le(uint64_t i) {
    uint64_t c = htole64(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c));

}

void File::write16be(uint16_t i) {
    uint16_t c = htobe16(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c));

}

void File::write32be(uint32_t i) {
    uint32_t c = htobe32(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c));

}

void File::write64be(uint64_t i) {
    uint64_t c = htobe64(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c));

}

void File::write(const std::string &s) {
    this->write(reinterpret_cast<const unsigned char*>(s.data()), s.size());
}

void File::write(const unsigned char *s, uint64_t size) {
    if(fwrite(s, 1, size, f) != size) {
        throw_system("Could not write data:");
    }

}

