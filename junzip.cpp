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

#include"zipfile.h"

int main(int argc, char **argv) {
    if(argc != 2 ) {
        printf("%s <zip file>\n", argv[0]);
        return 1;
    }
    ZipFile f(argv[1]);
    printf("Zip file %s has %d entries.\n", argv[1], (int)f.size());
    f.unzip();
    return 0;
}
