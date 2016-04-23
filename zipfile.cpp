/*
 * Copyright (C) 2016 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU General Public License as published
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


#include"zipfile.h"
#include<cstdio>
#include<stdexcept>
#include<memory>

const constexpr int LOCAL_SIG = 0x04034b50;

namespace {

localheader read_local_entry(FILE *f) {
    localheader h;
    h.signature = LOCAL_SIG;
    fread(&h.needed_version, sizeof(uint16_t), 1, f);
    fread(&h.gp_bitflag, sizeof(uint16_t), 1, f);
    fread(&h.compression, sizeof(uint16_t), 1, f);
    fread(&h.last_mod_time, sizeof(uint16_t), 1, f);
    fread(&h.last_mod_date, sizeof(uint16_t), 1, f);
    fread(&h.crc32, 4, 1, f);
    fread(&h.compressed_size, sizeof(uint32_t), 1, f);
    fread(&h.uncompressed_size, sizeof(uint32_t), 1, f);
    fread(&h.file_name_length, sizeof(uint16_t), 1, f);
    fread(&h.extra_field_length, sizeof(uint16_t), 1, f);
    h.fname.insert(0, h.file_name_length, 'a');
    h.extra.insert(0, h.extra_field_length, 'b');
    fread(&(h.fname[0]), h.file_name_length, 1, f);
    fread(&(h.extra[0]), h.extra_field_length, 1, f);
    return h;
}

}

ZipFile::ZipFile(const char *fname) {
    std::unique_ptr<FILE, int(*)(FILE *f)> ifile(fopen(fname, "r"), fclose);
    if(!ifile) {
        throw std::runtime_error("Could not open input file.");
    }
    while(true) {
        auto curloc = ftell(ifile.get());
        uint32_t head;
        fread(&head, sizeof(uint32_t), 1, ifile.get());
        if(head != LOCAL_SIG) {
            fseek(ifile.get(), curloc, SEEK_SET);
            break;
        }
        entries.push_back(read_local_entry(ifile.get()));
        fseek(ifile.get(), entries.back().compressed_size, SEEK_CUR);
        if(entries.back().gp_bitflag & (1<<2)) {
            fseek(ifile.get(), 3*4, SEEK_CUR);
        }
    }
    printf("This file has %d entries.\n", (int)entries.size());
}
