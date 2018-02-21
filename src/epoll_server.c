#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>

#include "wrapper.h"
#include "common.h"
#include "logging.h"
#include "epoll_server.h"

static volatile int running = 1;
static void handler(int sig) {
    running = 0;
}

void * epoll_handler(void * efd_ptr) {
    int efd = *((int *)efd_ptr);
    struct epoll_event event;
    struct epoll_event *events;

    // Buffer where events are returned (no more that 64 at the same time)
    events = calloc(MAXEVENTS, sizeof(event));

    while (running) {
        int n, i;
        //printf("current scale: %d\n",scaleback);
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
                // A socket got closed
                lost_con(((connection*)events[i].data.ptr)->sockfd);
                close_connection(events[i].data.ptr);
                continue;
            } else {
                if((events[i].events & EPOLLIN)) {
                    //regular incomming message echo it back
                    echo((connection *)event.data.ptr);
                }

                if((events[i].events & EPOLLOUT)) {
                    //we are now notified that we can send the rest of the data
                    //possible but unlikely, handle it anyway
                    echo_harder((connection *)event.data.ptr);
                }
            }
        }
    }
    free(events);

    return 0;
}

void epoll_server(const char * port) {
    int sfd;
    int total_threads = get_nprocs();
    int * epollfds = calloc(total_threads, sizeof(int));
    int efd;
    connection * con;
    struct epoll_event event;
    struct epoll_event *events;

    signal(SIGINT, handler);

    //make the epolls for the threads
    //then pass them to each of the threads
    for (int i = 0; i < total_threads; ++total_threads) {
        ensure((epollfds[i] = epoll_create1(0)) != -1);

        pthread_attr_t attr;
        pthread_t tid;

        ensure(pthread_attr_init(&attr) == 0);
        ensure(pthread_create(&tid, &attr, &epoll_handler, &epollfds[i]) == 0);
        ensure(pthread_attr_destroy(&attr) == 0);
        ensure(pthread_detach(tid) == 0);//be free!!
    }

    //make and bind the socket
    sfd = make_bound(port);
    set_non_blocking(sfd);

    //listening epoll
    ensure((efd = epoll_create1(0)) != -1);

    con = (connection *)calloc(1, sizeof(connection));
    init_connection(con, sfd);
    event.data.ptr = con;

    event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    ensure(epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) != -1);

    // Buffer where events are returned (no more that 64 at the same time)
    events = calloc(MAXEVENTS, sizeof(event));

    int epoll_pos = 0;
    //threads will handle the clients, the main thread will just add new ones
    while (running) {
        int n, i;
        //printf("current scale: %d\n",scaleback);
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
                // A socket got closed
                lost_con(((connection*)events[i].data.ptr)->sockfd);
                close_connection(events[i].data.ptr);
            } else { //EPOLLIN
                if (sfd == ((connection*)events[i].data.ptr)->sockfd) {
                    // We have a notification on the listening socket, which
                    // means one or more incoming connections.
                    while (1) {
                        struct sockaddr in_addr;
                        socklen_t in_len;
                        int infd;

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

                        // Make the incoming socket non-blocking and add it to the
                        // list of fds to monitor.
                        set_non_blocking(infd);
                        //enable_keepalive(infd);
                        set_recv_window(infd);
                        new_con(infd);

                        ensure(con = calloc(1, sizeof(connection)));
                        init_connection(con, infd);

                        event.data.ptr = con;

                        event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLEXCLUSIVE;
                        ensure(epoll_ctl(epollfds[epoll_pos], EPOLL_CTL_ADD, infd, &event) != -1);
                        //round robin client addition
                        epoll_pos = epoll_pos == total_threads ? 0 : epoll_pos + 1;
                    }
                }
            }
        }
    }

    //cleanup all fds and memory
    for (int i = 0; i < total_threads; ++total_threads) {
        close(epollfds[i]);
    }
    close(efd);
    close(sfd);

    free(epollfds);
    free(events);
    free(con);
}

