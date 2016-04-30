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

#include<cassert>
#include<stdexcept>
#include<sys/stat.h>


bool is_dir(const std::string &s) {
    struct stat sbuf;
    if(stat(s.c_str(), &sbuf) < 0) {
        return false;
    }
    return (sbuf.st_mode & S_IFMT) == S_IFDIR;
}

bool is_file(const std::string &s) {
    struct stat sbuf;
    if(stat(s.c_str(), &sbuf) < 0) {
        return false;
    }
    return (sbuf.st_mode & S_IFMT) == S_IFREG;
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
        if(!is_dir(curdir)) {
            mkdir(curdir.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
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
