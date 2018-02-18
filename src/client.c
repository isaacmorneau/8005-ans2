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

void add_client_con(const char * address, const char * port, int epoll_primary_fd, int epoll_fallback_fd) {
    static struct epoll_event event;
    connection * con;

    con = (connection *)calloc(1, sizeof(connection));

    init_connection(con, make_connected(address, port));

    set_non_blocking(con->sockfd);
    //disable rate limiting and TODO check that keep alive stops after connection close
    //enable_keepalive(con->sockfd);
    //set_recv_window(con->sockfd);

    //cant add EPOLLRDHUP as EPOLLEXCLUSIVE would then fail
    //instead check for a read of 0
    event.data.ptr = con;

    //we dont need to calloc the event its coppied.
    event.events = EPOLLIN | EPOLLOUT | EPOLLEXCLUSIVE;
    ensure(epoll_ctl(epoll_fallback_fd, EPOLL_CTL_ADD, con->sockfd, &event) != -1);

    event.events |= EPOLLET;
    ensure(epoll_ctl(epoll_primary_fd, EPOLL_CTL_ADD, con->sockfd, &event) != -1);
}

void client(const char * address,  const char * port, int initial, int rate) {
    int epoll_primary_fd;
    int epoll_fallback_fd;

    struct epoll_event event;
    struct epoll_event *events;

    int scaleback = 1;

    ensure((epoll_primary_fd = epoll_create1(0)) != -1);
    ensure((epoll_fallback_fd = epoll_create1(0)) != -1);
    //buffer where events are returned
    events = calloc(MAXEVENTS, sizeof(event));

    for(int i = 0; i < initial; ++i)
        add_client_con(address, port, epoll_primary_fd, epoll_fallback_fd);

    //TODO split off gradual increase of client # threads
#pragma omp parallel
    while (1) {
        int n, i, bytes;

        n = epoll_wait(epoll_primary_fd, events, MAXEVENTS, scaleback);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) { // error or unexpected close
                perror("epoll_wait");
                close_connection(events[i].data.ptr);
                continue;
            } else {
                if (events[i].events & EPOLLIN) {//data has been echoed back or remote has closed connection
                    bytes = black_hole_read((connection *)events[i].data.ptr);
                }

                if (events[i].events & EPOLLOUT) {//data can be written
                    bytes = send_pipe((connection *)events[i].data.ptr);
                }
            }
        }
        if (n == 0) { //timeout occured, fallback to level triggered just to be safe
            n = epoll_wait(epoll_fallback_fd, events, MAXEVENTS, 0);
            for (i = 0; i < n; i++) {
                if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) { // error or unexpected close
                    perror("epoll_wait");
                    close_connection(events[i].data.ptr);
                    continue;
                } else {
                    if (events[i].events & EPOLLIN) {//data has been echoed back or remote has closed connection
                        bytes = black_hole_read((connection *)events[i].data.ptr);
                    }

                    if (events[i].events & EPOLLOUT) {//data can be written
                        bytes = send_pipe((connection *)events[i].data.ptr);
                    }
                }
            }
            //if there was no event we are just waiting
            //increase wait time to avoid the cycles
            if (n == 0) {
                scaleback *= 2;
            } else {//event did happen and we recovered. return to edge triggered
                scaleback = 1;
            }
        }
    }
    free(events);
    close(epoll_primary_fd);
    close(epoll_fallback_fd);
}
