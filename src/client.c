#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sysinfo.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "client.h"
#include "common.h"
#include "logging.h"
#include "wrapper.h"

static pthread_cond_t * thread_cvs;
static pthread_mutex_t * thread_mts;
int * epollfds;

static volatile int running = 1;
static void handler(int sig) {
    running = 0;
}

void * client_handler(void * pass_pos) {
    int pos = (int)pass_pos;
    int efd = epollfds[pos];
    struct epoll_event *events;
    int bytes;
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(pos, &cpuset);

    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    // Buffer where events are returned
    events = calloc(MAXEVENTS, sizeof(struct epoll_event));

    pthread_cond_wait(&thread_cvs[pos], &thread_mts[pos]);

    while (running) {
        int n, i;
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) { // error or unexpected close
                lost_con(((connection*)events[i].data.ptr)->sockfd);
                close_connection(events[i].data.ptr);
                continue;
            } else {
                if (events[i].events & EPOLLIN) {//data has been echoed back or remote has closed connection
                    //puts("EPOLLIN");
                    bytes = black_hole_read((connection *)events[i].data.ptr);
                }

                if (events[i].events & EPOLLOUT) {//data can be written
                    //puts("EPOLLOUT");
                    bytes = white_hole_write((connection *)events[i].data.ptr);
                }
            }
        }
    }
    free(events);

    return 0;
}

void client(const char * address, const char * port, int rate, bool max) {
    int total_threads = get_nprocs();
    epollfds = calloc(total_threads, sizeof(int));
    thread_cvs = calloc(total_threads, sizeof(pthread_cond_t));
    thread_mts = calloc(total_threads, sizeof(pthread_mutex_t));
    connection * con;
    struct epoll_event event;
    int epoll_pos = 0;

    //signal(SIGINT, handler);

    //make the epolls for the threads
    //then pass them to each of the threads
    for (int i = 0; i < total_threads; ++i) {
        ensure((epollfds[i] = epoll_create1(0)) != -1);
        pthread_cond_init(&thread_cvs[i], NULL);
        pthread_mutex_init(&thread_mts[i], NULL);

        pthread_attr_t attr;
        pthread_t tid;

        ensure(pthread_attr_init(&attr) == 0);
        ensure(pthread_create(&tid, &attr, &client_handler, (void*)i) == 0);
        ensure(pthread_attr_destroy(&attr) == 0);
        ensure(pthread_detach(tid) == 0);//be free!!
        printf("thread %d on epoll fd %d\n", i, epollfds[i]);
    }

    if (rate) {
        while (running) {
            usleep(rate);
            con = (connection *)malloc(sizeof(connection));
            init_connection(con, make_connected(address, port));

            set_non_blocking(con->sockfd);
            //disable rate limiting and TODO check that keep alive stops after connection close
            //enable_keepalive(con->sockfd);
            set_recv_window(con->sockfd);
            new_con(con->sockfd);

            //cant add EPOLLRDHUP as EPOLLEXCLUSIVE would then fail
            //instead check for a read of 0
            event.data.ptr = con;

            //round robin client addition
            event.events = EPOLLET | EPOLLEXCLUSIVE | (!max||epoll_pos<total_threads?EPOLLIN | EPOLLOUT:0);
            ensure(epoll_ctl(epollfds[epoll_pos % total_threads], EPOLL_CTL_ADD, con->sockfd, &event) != -1);
            if (epoll_pos < total_threads) {
                pthread_cond_signal(&thread_cvs[epoll_pos]);
            }
            ++epoll_pos;
        }
    }
}
