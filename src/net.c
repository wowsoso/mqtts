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

#include "net.h"


int clear_connection(mqtts_t* mqtts, int sock)
{
    memset (&mqtts->conns[sock], 0, sizeof (connection_t));
    return 0;
}

void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);

    if (flags == -1)
    {
        perror ("fcntl(F_GETFL)");
    }

    if ((flags & O_NONBLOCK) == O_NONBLOCK)
    {
        return;
    }

    flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    if (flags == -1)
    {
        perror ("fcntl(F_SETFL)");
    }
}

int init_net(struct addrinfo* res, char* port)
{
    int s;
    int sfd;
    struct addrinfo hints;
    struct addrinfo* res0;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    if ((s = getaddrinfo(NULL, port, &hints, &res0)) != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
        return -1;
    }

    for (res = res0; res != NULL; res = res->ai_next)
    {
        sfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sfd == -1)
            continue;

        if ((s = bind (sfd, res->ai_addr, res->ai_addrlen)) == 0);
        {
            break;
        }

        close (sfd);
    }

    if (res == NULL)
    {
        fprintf (stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo (res0);

    set_nonblocking (sfd);

    if ((s = listen (sfd, SOMAXCONN) == -1))
    {
        perror ("listen error");
    }

    return sfd;

}
