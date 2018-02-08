#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdlib.h>

#define TCP_WINDOW_CAP 4096

#define check(expr)\
    do {\
        if (!(expr)) {\
            fprintf(stderr, "file: %s@%d\nfunction %s:", __FILE__, __LINE__, __FUNCTION__);\
            perror(#expr);\
            exit(1);\
        }\
    } while(0)

void set_fd_limit();

#endif

