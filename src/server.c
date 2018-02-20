#include "socketwrappers.h"

#define SERV_PORT 8000
#define LISTENQ 5
#define OPEN_MAX USHRT_MAX//255    //TODO: change to get max from sysconf (Advanced p. 51)
#define BUFSIZE 1024



void server() {
    int listenfd, connfd, n;
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    pid_t childpid;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    SetReuse(listenfd);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    Bind(listenfd, &servaddr);
    Listen(listenfd, LISTENQ);  //change to higher number

    //must call waitpid()
#pragma omp parallel
    while(1) {
        clilen = sizeof(cliaddr);

        if((connfd = Accept(listenfd, (struct sockaddr*) &cliaddr, &clilen)) < 0) {
            if(errno == EINTR) {    //restart from interrupted system call
                continue;
            } else {
                puts("accept error");
            }
        }

        printf("connfd: %d\n", connfd);

        if((childpid = fork()) == 0) {
            close(listenfd);

            printf("PID: %d\n", getpid());
            
            while(1) {
                //echo
                if((n = read(connfd, buf, BUFSIZE)) < 0) {
                    if(errno == ECONNRESET) {
                            //connection reset
                            close(connfd);
                    } else if (errno == EAGAIN) {
                        perror("EAGAIN");  
                    } else {
                            perror("reading error");
                    }
                } else {
                        write(connfd, buf, n);
                }
            }
        }

    }
}


