/*
 * Copyright (C) 2016 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU General Public License as published
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


#include"zipfile.h"
#include<memory>
#include<cstdio>
#include<stdexcept>

ZipFile::ZipFile(const char *fname) {
    std::unique_ptr<FILE, int(*)(FILE *f)> ifile(fopen(fname, "r"), fclose);
    if(!ifile) {
        throw std::runtime_error("Could not open input file.");
    }
    char header[2];
    fread(header, 2, 1, ifile.get());
    if(header[0] != 'P' || header[1] != 'K') {
        throw std::runtime_error("Not a zip file.");
    }
}
