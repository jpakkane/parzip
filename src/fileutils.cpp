/*
 * Copyright (C) 2016-2019 Jussi Pakkanen.
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

#include "fileutils.h"
#include "utils.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include <filesystem>
#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {

std::vector<fileinfo> expand_entry(const std::string &fname);

// The C++ filesystem API does not seem to have uids or gids.
// Thus we have to use platform specific code to get this information.

fileinfo get_unix_stats(const std::string &fname) {
    struct stat buf;
    fileinfo sd;
#ifdef _WIN32
    if(stat(fname.c_str(), &buf) != 0) {
#else
    if(lstat(fname.c_str(), &buf) != 0) {
#endif
        throw_system("Could not get entry stats: ");
    }
    sd.fname = fname;
    sd.ue.uid = buf.st_uid;
    sd.ue.gid = buf.st_gid;
#if defined(__APPLE__)
    sd.ue.atime = buf.st_atimespec.tv_sec;
    sd.ue.mtime = buf.st_mtimespec.tv_sec;
#elif defined(_WIN32)
    sd.ue.atime = buf.st_atime;
    sd.ue.mtime = buf.st_mtime;
#else
    sd.ue.atime = buf.st_atim.tv_sec;
    sd.ue.mtime = buf.st_mtim.tv_sec;
#endif
    sd.mode = buf.st_mode;
    sd.fsize = buf.st_size;
    sd.device_id = buf.st_rdev;
    return sd;
} // namespace


std::vector<std::string> handle_dir_platform(const std::string &dirname) {
    std::vector<std::string> entries;
    std::string basename;
    for(const auto &e : fs::directory_iterator(dirname.c_str())) {
        auto basename = e.path().filename();
        if(basename == "." || basename == "..") {
            continue;
        }
        entries.push_back(basename);
    }
    return entries;
}

std::vector<fileinfo> expand_dir(const std::string &dirname) {
    // Always set order to create reproducible zip files.
    std::vector<fileinfo> result;
    auto entries = handle_dir_platform(dirname);
    std::sort(entries.begin(), entries.end());
    std::string fullpath;
    for(const auto &base : entries) {
        fullpath = dirname + '/' + base;
        auto new_ones = expand_entry(fullpath);
        std::move(new_ones.begin(), new_ones.end(), std::back_inserter(result));
    }
    return result;
}

std::vector<fileinfo> expand_entry(const std::string &fname) {
    auto fi = get_unix_stats(fname);
    std::vector<fileinfo> result{fi};
    if(is_dir(fi)) {
        auto new_ones = expand_dir(fname);
        std::move(new_ones.begin(), new_ones.end(), std::back_inserter(result));
        return result;
    } else {
        return result;
    }
}

} // namespace

bool is_dir(const std::string &s) {
    fs::path p{s};
    return fs::is_directory(p);
}

bool is_dir(const fileinfo &f) { return S_ISDIR(f.mode); }

bool is_file(const std::string &s) {
    fs::path p{s};
    return fs::is_regular_file(p);
}

bool is_file(const fileinfo &f) { return S_ISREG(f.mode); }

bool is_symlink(const fileinfo &f) { return S_ISLNK(f.mode); }

bool exists_on_fs(const std::string &s) {
    fs::path p{s};
    return fs::exists(p);
}

void mkdirp(const std::string &s) {
    fs::path p{s};
    fs::create_directories(p);
}

void create_dirs_for_file(const std::string &s) {
    auto lastslash = s.rfind('/');
    if(lastslash == std::string::npos || lastslash == 0) {
        return;
    }
    mkdirp(s.substr(0, lastslash));
}

bool is_absolute_path(const std::string &fname) {
    if(fname.empty()) {
        return false;
    }
    // The C++ standard library seems to define an absolute path as
    // "absolute given the platform we are running on". Zip files
    // must be multiplatform, so we have to roll our own here.
    if(fname.front() == '/' || fname.front() == '\\' ||
            (fname.size() > 2 && fname[1] == ':' && (fname[2] == '/' || fname[2] == '\\'))) {
        return true;
    }
    return false;
}

std::vector<fileinfo> expand_files(const std::vector<std::string> &originals) {
    return std::accumulate(originals.begin(),
                           originals.end(),
                           std::vector<fileinfo>{},
                           [](std::vector<fileinfo> res, const std::string &s) {
                               auto n = expand_entry(s);
                               std::move(n.begin(), n.end(), std::back_inserter(res));
                               return res;
                           });
}
