#ifndef WRAPPER_H
#define WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct connection {
    int out_pipe[2];
    int pipe_bytes;
    int sockfd;
} connection;

void init_connection(connection * con, int sockfd);
void close_connection(connection * con);

void set_non_blocking(int sfd);
void set_recv_window(int sockfd);
void enable_keepalive(int sockfd);
int make_bound(const char * port);
int make_connected(const char * address, const char * port);

int send_pipe(connection * con);
int fill_pipe(connection * con, const char * buff, size_t len);
int black_hole_read(connection * con);

#ifdef __cplusplus
}
#endif

#endif
