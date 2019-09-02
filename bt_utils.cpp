#include "bt_utils.h"
#include <fcntl.h>

int setNonBlock(int fd) {
    int option = fcntl(fd, F_GETFL);
	option |= O_NONBLOCK;
	fcntl(fd, F_SETFL, option);
	return option;
}