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

#include"zipcreator.h"
#include"fileutils.h"
#include"file.h"
#include"utils.h"
#include"zipdefs.h"
#include"mmapper.h"

#include<sys/stat.h>

#include<stdexcept>

namespace {

void copy_contents(const File &ifile, File &ofile) {
    auto mmap = ifile.mmap();
    if(fwrite(mmap, 1, mmap.size(), ofile) != mmap.size()) {
        throw_system("Could not write file: ");
    }
}

void write_file(const File &ifile, File &ofile, const localheader &lh) {
    ofile.write32le(LOCAL_SIG);
    ofile.write16le(lh.needed_version);
    ofile.write16le(lh.gp_bitflag);
    ofile.write16le(lh.compression);
    ofile.write16le(lh.last_mod_time);
    ofile.write16le(lh.last_mod_date);
    ofile.write32le(lh.crc32);
    ofile.write32le(lh.compressed_size);
    ofile.write32le(lh.uncompressed_size);
    ofile.write16le(lh.fname.size());
    ofile.write16le(lh.extra.size());
    ofile.write(lh.fname);
    ofile.write(lh.extra);
    copy_contents(ifile, ofile);
}


void write_central_header(File &ofile, const centralheader &ch) {
    ofile.write32le(CENTRAL_SIG);
    ofile.write16le(ch.version_made_by);
    ofile.write16le(ch.version_needed);
    ofile.write16le(ch.bit_flag);
    ofile.write16le(ch.compression_method);
    ofile.write16le(ch.last_mod_time);
    ofile.write16le(ch.last_mod_date);
    ofile.write32le(ch.crc32);
    ofile.write32le(ch.compressed_size);
    ofile.write32le(ch.uncompressed_size);
    ofile.write16le(ch.fname.size());
    ofile.write16le(ch.extra_field.size());
    ofile.write16le(ch.comment.size());
    ofile.write16le(ch.disk_number_start);
    ofile.write16le(ch.internal_file_attributes);
    ofile.write32le(ch.external_file_attributes);
    ofile.write32le(ch.local_header_rel_offset);
    ofile.write(ch.fname);
    ofile.write(ch.extra_field);
    ofile.write(ch.comment);
}

void write_end_record(File &ofile, const endrecord &ed) {
    ofile.write32le(CENTRAL_END_SIG);
    ofile.write16le(ed.disk_number);
    ofile.write16le(ed.central_dir_disk_number);
    ofile.write16le(ed.this_disk_num_entries);
    ofile.write16le(ed.total_entries);
    ofile.write32le(ed.dir_size);
    ofile.write32le(ed.dir_offset_start_disk);
    ofile.write16le(ed.comment.size());
    ofile.write(ed.comment);
}

struct statdata {
    unixextra ue;
    uint16_t mode;
};

statdata get_unix_stats(const std::string &fname) {
    struct stat buf;
    statdata sd;
    if(lstat(fname.c_str(), &buf) != 0) {
        throw_system("Could not get entry stats: ");
    }
    sd.ue.uid = buf.st_uid;
    sd.ue.gid = buf.st_gid;
    sd.ue.atime = buf.st_atim.tv_sec;
    sd.ue.mtime = buf.st_mtim.tv_sec;
    sd.mode = buf.st_mode;
    return sd;
}

template<typename C>
void append_data(std::string &s, const C &c) {
    s.append(reinterpret_cast<const char*>(&c), sizeof(C));
}

std::string pack_unix_extra(unixextra ue) {
    const uint16_t tag = htole16(0x0d);
    const uint16_t size = htole16(4+4+2+2);
    std::string result;
    append_data(result, tag);
    append_data(result, size);
    append_data(result, ue.atime);
    append_data(result, ue.mtime);
    append_data(result, ue.uid);
    append_data(result, ue.gid);
    return result;
}

}

ZipCreator::ZipCreator(const std::string fname) : fname(fname) {

}

void ZipCreator::create(const std::vector<std::string> &files) {
    File ofile(fname, "wb");
    endrecord ed;
    std::vector<centralheader> chs;
    for(const auto &ifname : files) {
        auto stats = get_unix_stats(ifname);
        File ifile(ifname, "rb");
        localheader lh;
        centralheader ch;
        uint64_t local_header_offset = ofile.tell();
        lh.needed_version = NEEDED_VERSION;
        lh.gp_bitflag = 0;
        lh.compression = ZIP_NO_COMPRESSION;
        lh.last_mod_date = 0;
        lh.last_mod_time = 0;
        lh.crc32 = CRC32(ifile);
        lh.compressed_size = lh.uncompressed_size = ifile.size(); // FIXME ZIP64.
        lh.fname = ifname;
        lh.extra = pack_unix_extra(stats.ue);
        write_file(ifile, ofile, lh);

        ch.version_made_by = MADE_BY_UNIX << 8 | NEEDED_VERSION;
        ch.version_needed = lh.needed_version;
        ch.bit_flag = lh.gp_bitflag;
        ch.compression_method = lh.compression;
        ch.last_mod_time = lh.last_mod_time;
        ch.last_mod_date = lh.last_mod_date;
        ch.crc32 = lh.crc32;
        ch.compressed_size = lh.compressed_size;
        ch.uncompressed_size = lh.uncompressed_size;
        ch.fname = ifname;
        ch.disk_number_start = 0;
        ch.internal_file_attributes = 0;
        ch.external_file_attributes = uint32_t(stats.mode) << 16;
        ch.local_header_rel_offset = local_header_offset;
        chs.push_back(ch);
    }
    uint64_t ch_offset = ofile.tell();
    for(const auto &ch : chs) {
        write_central_header(ofile, ch);
    }
    uint64_t ch_end_offset = ofile.tell();
    ed.disk_number = 0;
    ed.central_dir_disk_number = 0;
    ed.this_disk_num_entries = chs.size();
    ed.total_entries = chs.size();
    ed.dir_size = ch_end_offset - ch_offset;
    ed.dir_offset_start_disk = ch_offset;
    write_end_record(ofile, ed);
}
