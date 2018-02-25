#include "poll_server.h"
#include "logging.h"
#include "wrapper.h"
#include "common.h"
#include "socketwrappers.h"
#include "poll.h"
#include "limits.h"
#include "omp.h"

#define LISTENQ 5
#define OPEN_MAX USHRT_MAX//1024    //TODO: change to get max from sysconf (Advanced p. 51)
#define BUFSIZE 4096




void poll_server(const char* port) {
    int listenfd, maxi, i, n, nready, connfd, sockfd;
    struct pollfd client[OPEN_MAX];
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    connection *con;

    listenfd = make_bound(port);
    ensure(listen(listenfd, LISTENQ) != -1);
//    set_non_blocking(listenfd); 

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
                //new_con(connfd);
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
            if((sockfd = client[i].fd) < 0) {
                continue;
            }

            ensure(con = calloc(1, sizeof(connection)));
            init_connection(con, sockfd);

            if(client[i].revents & (POLLRDNORM | POLLERR)) {
//                ensure_nonblock(n = lrecv(sockfd, buf, BUFSIZE, 0) != -1);
//               ensure_nonblock(lsend(sockfd, buf, n, 0) != -1);

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
                echo((connection *) con);
/*
                ensure_nonblock(n = recv(sockfd, buf, BUFSIZE, 0) != -1);
                if(n == 0) {
                    lost_con(connfd);
                }
                printf("echo: %d\n", n);
                ensure_nonblock(n = send(sockfd, buf, n, 0) != -1);
                printf("echo: %d\n", n);
                if(--nready <= 0) {
                    break;  //no more fds
                }
*/
            }
            free(con);
        }
    }
}


