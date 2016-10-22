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

#include"fileutils.h"
#include"zipcreator.h"
#include"utils.h"

#ifdef _WIN32
#include<WinSock2.h>
#include<windows.h>
#else
#include<unistd.h>
#endif
#include<thread>
#include<cstdio>
#include<vector>
#include<string>
#include<algorithm>

#include<cassert>

#ifndef _WIN32
using std::max;
#endif

int main(int argc, char **argv) {
    const int num_threads = max((int)std::thread::hardware_concurrency(), 1);
    if(argc < 3) {
        printf("%s <zip file> <files to archive>\n", argv[0]);
        return 1;
    }
    if(exists_on_fs(argv[1])) {
        printf("Output file already exists, will not overwrite.\n");
        return 1;
    }

    std::vector<std::string> filenames;
    for(int i=2; i<argc; i++) {
        filenames.push_back(argv[i]);
        if(filenames.back().empty()) {
            printf("Empty file name not permitted.");
            return 1;
        }
        if(is_absolute_path(filenames.back())) {
            printf("Absolute file names are forbidden in ZIP files.\n");
            return 1;
        }
    }
    if(filenames.empty()) {
        printf("No input files listed.\n");
        return 1;
    }
    std::vector<fileinfo> files;
    try {
        files = expand_files(filenames);
    } catch(const std::exception &e) {
        printf("Scanning input files failed: %s\n", e.what());
        return 1;
    }
    /*
     * First all directory entries so they get created before files that go in them.
     * Then files starting from the biggest ones, because a big file followed by lots of small files
     * causes waste of resources. See zipfile.cpp for the explanation.
     */
    auto midpoint = std::stable_partition(files.begin(), files.end(), [](const fileinfo&fi) { return is_dir(fi); });
    assert(midpoint >= files.begin());
    assert(midpoint <= files.end());
    std::sort(midpoint, files.end(), [](const fileinfo &f1, const fileinfo &f2) { return f1.fsize > f2.fsize; });
    ZipCreator zc(argv[1]);
    try {
        zc.create(files, num_threads);
    } catch(std::exception &e) {
        unlink(argv[1]);
        printf("Zip creation failed: %s\n", e.what());
        return 1;
    } catch(...) {
        unlink(argv[1]);
        printf("Zip creation failed due to an unknown reason.");
        return 1;
    }
    return 0;
}
