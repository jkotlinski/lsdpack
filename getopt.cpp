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

    return -1;
}
