#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/uio.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "wrapper.h"
#include "common.h"

void enable_keepalive(int sockfd) {
    int yes = 1;
    int idle = 1;
    int interval = 1;
    int maxpkt = 10;
    check(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) != -1);
    check(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) != -1);
    check(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int)) != -1);
    check(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int)) != -1);
}

void set_recv_window(int sockfd) {
    int rcvbuf = TCP_WINDOW_CAP;
    int clamp = TCP_WINDOW_CAP/2;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuf, sizeof(rcvbuf));
    setsockopt(sockfd, SOL_SOCKET, TCP_WINDOW_CLAMP, (char *)&clamp, sizeof(clamp));
}


void init_connection(connection * con, int sockfd) {
    con->pipe_bytes = 0;
    con->sockfd = sockfd;
    check(pipe(con->out_pipe) != -1);
}

void close_connection(connection * con) {
    close(con->out_pipe[0]);
    close(con->out_pipe[1]);
    if (con->sockfd != -1) {
        close(con->sockfd);
    }
}

void make_non_blocking (int sfd) {
    int flags;
    check((flags = fcntl(sfd, F_GETFL, 0)) != -1);
    flags |= O_NONBLOCK;
    check(fcntl(sfd, F_SETFL, flags) != -1);
}

int make_connected(const char * address, const char * port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     // Return IPv4 and IPv6 choices
    hints.ai_socktype = SOCK_STREAM; // We want a TCP socket
    hints.ai_flags = AI_PASSIVE;     // All interfaces
    check(getaddrinfo(address, port, &hints, &result) != 0);
    for (rp = result; rp != 0; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;
        s = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0) {
            break;
        }
        close(sfd);
    }
    check(!rp);
    freeaddrinfo(result);
    return sfd;
}

int make_bound(const char * port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd;
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     // Return IPv4 and IPv6 choices
    hints.ai_socktype = SOCK_STREAM; // We want a TCP socket
    hints.ai_flags = AI_PASSIVE;     // All interfaces
    check(getaddrinfo(0, port, &hints, &result) != 0);
    for (rp = result; rp != 0; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
            continue;
        }
        int enable = 1;
        check(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1);
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen)) {
            //we managed to bind successfully
            break;
        }
        close(sfd);
    }
    check(!rp);
    freeaddrinfo(result);
    return sfd;
}

static char message[TCP_WINDOW_CAP];
int send_pipe(connection * con) {
    if(con->pipe_bytes == 0) {//fill it up each time its empty, might be faster to just top it up TODO check later
        con->pipe_bytes = fill_pipe(con, message, TCP_WINDOW_CAP);
    }
    int ret;
    while (1) {
        check(ret = splice(con->out_pipe[0], 0, con->sockfd, 0, TCP_WINDOW_CAP, SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK) != -1
                || errno != EAGAIN);
        if (ret == 0) {
            break;
        }
        printf("wrote: %d\n", ret);
    }
    return 0;
}
int fill_pipe(connection * con, const char * buf, size_t len) {
    struct iovec iv;
    iv.iov_base = (void*)buf;
    iv.iov_len = len;
    int wrote = 0;
    //move does nothing to vmsplice and we cant gift unless we want to keep allocating the message
    //instead just copy it to skip the mallocs
    check((wrote = vmsplice(con->out_pipe[1], &iv, 1, SPLICE_F_NONBLOCK)) != -1);
    return wrote;
}
//multithreaded garbage write, never read from this
static char blackhole[TCP_WINDOW_CAP];
int black_hole_read(connection * con) {
    int read_bytes = 0, tmp;
    //spinlock on emptying the response
    while ((tmp = read(con->sockfd, blackhole, TCP_WINDOW_CAP)) != -1) read_bytes += tmp;
    return read_bytes;
}
