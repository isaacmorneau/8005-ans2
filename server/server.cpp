//8005 Assignment 2

#include <stdio.h>
#include <getopt.h>
#include "../library/select.h"

#define LISTEN_PORT 8005
#define NORM_MODE 1
#define SELECT_MODE 2
#define EPOLL_MODE 3

static int mode = 0;

int main(int argc, char** argv) {
        while(1) {
                int option_index = 0;
                static int c = 0;
                static struct option long_options[] = {
                        {"normal",  no_argument, 0, 'n'},
                        {"select",  no_argument, 0, 's'},
                        {"epoll",   no_argument, 0, 'e'},
                        {"help",    no_argument, 0, 'h'},
                        {0,         0,           0, 0}
                };

                c = getopt_long(argc, argv, "opt:i:h", long_options, &option_index);
                if(c == -1) 
                        break;
                
                switch(c) {
                        case 'n':
                                mode = NORM_MODE; 
                                //norm_server();
                                break;
                        case 's':
                                mode = SELECT_MODE;
                                //select_server();
                                break;
                        case 'e':
                                mode = EPOLL_MODE;
                                //epoll_server();
                                break;
                        case 'h':
                        case '?':
                        default:
                                //print_help();
                                return 1;
                }
        }

        if(!mode){
                //print_help();
                return 1;
        }

        switch(mode) {
                case NORM_MODE:
                        //norm_server();
                        break;
                case SELECT_MODE:
                        select_server();
                        break;
                case EPOLL_MODE:
                        //epoll_server();
                        break;
                default:
                        //print_help();
                        return 1;
        }

        return 0;
}


