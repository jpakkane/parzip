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
#include"endian.h"
#include<cassert>

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
    assert(f != nullptr);
}

File::~File() {
    fclose(f);
}

long File::tell() {
    return ftell(f);
}
int File::seek(long offset, int whence) {
    return fseek(f, offset, whence);
}

int File::fileno() const {
    return ::fileno(f);
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
