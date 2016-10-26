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

#include<cassert>

struct try_result {
    bool was_num;
    long value;
    bool has_next;
    char next_char;
};

template<class T>
try_result tryint(T &b, T&e) {
    try_result r;
    r.was_num = false;
    r.value = 0;
    r.has_next = true;
    assert(b != e);
    while(b != e) {
        r.next_char = *b++;
        if (r.next_char >= '0' && r.next_char <= '9') {
            r.was_num = true;
            r.value = 10*r.value + (r.next_char - '0');
        } else {
            return r;
        }
    }
    r.has_next = false;
    return r;
}

/*
 * Evaluate natural order using only forward iterators. Natural order means
 * that embedded numbers are sorted according to numerical value instead of
 * ASCIIbetical order. Returns -1, 0 or 1 depending on whether the first
 * string is less than, equal or greater than the second, respectively.
 */

template<class T>
int natural_compare(T str1_begin, T str1_end, T str2_begin, T str2_end) {
    while(true) {
        auto end1 = str1_begin == str1_end;
        auto end2 = str2_begin == str2_end;
        if(end1) {
            if(!end2) {
                return -1;
            }
            return 0;
        }
        if(end2) {
            return 1;
        }
        auto res1 = tryint(str1_begin, str1_end);
        auto res2 = tryint(str2_begin, str2_end);
        if(res1.was_num) {
            if(res2.was_num) {
                if(res1.value < res2.value) {
                    return -1;
                }
                if(res1.value > res2.value) {
                    return 1;
                }
            } else {
                // str1 was int but str2 was not.
                return res2.next_char < '0' ? -1 : 1;
            }
        } else if(res2.was_num) {
            assert(!res1.was_num);
            // str1 was not int but str2 was;
            return res1.next_char < '0' ? -1 : 1;
        } else {
            assert(!res1.was_num);
            assert(!res2.was_num);
        }
        if(res1.next_char < res2.next_char) {
            return -1;
        }
        if(res1.next_char > res2.next_char) {
            return 1;
        }
    }
}

/*
 * Overload for types that have .begin() and .end().
 */
template<class T1, class T2>
int natural_compare(T1 &str1, T2 &str2) {
    return natural_compare(str1.begin(), str1.end(), str2.begin(), str2.end());
}

/*
 * Comparator for use in std::sort.
 */
template<class T>
int natural_less(T &s1, T &s2) {
    return natural_compare(s1, s2) < 0;
}
