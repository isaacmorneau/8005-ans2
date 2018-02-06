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

#define MAXEVENTS 256

void client(const char rconst char * port, const char * data) {
    int efd;
    struct epoll_event event;
    struct epoll_event *events;

    check((efd = epoll_create1(0)) != -1);
    //buffer where events are returned
    events = calloc(MAXEVENTS, sizeof(event));
    //TODO split off gradual increase of client # threads
#pragma omp parallel
    while (1) {
        int n, i;
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP)) {
                // An error has occured on this fd, or the socket is not
                // ready for reading (why were we notified then?)
                perror("epoll_wait, unexpected error or close");
                close_connection(events[i].data.ptr);
                continue;
            } else if (events[i].events & EPOLLDHUP) {//connection closed
                close_connection(events[i].data.ptr);
                continue;
            } else if (events[i].events & EPOLLIN) {//data has been echoed back
                //TODO record read bytes
                black_hole_read((connectio *)events[i].data.ptr);
                continue;
            } else if (events[i].events & EPOLLOUT) {//data can be written
                send_pipe((connectio *)events[i].data.ptr);
                continue;
            }
        }
    }
    free(events);
    close(sfd);
    return 0;
}
