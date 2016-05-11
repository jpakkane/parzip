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

#include<cstdio>
#include<vector>
#include<string>

#include"zipcreator.h"

int main(int argc, char **argv) {
    if(argc < 3 ) {
        printf("%s <zip file> <files to archive>\n", argv[0]);
        return 1;
    }

    std::vector<std::string> files;
    files.push_back(argv[2]);
    try {
        ZipCreator zc(argv[1]);
        zc.create(files);
    } catch(std::exception &e) {
        printf("Zip creation failed: %s\n", e.what());
        return 1;
    } catch(...) {
        printf("Zip creation failed due to an unknown reason.");
        return 1;
    }
    return 0;
    return 0;
}
