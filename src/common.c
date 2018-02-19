#include <sys/resource.h>
#include "common.h"

void set_fd_limit() {
    struct rlimit lim;
    ensure(getrlimit(RLIMIT_NOFILE, &lim) != -1);
    lim.rlim_cur = lim.rlim_max;
    ensure(setrlimit(RLIMIT_NOFILE, &lim) != -1);
}
