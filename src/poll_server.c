#include "poll_server.h"
#include "epoll_server.h"
#include "logging.h"
#include "wrapper.h"
#include "common.h"
#include <netinet/ip.h>
#include "poll.h"
#include "limits.h"
#include "omp.h"
#include "pthread.h"
#include "sys/sysinfo.h"

#define LISTENQ 5
#define OPEN_MAX USHRT_MAX//1024    //TODO: change to get max from sysconf (Advanced p. 51)
#define BUFSIZE 4096

int maxi;

void * poll_handler(void *pass_client) {
    int nready;
    int sockfd;
    struct pollfd *client = pass_client;

    while(1) {
        nready = poll(client, maxi + 1, -1);

        for(int i = 1; i <= maxi; i++) {
            if((sockfd = client[i].fd) < 0) {
                continue;
            }

            if(client[i].revents & (POLLRDNORM | POLLERR)) {
                connection *con;

                ensure(con = calloc(1, sizeof(connection)));
                init_connection(con, sockfd);

                if(client[i].revents & (POLLRDNORM | POLLERR)) {
                    echo((connection *) con);
                }

                free(con);

            }
        }
    }
}

void poll_server(const char* port) {
    int listenfd, i, n, nready, connfd, sockfd;
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    //int total_threads = get_nprocs();
    int total_threads = 1;
    struct pollfd client[OPEN_MAX];

    listenfd = make_bound(port);
    ensure(listen(listenfd, LISTENQ) != -1);
    //set_non_blocking(listenfd);

    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    for(i = 1; i < OPEN_MAX; i++) {
        client[i].fd = -1;      //start everything off as available
    }
    maxi = 0;

    //create the worker threads to handle echo
    for(int i = 0; i < total_threads; i++) {
        pthread_attr_t attr;
        pthread_t tid;

        ensure(pthread_attr_init(&attr) == 0);
        ensure(pthread_create(&tid, &attr, &poll_handler, (void *)client) == 0);
        ensure(pthread_detach(tid) == 0);
        printf("thread %d\n", i);
    }

//#pragma omp parallel
    while(1) {
        nready = poll(client, maxi + 1, -1);    //for infinite timeout

        if(client[0].revents & POLLRDNORM) {
            clilen = sizeof(cliaddr);
            connfd = laccept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
            if(connfd == -1) {
                if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    break;
                } else {
                    perror("accept");
                    break;
                }
            } else {
                set_non_blocking(connfd);
                set_recv_window(connfd);

                for(i = 1; i < OPEN_MAX; i++) {
                    if(client[i].fd < 0) {
                        client[i].fd = connfd;  //save fd
                        break;
                    }
                }

                if(i == OPEN_MAX)
                    break;

                client[i].events = POLLRDNORM;
                if(i > maxi)
                    maxi = i;       //max index
                if(--nready <= 0)
                    continue;       //no more fds
            }
        }

        for(i = 1; i <= maxi; i++) {
            connection *con;

            if((sockfd = client[i].fd) < 0) {
                continue;
            }

            ensure(con = calloc(1, sizeof(connection)));
            init_connection(con, sockfd);

            if(client[i].revents & (POLLRDNORM | POLLERR)) {
                echo((connection *) con);
            }

            free(con);
        }

    }
}


