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

#include "file.h"
#include "zipdefs.h"
#include "bytequeue.hpp"

#include <string>

class TaskControl;

struct compressresult {
    filetype entrytype;
    uint32_t crc32;
    uint16_t cformat;
    std::string additional_unix_extra_data;
};

compressresult compress_entry(const fileinfo &f, ByteQueue &queue, bool use_lzma, const TaskControl &tc);
