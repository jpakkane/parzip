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

#pragma once
#include "taskcontrol.h"
#include "zipdefs.h"
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class ZipCreator final {

public:
    ZipCreator(const std::string fname);
    ~ZipCreator();

    TaskControl *create(const std::vector<fileinfo> &files, int num_threads);

private:
    void run(const std::vector<fileinfo> &files, const int num_threads);

    std::unique_ptr<std::thread> t;
    std::string fname;
    TaskControl tc;
};
