#include "socketwrappers.h"
#include "logging.h"
#include "wrapper.h"
#include "common.h"
#include "limits.h"
#include <pthread.h>

#define SERV_PORT 8000
#define LISTENQ 5
#define BUFSIZE 1024
#define MAX_THREADS USHRT_MAX 

void* echo_t(void* connfd);

void server(const char* port) {
    int listenfd, * connfd, n;
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    pid_t childpid;
    pthread_t threads[MAX_THREADS];

    listenfd = make_bound(port);
    Listen(listenfd, LISTENQ);  //change to higher number
    

    //should call waitpid()
    for(int i = 0; i < MAX_THREADS; i++) {
        clilen = sizeof(cliaddr);
        connfd = malloc(sizeof(int));
        //      if((*connfd = Accept(listenfd, (struct sockaddr*) &cliaddr, &clilen)) < 0) {
        printf("waiting for connection: %d\n");
        if((*connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen)) < 0) {
            if(errno == EINTR) {    //restart from interrupted system call
                continue;
            } else {
                puts("accept error");
            }
        }
    
        set_recv_window(*connfd);
        new_con(*connfd);

        ensure((pthread_create(&threads[i], NULL, echo_t, (void*) connfd)) == 0);
        ensure(pthread_detach(threads[i]) == 0);
        printf("created thread: %d\n", i);
    }
}

void *echo_t(void *fd) {
    int n;
    int connfd = *((int*)fd);
    char buf[BUFSIZE];

    printf("connfd: %d\n", connfd);
    
    while(1) {
        ensure(n = recv(connfd, buf, BUFSIZE, 0) != -1);
        printf("echo: %s\n", buf);
        if(n == 0) {
            lost_con(connfd);
        } else if (n == -1) {
            break;
        }
        ensure(send(connfd, buf, n, 0) != -1);
    }
}




