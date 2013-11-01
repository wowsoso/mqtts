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

#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "mqtts.h"
#include "net.h"
#include "handle.h"

void loop_accepted(mqtts_t* mqtts, struct epoll_event* event, int sfd, int efd)
{
    int s;
    while (1)
    {
        struct sockaddr in_addr;
        socklen_t in_len;
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
        int fd;

        in_len = sizeof in_addr;
        fd = accept (sfd, &in_addr, &in_len);
        if (fd == -1)
        {
            if ((errno == EAGAIN) ||
                (errno == EWOULDBLOCK))
            {
                break;
            }
            else
            {
                perror ("accept");
                break;
            }
        }
        else
        {
            connection_t conn;
            conn.fd = fd;
            memset(&conn.package_in, 0, sizeof (package_t));
            conn.package_in.buf = malloc(2048);
            conn.package_out.buf = malloc(2048);
            conn.conn_state = connect_conn_state_connected;
            conn.mqtt_state = connect_mqtt_state_wait_connect;
            /* FIXME: defense array size limit */
            mqtts->conns[conn.fd] = conn;

            s = getnameinfo (&in_addr, in_len,
                             hbuf, sizeof hbuf,
                             sbuf, sizeof sbuf,
                             NI_NUMERICHOST | NI_NUMERICSERV);
            if (s == 0)
            {
                printf ("Accepted connection on descriptor %d "
                       "(host=%s, port=%s)\n", fd, hbuf, sbuf);
            }

            /* Make the incoming socket non-blocking and add it to the
               list of fds to monitor. */
            set_nonblocking (fd);

            if (s == -1)
                abort ();

            event->data.fd = fd;
            event->events = EPOLLIN | EPOLLET;
            s = epoll_ctl (efd, EPOLL_CTL_ADD, fd, event);
            printf ("%s\n", "eppp");
            if (s == -1)
            {
                perror ("epoll_ctl");
                abort ();
            }
        }
    }
}


int loop_read(mqtts_t* mqtts, struct epoll_event* events, struct epoll_event* event, int event_index)
{
    int s;
    int done = 0;

    while (1)
    {
        /* count = read (events[i].data.fd, buf.package, sizeof buf.package); */
        s = read (events[event_index].data.fd, mqtts->conns[events[event_index].data.fd].package_in.buf, 1024);

        if (s == -1)
        {
            /* If errno == EAGAIN, that means we have read all
               data. So go back to the main loop. */
            if (errno != EAGAIN)
            {
                perror ("read");
                done = 1;
            }
            break;
        }
        else if (s == 0)
        {
            /* End of file. The remote has closed the
               connection. */
            done = 1;
            break;
        }

        handle(mqtts, events[event_index].data.fd);

        if (s == -1)
        {
            perror ("write");
            return -1;
        }

    }

    if (done)
    {
        printf ("Closed connection on descriptor %d\n",
                events[event_index].data.fd);

        /* handle(mqtts, events[i].data.fd, 0); */
        /* Closing the descriptor will make epoll remove it
           from the set of descriptors which are monitored. */
        close (events[event_index].data.fd);
        clear_connection(mqtts, events[event_index].data.fd);
    }

    return 0;
}

int loop(mqtts_t *mqtts, struct epoll_event* event, struct epoll_event* events, int sfd, int efd)
{
    while (1)
    {
        int n, i;

        n = epoll_wait (efd, events, MQTTS_MAXEVENTS, -1);

        if (n == -1)
        {
            printf ("error:epoll_wait:%d\n", errno);
            abort ();
        }
        for (i = 0; i < n; i++)
        {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN)))
            {
                /* An error has occured on this fd, or the socket is not
                   ready for reading (why were we notified then?) */
                fprintf (stderr, "epoll error\n");
                close (events[i].data.fd);
                clear_connection(mqtts, events[i].data.fd);
                continue;
            }
            else if (sfd == events[i].data.fd)
            {
                /* We have a notification on the listening socket, which
                   means one or more incoming connections. */
                loop_accepted (mqtts, event, sfd, efd);
                continue;
            }
            else
            {
                loop_read (mqtts, events, event, i);
            }
        }
    }

    free (events);
    close (sfd);
    return 0;
}
