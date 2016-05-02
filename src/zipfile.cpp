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

#include"zipfile.h"
#include"zimp.h"
#include"utils.h"
#include<endian.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<cstdio>
#include<cerrno>
#include<cstring>
#include<stdexcept>
#include<memory>
#include<future>
#include<thread>

namespace {

void unpack_zip64_sizes(const std::string &extra_field, uint64_t &compressed_size, uint64_t &uncompressed_size) {
    size_t offset = 0;
    while(offset < extra_field.size()) {
        uint16_t header_id = le16toh(*reinterpret_cast<const uint16_t*>(&extra_field[offset]));
        offset+=2;
        uint16_t data_size = le16toh(*reinterpret_cast<const uint16_t*>(&extra_field[offset]));
        offset+=2;
        if(header_id == ZIP_EXTRA_ZIP64) {
            uncompressed_size = le64toh(*reinterpret_cast<const uint64_t*>(&extra_field[offset]));
            offset += 8;
            compressed_size = le64toh(*reinterpret_cast<const uint64_t*>(&extra_field[offset]));
            return;
        }
        offset += data_size;
    }
    throw std::runtime_error("Entry extra field did not contain ZIP64 extension, file can not be parsed.");
}

void unpack_unix(const std::string &extra, unixextra &unix) {
    size_t offset = 0;
    while(offset < extra.size()) {
        uint16_t header_id = le16toh(*reinterpret_cast<const uint16_t*>(&extra[offset]));
        offset+=2;
        uint16_t data_size = le16toh(*reinterpret_cast<const uint16_t*>(&extra[offset]));
        offset+=2;
        auto extra_end = offset + data_size;
        if(header_id == ZIP_EXTRA_UNIX) {
            unix.atime = le32toh(*reinterpret_cast<const uint32_t*>(&extra[offset]));
            offset += 4;
            unix.mtime = le32toh(*reinterpret_cast<const uint32_t*>(&extra[offset]));
            offset += 4;
            unix.uid = le16toh(*reinterpret_cast<const uint16_t*>(&extra[offset]));
            offset += 2;
            unix.gid = le16toh(*reinterpret_cast<const uint16_t*>(&extra[offset]));
            offset += 2;
            unix.data = std::string(&extra[offset], &extra[extra_end]);
            return;
        }
        offset += data_size;
    }
    unix.atime = 0;
}

localheader read_local_entry(File &f) {
    localheader h;
    uint16_t fname_length, extra_length;
    h.needed_version = f.read16le();
    h.gp_bitflag = f.read16le();
    h.compression = f.read16le();
    h.last_mod_time = f.read16le();
    h.last_mod_date = f.read16le();
    f.read(&h.crc32, 4);
    h.compressed_size = f.read32le();
    h.uncompressed_size = f.read32le();
    fname_length = f.read16le();
    extra_length = f.read16le();
    h.fname.insert(0, fname_length, 'a');
    h.extra.insert(0, extra_length, 'b');
    f.read(&(h.fname[0]), fname_length);
    f.read(&(h.extra[0]), extra_length);
    if(h.compressed_size == 0xFFFFFFFF || h.uncompressed_size == 0xFFFFFFFF) {
        unpack_zip64_sizes(h.extra, h.compressed_size, h.uncompressed_size);
    }
    unpack_unix(h.extra, h.unix);
    return h;
}

centralheader read_central_entry(File &f) {
    centralheader c;
    uint16_t fname_length, extra_length, comment_length;
    c.version_made_by = f.read16le();
    c.version_needed = f.read16le();
    c.bit_flag = f.read16le();
    c.compression_method = f.read16le();
    c.last_mod_time = f.read16le();
    c.last_mod_date = f.read16le();
    f.read(&c.crc32, 4);
    c.compressed_size = f.read32le();
    c.uncompressed_size = f.read32le();
    fname_length = f.read16le();
    extra_length = f.read16le();
    comment_length = f.read16le();
    c.disk_number_start = f.read16le();
    c.internal_file_attributes = f.read16le();
    c.external_file_attributes = f.read32le();
    c.local_header_rel_offset = f.read32le();

    c.fname.insert(0, fname_length, 'a');
    c.extra_field.insert(0, extra_length, 'b');
    c.comment.insert(0, comment_length, 'c');
    f.read(&(c.fname[0]), fname_length);
    f.read(&(c.extra_field[0]), extra_length);
    f.read(&(c.comment[0]), comment_length);
    return c;
}

zip64endrecord read_z64_central_end(File &f) {
    zip64endrecord er;
    er.recordsize = f.read64le();
    er.version_made_by = f.read16le();
    er.version_needed = f.read16le();
    er.disk_number = f.read32le();
    er.dir_start_disk_number = f.read32le();
    er.this_disk_num_entries = f.read64le();
    er.total_entries = f.read64le();
    er.dir_size = f.read64le();
    er.dir_offset = f.read64le();
    auto ext_size = er.recordsize - 2 - 2 - 4 - 4 - 8 - 8 - 8 - 8;
    er.extensible.insert(0, ext_size, 'a');
    f.read(&(er.extensible[0]), ext_size);
    return er;
}

zip64locator read_z64_locator(File &f) {
    zip64locator loc;
    loc.central_dir_disk_number = f.read32le();
    loc.central_dir_offset = f.read64le();
    loc.num_disks = f.read32le();
    return loc;
}

endrecord read_end_record(File &f) {
    endrecord el;
    el.disk_number = f.read16le();
    el.central_dir_disk_number = f.read16le();
    el.this_disk_num_entries = f.read16le();
    el.total_entries = f.read16le();
    el.dir_size = f.read32le();
    el.dir_offset_start_disk = f.read32le();
    auto csize = f.read16le();
    el.comment.insert(0, csize, 'a');
    f.read(&(el.comment[0]), csize);
    return el;
}


}

ZipFile::ZipFile(const char *fname) : zipfile(fname, "r") {
    readLocalFileHeaders();
    readCentralDirectory();
    if(entries.size() != centrals.size()) {
        std::string msg("Mismatch. File has ");
        msg += std::to_string(entries.size());
        msg += " local entries but ";
        msg += std::to_string(centrals.size());
        msg += " central entries.";
        throw std::runtime_error(msg);
    }
    auto id = zipfile.read32le();
    if(id == ZIP64_CENTRAL_END_SIG) {
        z64end = read_z64_central_end(zipfile);
        if(z64end.total_entries != entries.size()) {
            throw std::runtime_error("File is broken, zip64 directory has incorrect number of entries.");
        }
        id = zipfile.read32le();
        if(id == ZIP64_CENTRAL_LOCATOR_SIG) {
            z64loc = read_z64_locator(zipfile);
            id = zipfile.read32le();
        }
    }
    if(id != CENTRAL_END_SIG) {
        throw std::runtime_error("Zip file broken, missing end locator.");
    }
    endloc = read_end_record(zipfile);
    if(endloc.total_entries != entries.size()) {
        throw std::runtime_error("Zip file broken, end record has incorrect directory size.");
    }
    zipfile.seek(0, SEEK_END);
    fsize = zipfile.tell();
}

void ZipFile::readLocalFileHeaders() {
    while(true) {
        auto curloc = zipfile.tell();
        uint32_t head = zipfile.read32le();
        if(head != LOCAL_SIG) {
            zipfile.seek(curloc);
            break;
        }
        entries.emplace_back(read_local_entry(zipfile));
        if(entries.back().gp_bitflag & 1) {
            throw std::runtime_error("This file is encrypted. Encrypted ZIP archives are not supported.");
        }
        data_offsets.push_back(zipfile.tell());
        zipfile.seek(entries.back().compressed_size, SEEK_CUR);
        if(entries.back().gp_bitflag & (1<<2)) {
            zipfile.seek(3*4, SEEK_CUR);
        }
    }
}

void ZipFile::readCentralDirectory() {
    while(true) {
        auto curloc = zipfile.tell();
        uint32_t head = zipfile.read32le();
        if(head != CENTRAL_SIG) {
            zipfile.seek(curloc);
            break;
        }
        centrals.push_back(read_central_entry(zipfile));
    }
}

void ZipFile::unzip() const {
    int fd = zipfile.fileno();
    if(fd < 0) {
        throw_system("Could not open zip file:");
    }
    auto unmapper = [&](void* d) { munmap(d, fsize); };
    std::unique_ptr<void, decltype(unmapper)> data(mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fd, 0),
            unmapper);
    close(fd);
    if(!data) {
        throw_system("Could not mmap zip file:");
    }
    unsigned char *file_start = (unsigned char*)(data.get());
    // Clearing will call all destructors, so all tasks will get run.
    std::vector<std::future<void>> futures;
    for(size_t i=0; i<entries.size(); i++) {
        auto unstoretask = [this, file_start, i](){
                unpack_entry(entries[i],
                        centrals[i],
                        file_start + data_offsets[i],
                        entries[i].compressed_size);
                };
        try {
            futures.emplace_back(std::async(std::launch::async, unstoretask));
        } catch(const std::system_error &) {
            futures.clear();
            futures.emplace_back(std::async(std::launch::async, unstoretask));
        }
    }
}
