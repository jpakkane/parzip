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
#include"utils.h"

#ifdef _WIN32
#include<WinSock2.h>
#include<windows.h>
#include<direct.h>
#else
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#endif
#include<memory>
#include<array>
#include<cassert>
#include<stdexcept>
#include<algorithm>
#include<numeric>

namespace {

std::vector<fileinfo> expand_entry(const std::string &fname);


fileinfo get_unix_stats(const std::string &fname) {
    struct stat buf;
    fileinfo sd;
#ifdef _WIN32
    if (stat(fname.c_str(), &buf) != 0) {
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
}

#ifdef _WIN32
std::vector<std::string> handle_dir_platform(const std::string &dirname) {
    std::string glob = dirname + "\\*.*";
    std::vector<std::string> entries;
    HANDLE hFind;
    WIN32_FIND_DATA data;

    hFind = FindFirstFile(glob.c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE) {
        throw_system("Could not get directory contents: ");
    }
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            entries.push_back(data.cFileName);
        } while(FindNextFile(hFind, &data));
        FindClose(hFind);
    }
    return entries;
}

#else

std::vector<std::string> handle_dir_platform(const std::string &dirname) {
    std::vector<std::string> entries;
    std::unique_ptr<DIR, int(*)(DIR*)> dirholder(opendir(dirname.c_str()), closedir);
    auto dir = dirholder.get();
    if(!dir) {
        printf("Could not access directory: %s\n", dirname.c_str());
        return entries;
    }
    std::array<char, sizeof(dirent) + NAME_MAX + 1> buf;
    struct dirent *cur = reinterpret_cast<struct dirent*>(buf.data());
    struct dirent *de;
    std::string basename;
    while (readdir_r(dir, cur, &de) == 0 && de) {
        basename = cur->d_name;
        if (basename == "." || basename == "..") {
            continue;
        }
        entries.push_back(basename);
    }
    return entries;
}
#endif

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

}

bool is_dir(const std::string &s) {
    struct stat sbuf;
    if(stat(s.c_str(), &sbuf) < 0) {
        return false;
    }
    return (sbuf.st_mode & S_IFMT) == S_IFDIR;
}

bool is_dir(const fileinfo &f) {
    return S_ISDIR(f.mode);
}

bool is_file(const std::string &s) {
    struct stat sbuf;
    if(stat(s.c_str(), &sbuf) < 0) {
        return false;
    }
    return (sbuf.st_mode & S_IFMT) == S_IFREG;
}

bool is_file(const fileinfo &f) {
    return S_ISREG(f.mode);
}

bool is_symlink(const fileinfo &f) {
    return S_ISLNK(f.mode);
}

bool exists_on_fs(const std::string &s) {
    struct stat sbuf;
    return stat(s.c_str(), &sbuf) == 0;
}


void mkdirp(const std::string &s) {
    if(is_dir(s)) {
        return;
    }
    std::string::size_type offset = 1;
    do {
        auto slash = s.find('/', offset);
        if(slash == std::string::npos) {
            slash = s.size();
        }
        auto curdir = s.substr(0, slash);
        if (!is_dir(curdir)) {
#ifdef _WIN32
            _mkdir(curdir.c_str());
#else
            mkdir(curdir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
            if(!is_dir(curdir)) {
                throw_system("Could not create directory:");
            }
        }
        offset = slash + 1;
    } while(offset <= s.size());
    assert(is_dir(s));
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
    if(fname.front() == '/' || fname.front() == '\\' ||
            (fname.size() > 2 && fname[1] == ':' && (fname[2] == '/' || fname[2] == '\\'))) {
        return true;
    }
    return false;

}

std::vector<fileinfo> expand_files(const std::vector<std::string> &originals) {
    return std::accumulate(originals.begin(), originals.end(), std::vector<fileinfo>{}, [](std::vector<fileinfo> res, const std::string &s) {
        auto n = expand_entry(s);
        std::move(n.begin(), n.end(), std::back_inserter(res));
        return res;
    });
}

