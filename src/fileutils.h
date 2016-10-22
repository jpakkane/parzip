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
#include<string>
#include<vector>

bool is_dir(const std::string &s);
bool is_dir(const fileinfo &f);
bool is_symlink(const fileinfo &f);
bool is_file(const std::string &s);
bool is_file(const fileinfo &f);
bool exists_on_fs(const std::string &s);

bool is_absolute_path(const std::string &fname);

void mkdirp(const std::string &s);
void create_dirs_for_file(const std::string &s);

std::vector<fileinfo> expand_files(const std::vector<std::string> &originals);

#if defined _WIN32
#if !defined S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFDIR) == _S_IFDIR)
#endif
#if !defined S_ISREG
#define S_ISREG(m) (((m) & _S_IFREG) == _S_IFREG)
#endif
#if !defined S_ISLNK
#define S_ISLNK(m) (false)
#endif
#endif