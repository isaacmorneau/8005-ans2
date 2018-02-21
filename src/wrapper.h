#ifndef WRAPPER_H
#define WRAPPER_H

#include <stdlib.h>
#include "common.h"
#define MAXEVENTS 256

#ifdef __cplusplus
extern "C" {
#endif

typedef struct connection {
    int bytes;
    int sockfd;
    char buffer[TCP_WINDOW_CAP];
} connection;

#define close_connection(con) do {\
    close(((connection*)con)->sockfd);\
    } while(0)

#define init_connection(con, fd) do {\
    ((connection*)con)->bytes = 0;\
    ((connection*)con)->sockfd = fd;\
    } while(0)

void set_fd_limit();
void set_non_blocking(int sfd);
void set_recv_window(int sockfd);
void enable_keepalive(int sockfd);
int make_bound(const char * port);
int make_connected(const char * address, const char * port);

int white_hole_write(connection * con);
int black_hole_read(connection * con);

int echo(connection * con);
int echo_harder(connection * con);

#ifdef __cplusplus
}
#endif

#endif
