#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <getopt.h>

#include "client.h"
#include "common.h"

#define SOCKOPTS "cshp:a:"

void print_help(void){
    printf("usage options:\n"
            "\t[c]lient - set the mode to client\n"
            "\t[s]erver - set the mode to server\n"
            "\t[p]ort <1-65535>> - the port to connect to\n"
            "\t[a]ddress <ip or url> - only used by client for connecting to a server\n"
            "\t[h]elp - this message\n");
}

int main (int argc, char *argv[]) {
    if (argc == 1) {
        print_help();
        return 0;
    }
    int c;

    bool server_mode = 0;
    bool client_mode = 0;
    int connect_mode = 0;

    char * port = 0;
    char * data = 0;
    char * address = 0;

    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            {"client",      no_argument,       0, 'c' },
            {"server",      no_argument,       0, 's' },
            {"help",        no_argument,       0, 'h' },
            {"port",        required_argument, 0, 'p' },
            {"address",     required_argument, 0, 'a' },
            {0,             0,                 0, 0   }
        };

        c = getopt_long(argc, argv, SOCKOPTS, long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'c':
                if (server_mode) {
                    printf("Server and client requested, exiting\n");
                    return 1;
                }
                client_mode = 1;
                break;
            case 's':
                if (client_mode) {
                    printf("Client and server requested, exiting\n");
                    return 1;
                }
                server_mode = 1;
                break;
            case 'p':
                port = optarg;
                break;
            case 'a':
                address = optarg;
                break;
            case 'h':
            case '?':
            default:
                print_help();
                return 0;
        }
    }

    if (server_mode) {
        return server(port);
    } else if (client_mode) {
        return client(address, port);
    } else {
        printf("Mode not specified, exiting\n");
        return 1;
    }

}
