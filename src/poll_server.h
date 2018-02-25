#ifndef POLL_SERVER_H
#define POLL_SERVER_H

struct pass_poll {
    int *thread_num;
    struct pollfd *client;
};

void poll_server(const char* port);

#endif
