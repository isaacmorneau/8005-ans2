#ifndef SELECT_H
#define SELECT_H

#include "socketwrappers.h"
#include <netinet/in.h>
#include <stdlib.h>

#define LISTENQ 5   //number of connections to listen for
#define LISTEN_PORT 8005

using namespace std;

void select_server();

typedef struct {
        int id;
        int duration;
        int requests;
        int sent;
        int recv;
} Record;

#endif

