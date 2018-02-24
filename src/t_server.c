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

void *echo_t(void *new_connection) {
    connection *con = ((connection *)new_connection);
    int n;

    while(1) {
        echo((connection *)con);
    }

}

void server(const char* port) {
    int listenfd, * connfd, client;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    pid_t childpid;
    pthread_t threads[MAX_THREADS];
    connection *con;

    listenfd = make_bound(port);
    ensure(listen(listenfd, LISTENQ) != -1);    //TODO Make bigger after testing

    while(1){
        clilen = sizeof(cliaddr);
        connfd = malloc(sizeof(int));
        //      if((*connfd = Accept(listenfd, (struct sockaddr*) &cliaddr, &clilen)) < 0) {
        if((*connfd = laccept(listenfd, (struct sockaddr*) &cliaddr, &clilen)) < 0) {
            if(errno == EINTR) {    //restart from interrupted system call
                continue;
            } else {
                perror("accept");
                break;
            }
        }

        client++;    //new client connection
        set_non_blocking(*connfd);
        set_recv_window(*connfd);
        ///new_con(*connfd);
        ensure(con = calloc(1, sizeof(connection)));
        init_connection(con, *connfd);
        ensure((pthread_create(&threads[client], NULL, echo_t, (void*) con)) == 0);
        ensure(pthread_detach(threads[client]) == 0);
        printf("created thread: %d\n", client);

        if(client == MAX_THREADS) {
            printf("MAX_THREADS reached: %d\n", MAX_THREADS);

            //wait until client closes connections
            while(1){}  //uhhh maybe do something more intelligent here
        }
    }
    return;
}


