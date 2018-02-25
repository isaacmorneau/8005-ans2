//#include "socketwrappers.h"
#include "logging.h"
#include "wrapper.h"
#include "common.h"
#include "limits.h"
#include <pthread.h>
#include <netinet/ip.h>

#define SERV_PORT 8000
#define LISTENQ 5
#define BUFSIZE 1024
#define MAX_THREADS USHRT_MAX

void *echo_t(void *new_connection) {
    connection *con = ((connection *)new_connection);

    while(1) {
        echo((connection *)con);
    }
}

void server(const char* port) {
    int listenfd, * connfd, client = 0;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    pthread_t threads[MAX_THREADS];
    connection *con;

    listenfd = make_bound(port);
    ensure(listen(listenfd, LISTENQ) != -1);    //TODO Make bigger after testing

    while(client < MAX_THREADS){
        clilen = sizeof(cliaddr);
        connfd = malloc(sizeof(int));
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
//        ensure(con = calloc(1, sizeof(connection)));
        con->sockfd = *connfd;
        con->bytes = 0;
        init_connection(con, *connfd);
        ensure((pthread_create(&threads[client], NULL, echo_t, (void*) con)) == 0);
//        free(con);
    }

    for(int i = 0; i < client; i++) {
        pthread_join(threads[i], NULL);
    }

    return;
}


