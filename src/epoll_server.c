#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <omp.h>

#include "wrapper.h"
#include "common.h"
#include "epoll_server.h"

void epoll_server(const char * port) {
    int sfd;

    int epoll_primary_fd;
    int epoll_fallback_fd;

    struct epoll_event event;
    struct epoll_event *events;
    struct epoll_event *events_fallback;

    connection * con;

    int scaleback = 0;
    int total_clients = 0;

    //make and bind the socket
    sfd = make_bound(port);
    set_non_blocking(sfd);

    ensure(listen(sfd, SOMAXCONN) != -1);

    ensure((epoll_primary_fd = epoll_create1(0)) != -1);
    ensure((epoll_fallback_fd = epoll_create1(0)) != -1);

    con = (connection *)calloc(1, sizeof(connection));
    init_connection(con, sfd);
    event.data.ptr = con;

    event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLEXCLUSIVE;
    ensure(epoll_ctl(epoll_primary_fd, EPOLL_CTL_ADD, sfd, &event) != -1);

    // Buffer where events are returned (no more that 64 at the same time)
    events = calloc(MAXEVENTS, sizeof(event));
    events_fallback = calloc(MAXEVENTS, sizeof(event));

#pragma omp parallel
    while (1) {
        int n, i;
        printf("current scale: %d\n",scaleback);
        n = epoll_wait(epoll_primary_fd, events, MAXEVENTS, scaleback);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
                // A socket got closed
                --total_clients;
                printf("Client lost, closing fd %d\n", ((connection*)events[i].data.ptr)->sockfd);
                close_connection(events[i].data.ptr);
                continue;
            } else {
                if((events[i].events & EPOLLIN)) {
                    puts("EPOLLIN");
                    if (sfd == ((connection*)events[i].data.ptr)->sockfd) {
                        // We have a notification on the listening socket, which
                        // means one or more incoming connections.
                        while (1) {
                            struct sockaddr in_addr;
                            socklen_t in_len;
                            int infd;
                            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                            in_len = sizeof(in_addr);

                            infd = accept(sfd, &in_addr, &in_len);
                            if (infd == -1) {
                                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                                    break;
                                } else {
                                    perror("accept");
                                    break;
                                }
                            }

                            ensure(getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf,
                                        NI_NUMERICHOST | NI_NUMERICSERV) == 0);
                            ++total_clients;
                            printf("Accepted connection %d on descriptor %d "
                                    "(host=%s, port=%s)\n", total_clients, infd, hbuf, sbuf);

                            // Make the incoming socket non-blocking and add it to the
                            // list of fds to monitor.
                            set_non_blocking(infd);
                            enable_keepalive(infd);
                            set_recv_window(infd);

                            ensure(con = calloc(1, sizeof(connection)));

                            init_connection(con, infd);

                            event.data.ptr = con;

                            event.events = EPOLLIN | EPOLLOUT | EPOLLEXCLUSIVE;
                            ensure(epoll_ctl(epoll_fallback_fd, EPOLL_CTL_ADD, infd, &event) != -1);

                            event.events |= EPOLLET;
                            ensure(epoll_ctl(epoll_primary_fd, EPOLL_CTL_ADD, infd, &event) != -1);

                        }
                        continue;
                    } else {
                        //regular incomming message echo it back
                        echo((connection *)event.data.ptr);
                    }
                }

                if((events[i].events & EPOLLOUT)) {
                    puts("EPOLLOUT");
                    //we are now notified that we can send the rest of the data
                    echo_harder((connection *)event.data.ptr);
                }
            }
        }
        if (n == 0) { //timeout occured, fallback to level triggered just to be safe
            n = epoll_wait(epoll_fallback_fd, events, MAXEVENTS, 0);
            for (i = 0; i < n; i++) {
                if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
                    // A socket got closed
                    --total_clients;
                    printf("Client lost, closing fd %d\n", ((connection*)events[i].data.ptr)->sockfd);
                    close_connection(events[i].data.ptr);
                    continue;
                } else {
                    if((events[i].events & EPOLLIN)) {
                        puts("EPOLLIN2");
                        //regular incomming message echo it back
                        echo((connection *)event.data.ptr);
                    }

                    if((events[i].events & EPOLLOUT)) {
                        puts("EPOLLOUT2");
                        //we are now notified that we can send the rest of the data
                        echo_harder((connection *)event.data.ptr);
                    }
                }
            }
            //if there was no event we are just waiting
            //increase wait time to avoid doing a mega core pin
            if (n == 0) {
                scaleback = scaleback? scaleback * 2: 1;
            } else {//event did happen and we recovered. return to edge triggered
                puts("recovered\n");
                scaleback = 0;
            }
        } else {
            scaleback = 0;
        }
    }
    free(events);
    close(sfd);
    close(epoll_primary_fd);
    close(epoll_fallback_fd);
}
