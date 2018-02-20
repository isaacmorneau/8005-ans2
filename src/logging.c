#include "logging.h"
#include <stdio.h>
#include <stdatomic.h>

atomic_int total_clients = 0;

void new_con(int fd) {
    printf("++tc:%d fd:%d\n", ++total_clients, fd);
}
void lost_con(int fd) {
    printf("--tc:%d fd:%d\n", --total_clients, fd);

}
