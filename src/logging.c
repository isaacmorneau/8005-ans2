#include "logging.h"
#include <stdio.h>
#include <stdatomic.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>


atomic_int total_clients = 0;

static FILE * log_fd = 0;

/*
 * Author & Designer: Isaac Morneau
 * Date: 26-2-2017
 * Function: init_logging
 * Parameters:
 *      const char * path - the file to log to, 0 for default
 * Return: void
 * Notes: opens loggign file, must be the first logging function called
 */
void init_logging(const char * path) {
    if (path != 0) {
        log_fd = fopen(path, "wb");
    } else {
        log_fd = fopen("logging", "wb");
    }
}

/*
 * Author & Designer: Isaac Morneau
 * Date: 26-2-2017
 * Function: close_logging
 * Parameters: void
 * Return: void
 * Notes: closes loggign file, must be the last logging function called
 */
void close_logging(void) {
    if (log_fd) {
        fclose(log_fd);
        log_fd = 0;
    }
}

/*
 * Author & Designer: Isaac Morneau
 * Date: 26-2-2017
 * Function: timestamp
 * Parameters: void
 * Return: unsigned int - milisecond time of day
 * Notes: gets the current time as a timestamp
 */
unsigned int timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (1e-6*tv.tv_sec) + tv.tv_usec;
}

/*
 * Author & Designer: Isaac Morneau
 * Date: 26-2-2017
 * Function: laccept
 * Notes: identical to the system call accept but logs that a new connection was established
 */
int laccept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len) {
    int ret;
    ret = accept(socket, address, address_len);
    if (ret != -1) {
        fprintf(log_fd,"%d %d o\n",timestamp(), ret);
    }
    return ret;
}

/*
 * Author & Designer: Isaac Morneau
 * Date: 26-2-2017
 * Function: lconnect
 * Notes: identical to the system call connect but logs that a new connection was established
 */
int lconnect(int socket, const struct sockaddr *address, socklen_t address_len) {
    int ret;
    ret = connect(socket, address, address_len);
    if (ret != -1) {
        fprintf(log_fd,"%d %d o\n",timestamp(), socket);
    }
    return ret;
}

/*
 * Author & Designer: Isaac Morneau
 * Date: 26-2-2017
 * Function: lsend
 * Notes: identical to the system call send but logs how much data was sent or
 *      that a connection was closed
 */
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

/*
 * Author & Designer: Isaac Morneau
 * Date: 26-2-2017
 * Function: lrecv
 * Notes: identical to the system call recv but logs how much data was read or
 *      that a connection was closed
 */
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
