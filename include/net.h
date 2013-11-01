#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include "mqtts.h"


int clear_connection(mqtts_t* mqtts, int sock);
void set_nonblocking(int fd);
int init_net(struct addrinfo* res, char* port);
