#include "poll_server.h"

#define SERV_PORT 8000
#define LISTENQ 5
#define OPEN_MAX 255    //TODO: change to get max from sysconf (Advanced p. 51)
#define BUFSIZE 1024

void poll_server() {
        int listenfd, maxi, i, n, nready, connfd, sockfd;
        struct pollfd client[OPEN_MAX];
        struct sockaddr_in cliaddr, servaddr;
        socklen_t clilen;
        char buf[BUFSIZE];

        listenfd = Socket(AF_INET, SOCK_STREAM, 0);
        SetReuse(listenfd);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(SERV_PORT);
        Bind(listenfd, &servaddr);
        Listen(listenfd, LISTENQ);  //change to higher number

        client[0].fd = listenfd;
        client[0].events = POLLRDNORM;
        for(i = 1; i < OPEN_MAX; i++) {
                client[i].fd = -1;      //start everything off as available
        }
        maxi = 0;

        while(1) {
                nready = poll(client, maxi + 1, -1);    //for infinite timeout

                if(client[0].revents & POLLRDNORM) {
                        clilen = sizeof(cliaddr);
                        connfd = Accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
                        for(i = 1; i < OPEN_MAX; i++) {
                                if(client[i].fd < 0) {
                                        client[i].fd = connfd;  //save fd
                                        break;
                                }        
                        }

                        if(i == OPEN_MAX)
                                return;
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
                                if((n = read(sockfd, buf, BUFSIZE)) < 0) {
                                        if(errno == ECONNRESET) {
                                                //connection reset
                                                close(sockfd);
                                                client[i].fd = -1;
                                        } else {
                                                perror("reading error");
                                        }
                                } else if(n == 0) {
                                        //connection closed
                                        close(sockfd);
                                        client[i].fd = -1;
                                } else {
                                        write(sockfd, buf, n);
                                }

                                if(--nready <= 0) {
                                        break;  //no more fds 
                                }
                        }
                }
        }
}


