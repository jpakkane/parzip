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
#define ZIP_LZMA 14

#define ZIP_EXTRA_ZIP64 1
#define ZIP_EXTRA_UNIX 0xd

const constexpr uint32_t LOCAL_SIG = 0x04034b50;
const constexpr uint32_t CENTRAL_SIG = 0x02014b50;
const constexpr uint32_t CENTRAL_END_SIG = 0x06054b50;
const constexpr uint32_t ZIP64_CENTRAL_END_SIG = 0x06064b50;
const constexpr uint32_t ZIP64_CENTRAL_LOCATOR_SIG = 0x07064b50;
const constexpr uint32_t NEEDED_VERSION = 63; // LZMA

const constexpr uint16_t MADE_BY_UNIX = 3;

// LZMA has heavy startup cost and needs some data to get going.
// If file to be compressed is smaller than this, just store it as is.
// This number was picked from a hat and might not be optimal.
const constexpr int TOO_SMALL_FOR_LZMA = 512;

enum filetype {
    FILE_ENTRY,
    DIRECTORY_ENTRY,
    SYMLINK_ENTRY,
    CHARDEV_ENTRY,
    UNKNOWN_ENTRY,
};

struct unixextra {
    uint32_t atime;
    uint32_t mtime;
    uint16_t uid;
    uint16_t gid;
    std::string data;
};

struct fileinfo {
    std::string fname;
    unixextra ue;
    uint32_t mode;
    uint64_t fsize;
    uint64_t device_id; // VERIFY: is big enough to hold dev_t?
};

struct localheader {
    uint16_t needed_version;
    uint16_t gp_bitflag;
    uint16_t compression;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint32_t crc32;
    uint64_t compressed_size; // On disk header format is 32 bits, but this is 64 bits to be able to store zip64 offsets, too.
    uint64_t uncompressed_size;
    std::string fname;
    std::string extra;
    unixextra unix;
};

struct centralheader {
    uint16_t version_made_by;
    uint16_t version_needed;
    uint16_t bit_flag;
    uint16_t compression_method;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint32_t crc32;
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

struct zip64endrecord {
    uint64_t recordsize;
    uint16_t version_made_by;
    uint16_t version_needed;
    uint32_t disk_number;
    uint32_t dir_start_disk_number;
    uint64_t this_disk_num_entries;
    uint64_t total_entries;
    uint64_t dir_size;
    uint64_t dir_offset;
    std::string extensible;
};

struct zip64locator {
    uint32_t central_dir_disk_number;
    uint64_t central_dir_offset;
    uint32_t num_disks;
};

struct endrecord {
    uint16_t disk_number;
    uint16_t central_dir_disk_number;
    uint16_t this_disk_num_entries;
    uint16_t total_entries;
    uint32_t dir_size;
    uint32_t dir_offset_start_disk;
    std::string comment;
};
