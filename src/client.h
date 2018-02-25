#ifndef CLIENT_H
#define CLIENT_H
#include <stdbool.h>

void client(const char * address, const char * port, int rate, int limit, int runtime, bool max);

#endif
