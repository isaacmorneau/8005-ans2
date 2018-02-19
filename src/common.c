#include <sys/resource.h>
#include <limits.h>
#include "common.h"

void set_fd_limit() {
    struct rlimit lim;
    //ensure(getrlimit(RLIMIT_NOFILE, &lim) != -1);
    //the kernel patch that allows for RLIM_INFINITY to work breaks stuff
    //so we are restricted to finite values
    lim.rlim_cur = USHRT_MAX;
    lim.rlim_max = USHRT_MAX;
    ensure(setrlimit(RLIMIT_NOFILE, &lim) != -1);
}
