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
#include <cstdio>
#include <zipfile.h>

/* Fuzzer application for zip parsing. */

int main(int argc, char **argv) {
    try {
        ZipFile zf(argv[1]);
    } catch (const std::exception &e) {
    }
    // Let unexpected errors bubble out.
    // "They should never happen."
    return 0;
}
