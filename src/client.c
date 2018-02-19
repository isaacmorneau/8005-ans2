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

const char ** gaddress;
const char ** gport;

int epoll_primary_fd;
int epoll_fallback_fd;

void add_client_con() {
    static struct epoll_event event;
    connection * con;

    con = (connection *)calloc(1, sizeof(connection));

    init_connection(con, make_connected(*gaddress, *gport));

    set_non_blocking(con->sockfd);
    //disable rate limiting and TODO check that keep alive stops after connection close
    enable_keepalive(con->sockfd);
    set_recv_window(con->sockfd);

    //cant add EPOLLRDHUP as EPOLLEXCLUSIVE would then fail
    //instead check for a read of 0
    event.data.ptr = con;

    //we dont need to calloc the event its coppied.
    event.events = EPOLLIN | EPOLLOUT | EPOLLEXCLUSIVE;
    ensure(epoll_ctl(epoll_fallback_fd, EPOLL_CTL_ADD, con->sockfd, &event) != -1);

    event.events |= EPOLLET;
    ensure(epoll_ctl(epoll_primary_fd, EPOLL_CTL_ADD, con->sockfd, &event) != -1);
}

int total_clients = 0;
void * client_increase(void * rate_ptr) {
    int rate = *((int*)rate_ptr);
    while (1) {
        usleep(rate);
        add_client_con();
        ++total_clients;
        printf("total_clients: %d\n", total_clients);
    }
}

void client(const char * address,  const char * port, int initial, int rate) {
    gaddress = &address;
    gport = &port;

    struct epoll_event event;
    struct epoll_event *events;

    int scaleback = 0;

    ensure((epoll_primary_fd = epoll_create1(0)) != -1);
    ensure((epoll_fallback_fd = epoll_create1(0)) != -1);

    //buffer where events are returned
    events = calloc(MAXEVENTS, sizeof(event));

#pragma omp parallel for
    for(int i = 0; i < initial; ++i) {
        add_client_con();
    }

    //if the rate is zero dont bother with the threads
    if (rate) {
        pthread_attr_t attr;
        pthread_t tid;

        total_clients = initial;

        ensure(pthread_attr_init(&attr) == 0);
        ensure(pthread_create(&tid, &attr, &client_increase, &rate) == 0);
        ensure(pthread_attr_destroy(&attr) == 0);
        ensure(pthread_detach(tid) == 0);
    }

#pragma omp parallel
    for (;;) {
        int n, i, bytes;

        //printf("current scale: %d\n",scaleback);
        n = epoll_wait(epoll_primary_fd, events, MAXEVENTS, scaleback);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) { // error or unexpected close
                --total_clients;
                printf("Client lost, closing fd %d\n", ((connection*)events[i].data.ptr)->sockfd);
                close_connection(events[i].data.ptr);
                continue;
            } else {
                if (events[i].events & EPOLLIN) {//data has been echoed back or remote has closed connection
                    bytes = black_hole_read((connection *)events[i].data.ptr);
                }

                if (events[i].events & EPOLLOUT) {//data can be written
                    bytes = white_hole_write((connection *)events[i].data.ptr);
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
                        bytes = white_hole_write((connection *)events[i].data.ptr);
                    }
                }
            }
            //if there was no event we are just waiting
            //increase wait time to avoid the cycles
            if (n == 0) {
                scaleback = scaleback ? scaleback * 2: 1;
            } else {//event did happen and we recovered. return to edge triggered
                //puts("recovered\n");
                scaleback = 0;
            }
        } else {
            scaleback = 0;
        }
    }
    free(events);
    close(epoll_primary_fd);
    close(epoll_fallback_fd);
}
