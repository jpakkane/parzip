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

#include"zipdefs.h"
#include"file.h"
#include"taskcontrol.h"
#include<string>
#include<vector>
#include<thread>

struct FileDisplayInfo {
    std::string fname;
    uint64_t compressed_size;
    uint64_t uncompressed_size;
};

struct DirectoryDisplayInfo {
    std::string dirname;
    std::vector<DirectoryDisplayInfo> dirs;
    std::vector<FileDisplayInfo> files;
};

class ZipFile {

public:
    ZipFile(const char *fname);
    ~ZipFile();

    size_t size() const { return entries.size(); }

    TaskControl* unzip(const std::string &prefix, int num_threads) const;

    const std::vector<localheader> localheaders() const { return entries; }

    DirectoryDisplayInfo build_tree() const;

private:

    void run(const std::string &prefix, int num_threads) const;

    void readLocalFileHeaders();
    void readCentralDirectory();

    File zipfile;
    std::vector<localheader> entries;
    std::vector<centralheader> centrals;
    std::vector<long> data_offsets;

    zip64endrecord z64end;
    zip64locator z64loc;
    endrecord endloc;
    size_t fsize;

    mutable std::unique_ptr<std::thread> t;
    mutable TaskControl tc;
};
