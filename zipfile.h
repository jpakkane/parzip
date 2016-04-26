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

class ZipFile {

public:
    ZipFile(const char *fname);

    size_t size() const { return entries.size(); }

    void unzip() const;

private:

    std::string zipfile;
    std::vector<localheader> entries;
    std::vector<long> data_offsets;
    size_t fsize;
};
