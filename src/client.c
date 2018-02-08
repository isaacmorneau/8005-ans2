#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>

#include "client.h"
#include "common.h"
#include "wrapper.h"

#define MAXEVENTS 256

void add_client_con(const char * address, const char * port, int efd) {
    static struct epoll_event event;
    connection * con;

    con = (connection *)calloc(1, sizeof(connection));

    init_connection(con, make_connected(address, port));
    //cant add EPOLLRDHUP as EPOLLEXCLUSIVE would then fail
    //instead check for a read of 0
    event.events = EPOLLIN | EPOLLOUT | EPOLLEXCLUSIVE | EPOLLET;
    event.data.ptr = con;
    //we dont need to calloc the event its coppied.
    check(epoll_ctl(efd, EPOLL_CTL_ADD, con->sockfd, &event) != -1);
}

void client(const char * address,  const char * port, int initial, int rate) {
    int efd;
    struct epoll_event event;
    struct epoll_event *events;

    check((efd = epoll_create1(0)) != -1);
    //buffer where events are returned
    events = calloc(MAXEVENTS, sizeof(event));
    for(int i = 0; i < initial; ++i)
        add_client_con(address, port, efd);
    //TODO split off gradual increase of client # threads
#pragma omp parallel
    while (1) {
        int n, i;
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP)) { // error or unexpected close
                perror("epoll_wait");
                close_connection(events[i].data.ptr);
                continue;
            } else if (events[i].events & EPOLLIN) {//data has been echoed back or remote has closed connection
                //TODO record read bytes
                int bytes = black_hole_read((connection *)events[i].data.ptr);
                if (bytes == 0 && (errno != EAGAIN || errno != EWOULDBLOCK))
                    close_connection(events[i].data.ptr);
                continue;
            } else if (events[i].events & EPOLLOUT) {//data can be written
                send_pipe((connection *)events[i].data.ptr);
                continue;
            }
        }
    }
    free(events);
    close(efd);
}
