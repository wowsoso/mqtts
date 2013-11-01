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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "handle.h"

int handle_connect(mqtts_t* mqtts, int sock, int remaining_length, int fixed_header_length, uint8_t step)
{
    int i;
    char* sub;

    if (mqtts->conns[sock].package_in.pos == remaining_length + fixed_header_length)
    {
        printf ("%s\n", "connected!!");
        return 0;
    }

    if (step == 1)
    {
        /* get protocol name */

        uint8_t msb, lsb;
        uint16_t str_len;

        msb = mqtts->conns[sock].package_in.buf[fixed_header_length + 0];
        lsb = mqtts->conns[sock].package_in.buf[fixed_header_length + 1];
        str_len = (msb << 8) + lsb;
        sub = malloc(sizeof(char) * str_len + 1);
        strncpy (sub, mqtts->conns[sock].package_in.buf + fixed_header_length + 2, str_len);
        mqtts->conns[sock].package_in.pos = fixed_header_length + 2 + str_len;
        printf ("%s\n", sub);
        handle_connect(mqtts, sock, remaining_length, fixed_header_length, 2);
        free(sub);
    }

    if (step == 2)
    {
        /* get protocol version number */

        sub = malloc(sizeof(char));
        strncpy (sub, mqtts->conns[sock].package_in.buf + mqtts->conns[sock].package_in.pos, 1);
        mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos + 1;
        printf ("%d\n", *sub);
        handle_connect(mqtts, sock, remaining_length, fixed_header_length, 3);
        free(sub);
    }

    if (step == 3)
    {
        /* get connect flags */

        sub = malloc(sizeof(char) + 1);
        strncpy (sub, mqtts->conns[sock].package_in.buf + mqtts->conns[sock].package_in.pos, 1);
        mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos + 1;
        printf ("%x\n", *sub);
        handle_connect(mqtts, sock, remaining_length, fixed_header_length, 4);
        free(sub);
    }

    if (step == 4)
    {
        /* get keep alive timer */
        uint8_t msb, lsb;
        uint16_t keep_alive_timer;

        msb = mqtts->conns[sock].package_in.buf[mqtts->conns[sock].package_in.pos];
        lsb = mqtts->conns[sock].package_in.buf[mqtts->conns[sock].package_in.pos + 1];
        keep_alive_timer = (msb << 8) + lsb;
        mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos + 2;
        printf ("%d\n", 4);
        handle_connect(mqtts, sock, remaining_length, fixed_header_length, -1);
    }

    return -1;
}

int handle_unknown(mqtts_t* mqtts, int sock, int remaining_length, int fixed_header_length, uint8_t step)
{
    printf ("%s\n", "unknown!!");
    return 0;
}


handler_t get_handler(uint8_t command)
{

    if ((command & 0xF0) == 0x10)
    {
        return handle_connect;
    }
    else
    {
        return handle_unknown;
    }
}


int handle(mqtts_t* mqtts, int sock)
{

    uint8_t command;
    int remaining_length;
    int fixed_header_length;
    uint8_t* buf;

    buf = mqtts->conns[sock].package_in.buf;
    command = buf[0];

    int multiplier = 1;
    int count = 0;
    remaining_length = 0;

    do
    {
        count++;
        remaining_length += (buf[count] & 127) * multiplier;
        multiplier *= 128;
    } while (buf[count] & 128 != 0);

    fixed_header_length = 1 + count;

    get_handler(command)(mqtts, sock, remaining_length, fixed_header_length, 1);

}
