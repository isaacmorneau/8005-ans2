#ifndef LOGGING_H
#define LOGGING_H

#include <sys/types.h>

void new_con(int fd);
void lost_con(int fd);

ssize_t lsend(int socket, const void *buffer, size_t length, int flags);
ssize_t lrecv(int socket, void *buffer, size_t length, int flags);

void init_logging(const char * path);
void close_logging();

#endif
