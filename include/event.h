#ifndef EVENT_H
#define EVENT_H

#include "mqtts.h"

int init_event(int sfd, struct epoll_event* event, struct epoll_event* events);
void add_event(struct epoll_event* event, int efd, int fd, int mask);

#endif /* EVENT_H */
