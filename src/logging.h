#ifndef LOGGING_H
#define LOGGING_H

#include <sys/types.h>
#include <sys/socket.h>

int laccept(int socket, struct sockaddr * address, socklen_t * address_len);
ssize_t lsend(int socket, const void *buffer, size_t length, int flags);
ssize_t lrecv(int socket, void *buffer, size_t length, int flags);
int lconnect(int socket, const struct sockaddr *address, socklen_t address_len);
int laccept(int socket, struct sockaddr *address, socklen_t *address_len);

void init_logging(const char * path);
void close_logging();

#endif
