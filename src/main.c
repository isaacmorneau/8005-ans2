#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <getopt.h>

#include "poll_server.h"
#include "t_server.h"
#include "client.h"
#include "epoll_server.h"
#include "common.h"
#include "wrapper.h"
#include "logging.h"

#define SOCKOPTS "csothmp:a:r:"

void print_help(void){
    puts("usage options:\n"
            "\t[c]lient - set the mode to client\n"
            "\t[s]erver - set the mode to server\n"
            "\t[m]ax - set the mode to maximize\n"
            "\tp[o]llerver - set the mode to poll\n"
            "\t[t]raditional erver - set the mode to traditional\n"
            "\t[r]ate <default 500ms> - milisecond delay before adding new clients\n"
            "\t[l]imit <default infinity> - the maximum number of clients to connect\n"
            "\t[p]ort <default 54321> - the port to connect to\n"
            "\t[a]ddress <default localhost> - only used by client for connecting to a server\n"
            "\t[h]elp - this message");
}

int main (int argc, char *argv[]) {
    if (argc == 1) {
        print_help();
        return 0;
    }
    int c;

    int server_mode = 0;
    bool client_mode = 0;
    bool max_mode = 0;

    char * port = "54321";
    char * address = 0;

    int rate = 500;
    int limit = -1;

    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            {"client",      no_argument,       0, 'c' },
            {"server",      no_argument,       0, 's' },
            {"poll",        no_argument,       0, 'o' },
            {"max",         no_argument,       0, 'm' },
            {"traditional", no_argument,       0, 't' },
            {"help",        no_argument,       0, 'h' },
            {"rate",        required_argument, 0, 'r' },
            {"limit",       required_argument, 0, 'l' },
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
                    puts("Server and client requested, exiting\n");
                    return 1;
                }
                client_mode = 1;
                break;
            case 's':
                if (client_mode) {
                    puts("Client and server requested, exiting\n");
                    return 1;
                }
                server_mode = 1;
                break;
            case 'o':
                if (client_mode) {
                    puts("Client and server requested, exiting\n");
                    return 1;
                }
                server_mode = 2;
                break;
            case 't':
                if (client_mode) {
                    puts("Client and server requested, exiting\n");
                    return 1;
                }
                server_mode = 3;
                break;
            case 'm':
                max_mode = 1;
                break;
            case 'r':
                rate = atoi(optarg);
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

    set_fd_limit();

    init_logging(0);
    if (server_mode) {
        switch (server_mode) {
            case 1:
                epoll_server(port, max_mode);
                break;
            case 2:
                poll_server(port);
                break;
            case 3:
                server(port);
                break;
            default:
                return 0;
        }
    } else if (client_mode) {
        client(address, port, rate, limit, max_mode);
    }
    close_logging();
    return 0;
}
