/* lsdpack - standalone LSDj (Little Sound Dj) recorder + player {{{
   Copyright (C) 2018  Johan Kotlinski
   https://www.littlesounddj.com

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. }}} */

#include "getopt.h"

#include <cstdio>

const char* optarg;
int optind = 1;

int getopt(int argc, char* const argv[], const char *optstring) {
    if (optind >= argc) {
        return -1;
    }

    const char dash = argv[optind][0];
    if (dash != '-') {
        return -1;
    }

    int optchar = argv[optind][1];
    while (*optstring) {
        if (*optstring == optchar) {
            ++optind;
            if (optstring[1] == ':') {
                if (optind >= argc) {
                    fprintf(stderr, "Missing argument for -%c\n", optchar);
                    return -1;
                }
                // Read option argument.
                optarg = argv[optind];
                ++optind;
            }
            return optchar;
        }
        ++optstring;
    }

    ++optind;
    return optchar;
}
