#include "logging.h"
#include <stdio.h>
#include <stdatomic.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>


atomic_int total_clients = 0;

static FILE * log_fd = 0;

void init_logging(const char * path) {
    if (path != 0) {
        log_fd = fopen(path, "wb");
    } else {
        log_fd = fopen("logging", "wb");
    }
}

void close_logging() {
    if (log_fd) {
        fclose(log_fd);
        log_fd = 0;
    }
}

//returns microsecond timestamp
unsigned int timestamp() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (1e-6*tv.tv_sec) + tv.tv_usec;
}

int laccept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len) {
    int ret;
    ret = accept(socket, address, address_len);
    if (ret != -1) {
        fprintf(log_fd,"%d %d o\n",timestamp(), ret);
    }
    return ret;
}

int lconnect(int socket, const struct sockaddr *address, socklen_t address_len) {
    int ret;
    ret = connect(socket, address, address_len);
    if (ret != -1) {
        fprintf(log_fd,"%d %d o\n",timestamp(), socket);
    }
    return ret;
}

ssize_t lsend(int socket, const void *buffer, size_t length, int flags) {
    ssize_t ret;
    ret = send(socket, buffer, length, flags);
    if (ret == -1) {
        switch (errno) {
            case EPIPE://closed
            case ECONNRESET://reset
                fprintf(log_fd,"%d %d c\n",timestamp(), socket);
                break;
            default:
                break;
        }
    } else {
        fprintf(log_fd,"%d %d s %d\n",timestamp(), socket, (int)ret);
    }
    return ret;
}

ssize_t lrecv(int socket, void *buffer, size_t length, int flags) {
    ssize_t ret;
    ret = recv(socket, buffer, length, flags);
    if (ret == -1) {
        switch (errno) {
            case EPIPE://closed
            case ECONNRESET://reset
                fprintf(log_fd,"%d %d c\n",timestamp(), socket);
                break;
            default:
                break;
        }
    } else if (ret == 0) {
        fprintf(log_fd,"%d %d c\n",timestamp(), socket);
    } else {
        fprintf(log_fd,"%d %d r %d\n",timestamp(), socket, (int)ret);
    }
    return ret;
}
