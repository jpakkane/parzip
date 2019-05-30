/*
 * Copyright (C) 2019 Jussi Pakkanen.
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

// An extremely simple unit testing framework.

#pragma once

#include <stdio.h>
#include <unistd.h>

static void __attribute__((noreturn)) st_die(const char *fname, const char *lname, int line_num) {
    printf("FAIL:\n file: %s\n function: %s\n line: %d\n", fname, lname, line_num);
    fflush(stdout);
    fflush(stderr);
    _exit(1);
}

static void st_test_start(const char *test_func_name) {
    printf("Test start: %s\n", test_func_name);
}

static void st_test_end(const char *test_func_name) {
    fflush(stdout);
    fflush(stderr);
    printf("Test   end: %s\n", test_func_name);
}

#define ST_STRINGIFY(x) #x
#define ST_TOSTRING(x) ST_STRINGIFY(x)

#define ST_ASSERT(stmt)                                                                            \
    do {                                                                                           \
        if (!(stmt))                                                                               \
            st_die(__FILE__, __PRETTY_FUNCTION__, __LINE__);                                       \
    } while (0);

#define ST_TEST(func_name)                                                                         \
    do {                                                                                           \
        st_test_start(ST_TOSTRING(func_name));                                                     \
        func_name();                                                                               \
        st_test_end(ST_TOSTRING(func_name));                                                       \
    } while (0);
