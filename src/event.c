/*
--------------------------The MIT License---------------------------
Copyright (c) 2013 Qi Wang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "event.h"

int init_event(int sfd, struct epoll_event* event, struct epoll_event* events)
{
    int s;
    int efd;

    if ((efd = epoll_create1 (0)) == -1)
    {
        perror ("epoll_create");
        abort ();
    };

    event->data.fd = sfd;
    event->events = EPOLLIN | EPOLLET;

    if ((s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, event)) == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }

    events = calloc (MQTTS_MAXEVENTS, sizeof (*event));

    return efd;
}

void add_event(struct epoll_event* event, int efd, int fd, int mask)
{
    /*
        mask ==> {EPOLLIN,EPOLLOUT}
    */
    int s;

    event->data.fd = fd;
    event->events = mask | EPOLLET;

    if ((s = epoll_ctl (efd, EPOLL_CTL_ADD, fd, event)) == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }
}
