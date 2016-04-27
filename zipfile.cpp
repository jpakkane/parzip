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

const constexpr int LOCAL_SIG = 0x04034b50;

namespace {

localheader read_local_entry(File &f) {
    localheader h;
    h.signature = LOCAL_SIG;
    h.needed_version = f.read16le();
    h.gp_bitflag = f.read16le();
    h.compression = f.read16le();
    h.last_mod_time = f.read16le();
    h.last_mod_date = f.read16le();
    f.read(&h.crc32, 4);
    h.compressed_size = f.read32le();
    h.uncompressed_size = f.read32le();
    h.file_name_length = f.read16le();
    h.extra_field_length = f.read16le();
    h.fname.insert(0, h.file_name_length, 'a');
    h.extra.insert(0, h.extra_field_length, 'b');
    f.read(&(h.fname[0]), h.file_name_length);
    f.read(&(h.extra[0]), h.extra_field_length);
    return h;
}

}

ZipFile::ZipFile(const char *fname) : zipfile(fname, "r") {
    while(true) {
        auto curloc = zipfile.tell();
        uint32_t head = zipfile.read32le();
        if(head != LOCAL_SIG) {
            zipfile.seek(curloc);
            break;
        }
        entries.push_back(read_local_entry(zipfile));
        data_offsets.push_back(zipfile.tell());
        zipfile.seek(entries.back().compressed_size, SEEK_CUR);
        if(entries.back().gp_bitflag & (1<<2)) {
            zipfile.seek(3*4, SEEK_CUR);
        }
    }
    zipfile.seek(0, SEEK_END);
    fsize = zipfile.tell();
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
                unpack_entry(entries[i].compression,
                        file_start + data_offsets[i],
                        entries[i].compressed_size,
                        entries[i].fname);
                };
        try {
            futures.emplace_back(std::async(std::launch::async, unstoretask));
        } catch(const std::system_error &) {
            futures.clear();
            futures.emplace_back(std::async(std::launch::async, unstoretask));
        }
    }
}
