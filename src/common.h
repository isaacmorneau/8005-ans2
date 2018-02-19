#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define TCP_WINDOW_CAP 4096

#define ensure(expr)\
    do {\
        if (!(expr)) {\
            fprintf(stderr, "%s::%s::%d\n\t", __FILE__, __FUNCTION__, __LINE__);\
            perror(#expr);\
            exit(1);\
        }\
    } while(0)

#define ensure_nonblock(expr)\
    do {\
        if (!(expr) && errno != EAGAIN && errno != EWOULDBLOCK) {\
            fprintf(stderr, "%s::%s::%d\n\t", __FILE__, __FUNCTION__, __LINE__);\
            perror(#expr);\
            exit(1);\
        }\
    } while(0)

void set_fd_limit();

#ifdef __cplusplus
}
#endif

#endif
