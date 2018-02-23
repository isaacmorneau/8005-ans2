#include "logging.h"
#include <stdio.h>
#include <stdatomic.h>
#include <errno.h>
#include <sys/socket.h>

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

void new_con(int fd) {
    printf("++tc:%d fd:%d\n", ++total_clients, fd);
}

void lost_con(int fd) {
    printf("--tc:%d fd:%d\n", --total_clients, fd);
}

ssize_t lsend(int socket, const void *buffer, size_t length, int flags) {
    ssize_t ret;
    ret = send(socket, buffer, length, flags);
    if (ret == -1) {
        switch (errno) {
            case EPIPE://closed
            case ECONNRESET://reset
                //TODO connection closed
                break;
            case EINTR://interrupted
            case EAGAIN://nothing more
            default:
                break;
        }
    } else if (ret == 0) {
        //TODO connection closed
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
                //TODO connection closed
                break;
            case EINTR://interrupted
            case EAGAIN://nothing more
            default:
                break;
        }
    } else if (ret == 0) {
        //TODO connection closed
    }
    return ret;
}
