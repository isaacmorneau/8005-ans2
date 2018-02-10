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
    ensure(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) != -1);
    ensure(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) != -1);
    ensure(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int)) != -1);
    ensure(setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int)) != -1);
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
    ensure(pipe(con->out_pipe) != -1);
}

void close_connection(connection * con) {
    close(con->out_pipe[0]);
    close(con->out_pipe[1]);
    if (con->sockfd != -1) {
        close(con->sockfd);
    }
}

void set_non_blocking (int sfd) {
    int flags;
    ensure((flags = fcntl(sfd, F_GETFL, 0)) != -1);
    flags |= O_NONBLOCK;
    ensure(fcntl(sfd, F_SETFL, flags) != -1);
}

int make_connected(const char * address, const char * port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     // Return IPv4 and IPv6 choices
    hints.ai_socktype = SOCK_STREAM; // We want a TCP socket
    hints.ai_flags = AI_PASSIVE;     // All interfaces
    ensure(getaddrinfo(address, port, &hints, &result) == 0);
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
    ensure(rp);
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
    ensure(getaddrinfo(0, port, &hints, &result) == 0);
    for (rp = result; rp != 0; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
            continue;
        }
        int enable = 1;
        ensure(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1);
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen)) {
            //we managed to bind successfully
            break;
        }
        close(sfd);
    }
    ensure(rp);
    freeaddrinfo(result);
    return sfd;
}

static char message[TCP_WINDOW_CAP];
int send_pipe(connection * con) {
    fill_pipe(con, message, TCP_WINDOW_CAP);
    int ret;
    int total = 0;
    while (1) {
        if ((ret = splice(con->out_pipe[0], 0, con->sockfd, 0, TCP_WINDOW_CAP, SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK)) == -1) {
            if (errno == EAGAIN) {
                break;
            } else {
                perror("splice");
                exit(1);
            }
        }
        if (ret == 0) {
            fill_pipe(con, message, TCP_WINDOW_CAP);
        }
        total += ret;
    }
    return total;
}
void fill_pipe(connection * con, const char * buf, size_t len) {
    struct iovec iv;
    iv.iov_base = (void*)buf;
    iv.iov_len = len;
    //move does nothing to vmsplice and we cant gift unless we want to keep allocating the message
    //instead just copy it to skip the mallocs
    ensure(vmsplice(con->out_pipe[1], &iv, 1, SPLICE_F_NONBLOCK) != -1);
}
//multithreaded garbage write, never read from this
static char blackhole[TCP_WINDOW_CAP];
int black_hole_read(connection * con) {
    int read_bytes = 0, tmp;
    //spinlock on emptying the response
    do {
        tmp = read(con->sockfd, blackhole, TCP_WINDOW_CAP);
        if (tmp == -1) {
            if (errno == EAGAIN) {
                break;
            } else {
                perror("read");
                exit(1);
            }
        } else if (tmp == 0) {
            close_connection(con);
            break;
        }
        read_bytes += tmp;
    } while (1);
    return read_bytes;
}
