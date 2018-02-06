#include <sys/resource.h>
#include "common.h"

void set_fd_limit() {
    struct rlimit lim;
    lim.rlim_cur = RLIM_INFINITY;
    lim.rlim_max = RLIM_INFINITY;
    check(setrlimit(RLIMIT_NOFILE, &lim) != -1);
}
