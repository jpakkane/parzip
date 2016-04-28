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

#include<cstdint>
#include<string>

#define ZIP_NO_COMPRESSION 0
#define ZIP_DEFLATE 8

const constexpr int LOCAL_SIG = 0x04034b50;
const constexpr int CENTRAL_SIG = 0x02014b50;

struct localheader {
    uint16_t needed_version;
    uint16_t gp_bitflag;
    uint16_t compression;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint8_t crc32[4];
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    std::string fname;
    std::string extra;
};

struct centralheader {
    uint16_t version_made_by;
    uint16_t version_needed;
    uint16_t bit_flag;
    uint16_t compression_method;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint8_t crc32[4];
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    //file name length                2 bytes
    //extra field length              2 bytes
    //file comment length             2 bytes
    uint16_t disk_number_start;
    uint16_t internal_file_attributes;
    uint32_t external_file_attributes;
    uint32_t local_header_rel_offset;

    std::string fname;
    std::string extra_field;
    std::string comment;
};
