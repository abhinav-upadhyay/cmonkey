#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>

#define test(expr, ...) if (expr) { \
    ; } else { \
    fprintf(stderr, __VA_ARGS__); \
    abort(); \
    }
#endif