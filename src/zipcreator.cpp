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

#include"zipcreator.h"
#include"fileutils.h"

#include<stdexcept>

ZipCreator::ZipCreator(const std::string fname) : fname(fname) {

}

void ZipCreator::create(const std::vector<std::string> &files) {
    if(exists_on_fs(fname)) {
        throw std::runtime_error("Output file already exists, will not overwrite.");
    }
}
