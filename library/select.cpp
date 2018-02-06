#include "select.h"

#define BUFSIZE 1024

using namespace std;

void select_server() {
        int listenfd, maxfd, sockfd, maxi, connfd, nready; 
        int i, n, client[FD_SETSIZE];
        struct sockaddr_in servaddr, cliaddr;
        socklen_t clilen;
        fd_set rset, allset;
        char buf[BUFSIZE];

        listenfd = Socket(AF_INET, SOCK_STREAM, 0);
        SetReuse(listenfd);
        bzero(&servaddr, sizeof(struct sockaddr_in));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(LISTEN_PORT);
        Bind(listenfd, &servaddr);
        Listen(listenfd, LISTENQ);          

        maxfd = listenfd;   
        maxi = -1;         

        for(i = 0; i < FD_SETSIZE; i++) {
                client[i] = -1;
        }

        FD_ZERO(&allset);
        FD_SET(listenfd, &allset);

        //main loop for incoming data
        while(1) {
                rset = allset;
                nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
                
                //check for new connection request
                if(FD_ISSET(listenfd, &rset)) {
                        clilen = sizeof(cliaddr);
                        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
   
                        //find first open spot
                        for(i = 0; i < FD_SETSIZE; i++) {
                                if(client[i] < 0) {
                                        client[i] = connfd; //save the fd 
                                        break;
                                }
                        }

                        //check if the client list is already ful
                        if(i == FD_SETSIZE) {
                                puts("Too many clients");
                                exit(1);
                        }

                        FD_SET(connfd, &allset);    //add new fd to the select set
                        
                        if(connfd > maxfd) {
                                maxfd = connfd; //update max if needed
                        }

                        if(i > maxi) {
                                maxi = i;   //update max index in client array
                        }

                        //add client details to records

                        if(--nready <= 0){
                                continue;   //nothing else to check
                        }
                }

                //check clients for data
                for(i = 0; i <= maxi; i++) {
                        //check if fd is set
                        if((sockfd = client[i]) < 0) {
                                continue;
                        }

                        if(FD_ISSET(sockfd, &rset)) {
                                if((n = read(sockfd, buf, BUFSIZE)) == 0) {
                                        //connection was closed
                                        close(sockfd);
                                        FD_CLR(sockfd, &allset);
                                        client[i] = -1;
                                } else {
                                        write(sockfd, buf, n);  //echo
                                        //log results
                                }

                                if(--nready <= 0) {
                                        break;  //nothing else to check
                                }
                        }
                }
        }
}


