#ifndef SERVER_H
#define SERVER_H

#include "socketwrappers.h"

void server();
void* echo_t(void* connfd);

#endif
