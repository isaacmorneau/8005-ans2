#include "poll_server.h"
#include "epoll_server.h"
#include "logging.h"
#include "wrapper.h"
#include "common.h"
#include "socketwrappers.h"
#include "poll.h"
#include "limits.h"
#include "omp.h"
#include "sys/sysinfo.h"

#define LISTENQ 5
#define OPEN_MAX USHRT_MAX//1024    //TODO: change to get max from sysconf (Advanced p. 51)
#define BUFSIZE 4096


void poll_server(const char* port) {
    int listenfd, maxi, i, n, nready, connfd, sockfd;
    struct pollfd client[OPEN_MAX];
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    int total_threads = get_nprocs();

    for(int i = 0; i < total_threads; i++) {
        pthread_attr_t attr;
        pthread_tid

        ensure(pthread_attr_init(&attr) == 0);
        ensure(pthread_create(&tid, &attr, &poll_handler, (void *)i) == 0);
        ensure:
    }



    listenfd = make_bound(port);
    ensure(listen(listenfd, LISTENQ) != -1);
    set_non_blocking(listenfd); 

    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    for(i = 1; i < OPEN_MAX; i++) {
        client[i].fd = -1;      //start everything off as available
    }
    maxi = 0;

//#pragma omp parallel
    while(1) {
        nready = poll(client, maxi + 1, -1);    //for infinite timeout

        if(client[0].revents & POLLRDNORM) {
            clilen = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
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
                new_con(connfd);
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
/*
        for(i = 1; i <= maxi; i++) {
//            connection *con;

            if((sockfd = client[i].fd) < 0) {
                continue;
            }
            
//            ensure(con = calloc(1, sizeof(connection)));
//            init_connection(con, sockfd);

            if(client[i].revents & (POLLRDNORM | POLLERR)) {
                int ret;
                int total = 0;

                while (total < 4096) {
                    //ensure_nonblock((ret = recv(sockfd, buf, TCP_WINDOW_CAP, 0)) != -1);
                    ret = recv(sockfd, buf, TCP_WINDOW_CAP, 0);
                    if (ret == -1) {
                        break;
                    } else if (ret == 0) {//actually means the connection was closed
                        close(sockfd);
                        lost_con(sockfd);
                    }
                    total += ret;
                }

                if(ret == -1) {
                    continue;
                }
                
                printf("echo recv: %d\n", TCP_WINDOW_CAP);

                while (total > 0) {
                        ensure_nonblock((ret = send(sockfd, buf + (TCP_WINDOW_CAP - total), total, 0)) != -1);
                        if (ret == -1) 
                            break;
                        total -= ret;
                }
                printf("echo sent: %d\n", ret);
            }
        }
*/
    }
}


