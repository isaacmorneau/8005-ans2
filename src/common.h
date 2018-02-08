#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define TCP_WINDOW_CAP 4096

#define check(expr)\
    do {\
        if (!(expr)) {\
            fprintf(stderr, "%s::%s::%d\n\t", __FILE__, __FUNCTION__, __LINE__);\
            if (errno != 0) {\
                perror(#expr);\
            } else { \
                fprintf(stderr, #expr "\n");\
            }\
            exit(1);\
        }\
    } while(0)

void set_fd_limit();

#ifdef __cplusplus
}
#endif

#endif
