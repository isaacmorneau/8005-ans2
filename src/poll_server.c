#include "poll_server.h"
#include "logging.h"
#include "wrapper.h"
#include "common.h"
#include "socketwrappers.h"
#include "poll.h"
#include "limits.h"
#include "omp.h"

#define SERV_PORT 8000
#define LISTENQ 5
#define OPEN_MAX USHRT_MAX//1024    //TODO: change to get max from sysconf (Advanced p. 51)
#define BUFSIZE 1024

void poll_server(const char* port) {
    int listenfd, maxi, i, n, nready, connfd, sockfd;
    struct pollfd client[OPEN_MAX];
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    char buf[BUFSIZE];

    //        listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    //SetReuse(listenfd);
    listenfd = make_bound(port);

    /*        bzero(&servaddr, sizeof(servaddr));
              servaddr.sin_family = AF_INET;
              servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
              servaddr.sin_port = htons(SERV_PORT);
              Bind(listenfd, &servaddr);
              */
    Listen(listenfd, LISTENQ);  //change to higher number

    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    for(i = 1; i < OPEN_MAX; i++) {
        client[i].fd = -1;      //start everything off as available
    }
    maxi = 0;

#pragma omp parallel
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
            }

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

            set_recv_window(connfd);
            client[i].events = POLLRDNORM;
            if(i > maxi)
                maxi = i;       //max index
            if(--nready <= 0)
                continue;       //no more fds
        }

        for(i = 1; i <= maxi; i++) {
            if((sockfd = client[i].fd) < 0) {
                continue;
            }
            if(client[i].revents & (POLLRDNORM | POLLERR)) {
                ensure_nonblock(n = recv(sockfd, buf, BUFSIZE, 0) != -1);
                if(n == 0) {
                    lost_con(connfd);
                }
                ensure_nonblock(send(sockfd, buf, n, 0) != -1);

/*
                if((n = read(sockfd, buf, BUFSIZE)) < 0) {
                    if(errno == ECONNRESET) {
                        //connection reset
                        close(sockfd);
                        client[i].fd = -1;
                        puts("closed connection");
                    } else {
                        perror("reading error");
                    }
                } else if(n == 0) {
                    //connection closed
                    close(sockfd);
                    client[i].fd = -1;
                    puts("closed connection");
                    exit(1);
                } else {
                    write(sockfd, buf, n);
                }
*/
                if(--nready <= 0) {
                    break;  //no more fds
                }
            }
        }
    }
}


