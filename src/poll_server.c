#define _GNU_SOURCE

#include "poll_server.h"
#include "epoll_server.h"
#include "logging.h"
#include "wrapper.h"
#include "common.h"
#include <netinet/ip.h>
#include <sched.h>
#include <poll.h>
#include <limits.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#define LISTENQ 5
#define OPEN_MAX USHRT_MAX//1024    //TODO: change to get max from sysconf (Advanced p. 51)
#define BUFSIZE 4096

static volatile int running = 1;
static void handler (int sig) {
    running = 0;
}

int maxi;


/*
 * Author & Designer: Aing Ragunathan
 * Date: 26-2-2017
 * Function: poll_handler
 * Parameters: void* pass_thread - struct holding the thread number and poll struct
 * Return: void* - unused
 * Notes: worker thread for handling client data
 */
void * poll_handler(void *pass_thread) {
    int nready;
    int sockfd;
    struct pass_poll *pass = (struct pass_poll*) pass_thread;
    struct pollfd *client = pass->client;
    int *thread_num = pass->thread_num;

    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(*thread_num, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while(running) {
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
                } else if(client[i].revents & (POLLIN)) {
                    echo((connection *) con);
                } else if(client[i].revents & (POLLOUT)) {
                    echo_harder((connection *) con);
                }

                free(con);

            }
        }
    }

    free(pass_thread);
    free(client);

    return 0;
}


/*
 * Author & Designer: Aing Ragunathan
 * Date: 26-2-2017
 * Function: poll_server
 * Parameters: const char* port - port number to listen on for clients
 * Return: void - unused
 * Notes: creates a server, accepts new clients and spawns worker threads
 */
void poll_server(const char* port) {
    int listenfd, i, nready, connfd;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    int total_threads = get_nprocs();
    //int total_threads = 1;
    struct pollfd client[OPEN_MAX];

    listenfd = make_bound(port);
    ensure(listen(listenfd, LISTENQ) != -1);

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
        struct pass_poll *pass = malloc(sizeof(struct pass_poll));
        int *thread_num = malloc(sizeof(int));

        *thread_num = i;
        pass->thread_num = thread_num;
        pass->client = client;

        ensure(pthread_attr_init(&attr) == 0);
        ensure(pthread_create(&tid, &attr, &poll_handler, (void *)pass) == 0);
        ensure(pthread_detach(tid) == 0);
    }

    while(running) {
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

                client[i].events = POLLRDNORM | POLLIN;
                if(i > maxi)
                    maxi = i;       //max index
                if(--nready <= 0)
                    continue;       //no more fds
            }
        }
    }
}


