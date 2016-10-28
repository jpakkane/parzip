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
#include<thread>

#ifdef _WIN32
#include<WinSock2.h>
#include<Windows.h>
#endif

#include"zipfile.h"

int main(int argc, char **argv) {
    const int num_threads = -1;
    if(argc != 2 ) {
        printf("%s <zip file>\n", argv[0]);
        return 1;
    }
    int num_failures;
    try {
        size_t i = 0;
        ZipFile f(argv[1]);
        TaskControl *tc = f.unzip("", num_threads);
        size_t total_tasks = tc->total();
        while(i<total_tasks) {
            if(i>=tc->finished()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                auto txt = tc->entry(i++);
                printf("%s\n", txt.c_str());
            }
        }
        printf("Success: %ld\n", (long)tc->successes());
        printf("Fail:    %ld\n", (long)tc->failures());
        num_failures = (int)tc->failures();
    } catch(std::exception &e) {
        printf("Unpacking failed: %s\n", e.what());
        return 1;
    } catch(...) {
        printf("Unpacking failed due to an unknown reason.");
        return 1;
    }
    return num_failures;
}
