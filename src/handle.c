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
#include <sys/timerfd.h>
#include "handle.h"

static int encode_remaining_length(int remaining_len,
                                   int* digit,
                                   int* bit_count)
{
    do
    {
        *digit = remaining_len % 128;
        remaining_len = remaining_len / 128;
        if (remaining_len > 0)
        {
            *digit = *digit | 0x80;
        }
    } while (remaining_len > 0);

    if (*digit < 128)
    {
        *bit_count = 1;
    }
    else if (*digit < 16384)
    {
        *bit_count = 2;
    }
    else if (*digit < 2097152)
    {
        *bit_count = 3;
    }
    else if (*digit < 268435456)
    {
        *bit_count = 4;
    }
    else
    {
        /* FIXME return a err msg */
        return -1;
    }

    return 0;
}

static void read_mqtt_string(mqtts_t* mqtts,
                             int sock, uint8_t** sub,
                             uint8_t* msb,
                             uint8_t* lsb)
{
    uint16_t str_len;

    *msb = mqtts->conns[sock]->package_in.buf[
        mqtts->conns[sock]->package_in.pos];
    *lsb = mqtts->conns[sock]->package_in.buf[
        mqtts->conns[sock]->package_in.pos + 1];

    str_len = (*msb << 8) + *lsb;

    *sub = malloc (str_len * sizeof (uint8_t) + 1);
    memset (*sub, 0, str_len * sizeof (uint8_t) + 1);

    strncpy (*sub,
             mqtts->conns[sock]->package_in.buf +
             mqtts->conns[sock]->package_in.pos + 2,
             str_len);

    mqtts->conns[sock]->package_in.pos = mqtts->conns[sock]->package_in.pos
        + 2 + str_len;
}

static void read_mqtt_byte(mqtts_t* mqtts,
                           int sock,
                           uint8_t** sub)
{
    *sub = malloc(sizeof(uint8_t) + 1);
    memset (*sub, 0, sizeof (uint8_t) + 1);

    strncpy (*sub,
             mqtts->conns[sock]->package_in.buf +
             mqtts->conns[sock]->package_in.pos, 1);

    mqtts->conns[sock]->package_in.pos = mqtts->conns[sock]->package_in.pos + 1;
}

static void read_mqtt_byte2(mqtts_t* mqtts,
                            int sock,
                            uint16_t** sub)
{
    uint8_t msb, lsb;
    uint16_t str_len;

    msb = mqtts->conns[sock]->package_in.buf[
        mqtts->conns[sock]->package_in.pos];
    lsb = mqtts->conns[sock]->package_in.buf[
        mqtts->conns[sock]->package_in.pos + 1];

    str_len = (msb << 8) + lsb;

    *sub = malloc (sizeof (uint16_t));
    memset (*sub, 0, sizeof (uint16_t) + 1);

    strncpy (*sub, &str_len, 2);

    mqtts->conns[sock]->package_in.pos = mqtts->conns[sock]->package_in.pos + 2;
}

int handle_connack(mqtts_t* mqtts, int sock)
{
    char a[4];
    a[0] = 32;
    a[1] = 2;
    a[2] = 0;
    a[3] = MQTTS_CONNACK_Accepted;

    send(mqtts->conns[sock]->fd, a, 4, 0);
    mqtts->conns[sock]->mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock]->state = 0;

    free_fixed_header ((fixed_header_t*)(mqtts->conns[sock]->fixed_header));
    free_variable_header_connect ((variable_header_connect_t*)(mqtts->conns[sock]->variable_header));
    free_payload_connect ((payload_connect_t*)(mqtts->conns[sock]->payload));
    mqtts->conns[sock]->fixed_header = NULL;
    mqtts->conns[sock]->variable_header = NULL;
    mqtts->conns[sock]->payload = NULL;
}

static int handle_puback_client(mqtts_t* mqtts, int sock)
{
    uint8_t a[4];
    int i;
    topic_t* topics;
    subscriber_t* subscribers;

    a[0] = 64; /* 0100 0000 */
    a[1] = 2;  /* 0000 0010 */
    a[2] = ((variable_header_publish_t*)(mqtts->conns[sock]->variable_header))->message_id >> 8;   /*  msb  */
    a[3] = ((variable_header_publish_t*)(mqtts->conns[sock]->variable_header))->message_id;        /*  lsb  */

    if (mqtts->conns[sock]->fixed_header->qos > 0)
    {
        printf ("qos----%d\n", mqtts->conns[sock]->fixed_header->qos);
        send (mqtts->conns[sock]->fd, a, 4, 0);
    }

    topics = get_topic (mqtts->topics,
                        ((variable_header_publish_t*)(mqtts->conns[sock]->variable_header))->topic_name);
    printf ("%s\n",
            ((variable_header_publish_t*)(mqtts->conns[sock]->variable_header))->topic_name);
    subscribers = topics->subscribers;

    mqtts->conns[sock]->mqtt_state = connect_mqtt_state_wait_publish;
    mqtts->conns[sock]->state = 1;

    /* publish_to_client (mqtts, subscribers, */
    /*                    mqtts->conns[sock]->fixed_header, */
    /*                    ((variable_header_publish_t*)(mqtts->conns[sock]->variable_header)), */
    /*                    ((payload_publish_t*)(mqtts->conns[sock]->payload)), 0); */

    /* free_fixed_header ((fixed_header_t*)(mqtts->conns[sock]->fixed_header)); */
    /* free_variable_header_publish ((variable_header_publish_t*)(mqtts->conns[sock]->variable_header)); */
    /* free_payload_publish ((payload_publish_t*)(mqtts->conns[sock]->payload)); */
    /* mqtts->conns[sock]->fixed_header = NULL; */
    /* mqtts->conns[sock]->variable_header = NULL; */
    /* mqtts->conns[sock]->payload = NULL; */
}

static int handle_to_publish(mqtts_t* mqtts, int sock)
{
    uint8_t* a;
    uint16_t mid = mqtts->last_mid + 1;
    uint16_t _mid;
    int payload_len;
    int remaining_len;
    int bit_count;
    int digit;
    int pos;
    int i;
    int tfd;
    topic_t* topics;
    subscriber_t* subscribers;
    message_t* message;

    struct epoll_event* event  = mqtts->event;
    struct epoll_event* events = mqtts->events;;
    int efd                    = mqtts->efd;


    fixed_header_t* fixed_header = mqtts->conns[sock]->fixed_header;
    variable_header_publish_t* variable_header = (variable_header_publish_t*)(mqtts->conns[sock]->variable_header);
    payload_publish_t* payload = (payload_publish_t*)(mqtts->conns[sock]->payload);

    topics = get_topic (mqtts->topics, variable_header->topic_name);
    subscribers = topics->subscribers;

    payload_len = strlen (payload->data);
    remaining_len = payload_len;
    remaining_len = remaining_len + 2;  /* topic_name lsb & msb */
    remaining_len = remaining_len + strlen (variable_header->topic_name);

    if (fixed_header->qos > 0) {
        remaining_len = remaining_len + 2;  /* message id */
    }

    if (! payload_len)
    {
        /* FIXME: maybe returns a err msg to client that is right */
    }

    if (! remaining_len)
    {
        /* FIXME: maybe returns a err msg to client that is right */
    }

    encode_remaining_length (remaining_len, &digit, &bit_count);

    a = malloc (1 + bit_count + remaining_len);
    memset (a, 0, 1 + bit_count + remaining_len);

    fixed_header->dup = dup;
    a[0] = fixed_header->message_type << 4 |
           fixed_header->dup << 3 |
           fixed_header->qos << 1 |
           fixed_header->retain;

    for (i = 0; i < bit_count; i++)
    {
        if (i == bit_count - 1)
        {
            a[i + 1] = digit - 127 * i;
        }
        else
        {
            a[i + 1] = 127;
        }
    }
    pos = bit_count + 1;

    a[pos++] = variable_header->topic_name_msb;
    a[pos++] = variable_header->topic_name_lsb;

    memcpy (a + pos,
            variable_header->topic_name,
            strlen (variable_header->topic_name));
    pos = pos + strlen (variable_header->topic_name);

    _mid = htons(mid);

    if (fixed_header->qos > 0) {
        memcpy (a + pos, &_mid, 2);
        pos = pos + 2;
    }

    printf ("-------------%s\n", payload->data);
    memcpy (a+pos, payload->data, payload_len);

    if (fixed_header->qos > 0) {
        tfd = timerfd_create (CLOCK_REALTIME, TFD_NONBLOCK);
        printf ("!-----%d\n", tfd);
        message = insert_message (mqtts, a, mid, 1 + bit_count + remaining_len, subscribers, tfd);

        struct itimerspec newValue;
        struct itimerspec oldValue;

        bzero(&newValue,sizeof(newValue));
        bzero(&oldValue,sizeof(oldValue));

        struct timespec ts;

        ts.tv_sec = 2;
        ts.tv_nsec = 0;
        newValue.it_value = ts;
        newValue.it_interval = ts;

        if ( timerfd_settime (tfd, 0, &newValue, &oldValue) < 0)
        {
            printf ("settime error\n");
            abort ();
        }
        printf ("%s\n", "fufufu");
        add_event (event, efd, tfd, EPOLLIN);
    }


    while (subscribers)
    {
        printf ("%s\n", "shit");
        printf ("%d\n", ((connection_t*)subscribers->connection)->fd);
        send(((connection_t*)subscribers->connection)->fd,
             a,
             1 + bit_count + remaining_len, 0);
        subscribers = subscribers->next;
    }

    free (a);

    mqtts->conns[sock]->mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock]->state = 0;

    free_fixed_header (fixed_header);
    free_variable_header_publish (variable_header);
    free_payload_publish (payload);
    mqtts->conns[sock]->fixed_header = NULL;
    mqtts->conns[sock]->variable_header = NULL;
    mqtts->conns[sock]->payload = NULL;
}

static int handle_suback(mqtts_t* mqtts, int sock)
{
    uint8_t* a;
    int payload_len;
    int remaining_len;
    int bit_count;
    int digit;
    int i;
    int pos;
    payload_subscribe_topic_t* topics;

    topics = ((payload_subscribe_t*)(mqtts->conns[sock]->payload))->topics;

    payload_len = 0;
    while (topics)
    {
        payload_len++;
        topics = topics->next;
    }

    if (! payload_len)
    {
        /* FIXME: maybe returns a err msg to client that is right */
    }

    remaining_len = payload_len + 2;

    encode_remaining_length (remaining_len, &digit, &bit_count);

    topics = ((payload_subscribe_t*)(mqtts->conns[sock]->payload))->topics;

    a = malloc (1 + bit_count + remaining_len);
    memset (a, 0, 1 + bit_count + remaining_len);

    a[0] = 144; /* 1001 0000 */
    for (i = 0; i < bit_count; i++)
    {
        if (i == bit_count - 1)
        {
            a[i + 1] = digit - 127 * i;
        }
        else
        {
            a[i + 1] = 127;
        }
    }
    pos = bit_count + 1;


    a[pos++] = ((variable_header_subscribe_t*)(mqtts->conns[sock]->variable_header))->message_id >> 8;   /*  msb  */
    a[pos++] = ((variable_header_subscribe_t*)(mqtts->conns[sock]->variable_header))->message_id;        /*  lsb  */

    while (topics)
    {
        a[pos++] = topics->qos;
        topics = topics->next;
    }

    send(mqtts->conns[sock]->fd, a, 1 + bit_count + remaining_len, 0);
    mqtts->conns[sock]->mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock]->state = 0;

    free (a);
    free_fixed_header ((fixed_header_t*)(mqtts->conns[sock]->fixed_header));
    free_variable_header_subscribe ((variable_header_subscribe_t*)(mqtts->conns[sock]->variable_header));
    free_payload_subscribe ((payload_subscribe_t*)(mqtts->conns[sock]->payload));
    mqtts->conns[sock]->fixed_header = NULL;
    mqtts->conns[sock]->variable_header = NULL;
    mqtts->conns[sock]->payload = NULL;
}

static int handle_unsuback(mqtts_t* mqtts, int sock)
{
    uint8_t a[4];

    a[0] = 176; /* 1011 0000 */
    a[1] = 2;  /* 0000 0010 */
    a[2] = ((variable_header_unsubscribe_t*)(mqtts->conns[sock]->variable_header))->message_id >> 8;   /*  msb  */
    a[3] = ((variable_header_unsubscribe_t*)(mqtts->conns[sock]->variable_header))->message_id;        /*  lsb  */

    send(mqtts->conns[sock]->fd, a, 4, 0);
    free_fixed_header ((fixed_header_t*)(mqtts->conns[sock]->fixed_header));
    /* free_variable_header_unsub ((variable_header_unsubscribe_t*)(mqtts->conns[sock]->variable_header)); */
    mqtts->conns[sock]->fixed_header = NULL;
    printf ("%d\n", sock);
    mqtts->conns[sock]->mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock]->state = 0;
    /* mqtts->conns[sock]->variable_header = NULL; */
    /* mqtts->conns[sock]->payload = NULL;     */
}

static int handle_pingresp(mqtts_t* mqtts, int sock)
{
    uint8_t a[2];

    a[0] = 208; /* 1101 0000 */
    a[1] = 0;  /* 0000 0010 */

    mqtts->conns[sock]->mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock]->state = 0;

    send(mqtts->conns[sock]->fd, a, 2, 0);
    free_fixed_header ((fixed_header_t*)(mqtts->conns[sock]->fixed_header));
    mqtts->conns[sock]->fixed_header = NULL;
}

static int handle_connect(mqtts_t* mqtts,
                   int sock,
                   fixed_header_t* fixed_header,
                   void* variable_header,
                   void* payload,
                   int fixed_header_length,
                   uint8_t step)
{
    int i;

    if (mqtts->conns[sock]->package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {

        mqtts->conns[sock]->fixed_header = fixed_header;
        mqtts->conns[sock]->variable_header = variable_header;
        mqtts->conns[sock]->payload = payload;
        mqtts->conns[sock]->mqtt_state = connect_mqtt_state_wait_connack;
        mqtts->conns[sock]->state = 1;
        memset (mqtts->conns[sock]->package_in.buf, 0, 2048);
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock]->package_in.pos = fixed_header_length;
        variable_header = malloc (sizeof (variable_header_connect_t));
        memset (variable_header, 0, sizeof (variable_header_connect_t));
        handle_connect (mqtts,
                        sock,
                        fixed_header,
                        variable_header,
                        payload,
                        fixed_header_length, 1);
    }

    if (step == 1)
    {
        /* get protocol name */
        uint8_t* sub;
        uint8_t msb;
        uint8_t lsb;

        read_mqtt_string (mqtts, sock, &sub, &msb, &lsb);
        ((variable_header_connect_t*)variable_header)->protocol_name = sub;
        handle_connect (mqtts,
                        sock,
                        fixed_header,
                        variable_header,
                        payload,
                        fixed_header_length, 2);
    }

    if (step == 2)
    {
        /* get protocol version number */
        uint8_t* sub;
        read_mqtt_byte (mqtts, sock, &sub);
        ((variable_header_connect_t*)variable_header)->protocol_version_number = *sub;
        handle_connect (mqtts,
                        sock,
                        fixed_header,
                        variable_header,
                        payload,
                        fixed_header_length, 3);
        free(sub);
    }

    if (step == 3)
    {
        /* get connect flags */
        uint8_t* sub;
        read_mqtt_byte (mqtts, sock, &sub);

        ((variable_header_connect_t*)variable_header)->username_flag = sub[0] & 0x80;
        ((variable_header_connect_t*)variable_header)->password_flag = sub[0] & 0x40;
        ((variable_header_connect_t*)variable_header)->will_retain   = sub[0] & 0x20;
        ((variable_header_connect_t*)variable_header)->will_qos      = sub[0] & 0x18;
        ((variable_header_connect_t*)variable_header)->will_flag     = sub[0] & 0x04;
        ((variable_header_connect_t*)variable_header)->clean_session = sub[0] & 0x02;
        ((variable_header_connect_t*)variable_header)->reserved      = 0;

        handle_connect (mqtts,
                        sock,
                        fixed_header,
                        variable_header,
                        payload,
                        fixed_header_length, 4);
    }

    if (step == 4)
    {
        /* get keep alive timer */
        uint16_t* sub;

        read_mqtt_byte2 (mqtts, sock, &sub);
        ((variable_header_connect_t*)variable_header)->keep_alive_timer = *sub;
        handle_connect(mqtts,
                       sock,
                       fixed_header,
                       variable_header,
                       payload,
                       fixed_header_length, 5);
    }

    if (step == 5)
    {
        /* get payload */
        uint8_t* sub;
        uint8_t msb;
        uint8_t lsb;

        read_mqtt_string (mqtts, sock, &sub, &msb, &lsb);
        handle_connect(mqtts,
                       sock,
                       fixed_header,
                       variable_header,
                       payload,
                       fixed_header_length, -1);
        free (sub);
    }

    return -1;
}

static int handle_publish(mqtts_t* mqtts,
                   int sock,
                   fixed_header_t* fixed_header,
                   void* variable_header,
                   void* payload,
                   int fixed_header_length,
                   uint8_t step)
{
    int i;

    if (mqtts->conns[sock]->package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        mqtts->conns[sock]->fixed_header = fixed_header;
        mqtts->conns[sock]->variable_header = variable_header;
        mqtts->conns[sock]->payload = payload;

        if (fixed_header->qos == 0) {
            /* FIXME return 0 directly */
            mqtts->conns[sock]->mqtt_state = connect_mqtt_state_wait_puback;
        }

        if (fixed_header->qos == 1) {
            mqtts->conns[sock]->mqtt_state = connect_mqtt_state_wait_puback;
        }

        int ss = 0;
        ss = insert_topic (mqtts->topics,
                          ((variable_header_publish_t*)(mqtts->conns[sock]->variable_header))->topic_name,
                          ((payload_publish_t*)(mqtts->conns[sock]->payload))->data);

        mqtts->conns[sock]->state = 1;
        memset (mqtts->conns[sock]->package_in.buf, 0, 2048);
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock]->package_in.pos = fixed_header_length;

        variable_header = malloc (sizeof (variable_header_publish_t));
        memset (variable_header, 0, sizeof (variable_header_publish_t));
        handle_publish(mqtts,
                       sock,
                       fixed_header,
                       variable_header,
                       payload,
                       fixed_header_length, 1);
    }
    if (step == 1)
    {
        /* get topic name */
        uint8_t* sub;
        uint8_t msb;
        uint8_t lsb;

        read_mqtt_string (mqtts, sock, &sub, &msb, &lsb);
        ((variable_header_publish_t*)variable_header)->topic_name_msb = msb;
        ((variable_header_publish_t*)variable_header)->topic_name_lsb = lsb;
        ((variable_header_publish_t*)variable_header)->topic_name = sub;
        handle_publish (mqtts,
                        sock,
                        fixed_header,
                        variable_header,
                        payload,
                        fixed_header_length, 2);
    }
    if (step == 2)
    {
        /* get message id */

        payload = malloc (sizeof (payload_publish_t));
        memset (payload, 0, sizeof (payload_publish_t));

        if (fixed_header->qos == 0)
        {
            ((variable_header_publish_t*)variable_header)->message_id = NULL;
        }
        else
        {
            uint16_t* sub;

            read_mqtt_byte2 (mqtts, sock, &sub);
            ((variable_header_publish_t*)variable_header)->message_id = *sub;
        }
        handle_publish (mqtts,
                        sock,
                        fixed_header,
                        variable_header,
                        payload,
                        fixed_header_length, 3);
    }
    if (step == 3)
    {
        /* get payload */
        uint8_t* sub;

        sub = malloc (fixed_header->remaining_length - mqtts->conns[sock]->package_in.pos + fixed_header_length);
        memset (sub, 0, fixed_header->remaining_length - mqtts->conns[sock]->package_in.pos + fixed_header_length);
        strncpy(sub,
                mqtts->conns[sock]->package_in.buf + mqtts->conns[sock]->package_in.pos,
                fixed_header->remaining_length - mqtts->conns[sock]->package_in.pos + fixed_header_length);
        mqtts->conns[sock]->package_in.pos = fixed_header->remaining_length + fixed_header_length;

        ((payload_publish_t*)payload)->data = sub;
        handle_publish (mqtts,
                        sock,
                        fixed_header,
                        variable_header,
                        payload,
                        fixed_header_length, -1);
    }
}

static int handle_puback(mqtts_t* mqtts,
                   int sock,
                   fixed_header_t* fixed_header,
                   void* variable_header,
                   void* payload,
                   int fixed_header_length,
                   uint8_t step)
{
    int i;

    if (mqtts->conns[sock]->package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        message_linked_t* messages;
        connection_linked_t* connections;
        connection_linked_t* _connections = NULL;

        messages = mqtts->conns[sock]->messages;

        while (messages)
        {
            if (messages->message->mid == ((variable_header_puback_t*)variable_header)->message_id)
            {
                connections = messages->message->connections;

                while (connections)
                {
                    if (connections->connection == mqtts->conns[sock])
                    {
                        if (! _connections)
                        {
                            messages->message->connections = NULL;
                            close (messages->message->tfd);
                            hasht_remove(mqtts->messages, messages->message->tfd_str);
                            free (messages->message->tfd_str);
                        }
                        else
                        {
                            _connections->next = connections->next;
                        }
                        free (connections);
                        break;
                    }
                    else
                    {
                        connections = connections->next;
                    }
                }
            }
            messages = messages->next;
        }

        free_fixed_header ((fixed_header_t*)(mqtts->conns[sock]->fixed_header));
        free_variable_header_subscribe ((variable_header_puback_t*)(mqtts->conns[sock]->variable_header));

        mqtts->conns[sock]->mqtt_state = connect_mqtt_state_connected;
        mqtts->conns[sock]->mqtt_state = 0;

        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock]->package_in.pos = fixed_header_length;

        variable_header = malloc (sizeof (variable_header_puback_t));
        memset (variable_header, 0, sizeof (variable_header_puback_t));
        handle_puback(mqtts,
                       sock,
                       fixed_header,
                       variable_header,
                       payload,
                       fixed_header_length, 1);
    }
    if (step == 1)
    {
        /* get message id */

        uint16_t* sub;

        read_mqtt_byte2 (mqtts, sock, &sub);
        ((variable_header_puback_t*)variable_header)->message_id = *sub;

        handle_puback (mqtts,
                        sock,
                        fixed_header,
                        variable_header,
                        payload,
                        fixed_header_length, -1);
    }
}

static int handle_subscribe(mqtts_t* mqtts,
                     int sock,
                     fixed_header_t* fixed_header,
                     void* variable_header,
                     void* payload,
                     int fixed_header_length,
                     uint8_t step)
{
    int i;

    if (mqtts->conns[sock]->package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        topic_t* topic;
        payload_subscribe_topic_t* topics = ((payload_subscribe_t*)payload)->topics;

        mqtts->conns[sock]->fixed_header = fixed_header;
        mqtts->conns[sock]->variable_header = variable_header;
        mqtts->conns[sock]->payload = payload;

        while (topics)
        {
            topic = get_topic (mqtts->topics, topics->topic_name);

            if (! topic)
            {
                insert_topic(mqtts->topics, topics->topic_name, NULL);
                topic = get_topic (mqtts->topics, topics->topic_name);
            }

            if (! insert_subscriber(topic, mqtts->conns[sock]))
            {
                topic_name_t*  _topic_names;
                _topic_names = malloc (sizeof (topic_name_t));
                _topic_names = topics->topic_name;

                if (! mqtts->conns[sock]->topics)
                {
                    mqtts->conns[sock]->topics = _topic_names;
                }
                else
                {
                    _topic_names->next = mqtts->conns[sock]->topics;
                    mqtts->conns[sock]->topics = _topic_names;
                }
            }

            topics = topics->next;
        }

        mqtts->conns[sock]->mqtt_state = connect_mqtt_state_wait_suback;
        mqtts->conns[sock]->state = 1;
        memset (mqtts->conns[sock]->package_in.buf, 0, 2048);
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock]->package_in.pos = fixed_header_length;
        variable_header = malloc (sizeof (variable_header_subscribe_t));
        memset (variable_header, 0, sizeof (variable_header_subscribe_t));
        handle_subscribe (mqtts,
                          sock,
                          fixed_header,
                          variable_header,
                          payload,
                          fixed_header_length, 1);
    }
    if (step == 1)
    {
        /* get message id */
        uint16_t* sub;

        read_mqtt_byte2 (mqtts, sock, &sub);

        ((variable_header_subscribe_t*)variable_header)->message_id = *sub;

        payload = malloc (sizeof (payload_subscribe_t));
        memset (payload, 0, sizeof (payload_subscribe_t));
        handle_subscribe (mqtts,
                          sock,
                          fixed_header,
                          variable_header,
                          payload,
                          fixed_header_length, 2);
    }
    if (step == 2)
    {
        /* get topics */
        payload_subscribe_topic_t* topics = ((payload_subscribe_t*)payload)->topics;

        while (mqtts->conns[sock]->package_in.pos < fixed_header->remaining_length + fixed_header_length)
        {
            uint8_t* sub_topic_name;
            uint8_t* sub_qos;
            uint8_t msb;
            uint8_t lsb;

            read_mqtt_string (mqtts, sock, &sub_topic_name, &msb, &lsb);
            read_mqtt_byte (mqtts, sock, &sub_qos);

            if (! ((payload_subscribe_t*)payload)->topics)
            {
                ((payload_subscribe_t*)payload)->topics = malloc (sizeof (payload_subscribe_topic_t));
                ((payload_subscribe_t*)payload)->topics->topic_name = sub_topic_name;
                ((payload_subscribe_t*)payload)->topics->qos = *sub_qos;
                ((payload_subscribe_t*)payload)->topics->next = NULL;
            }
            else
            {
                topics = ((payload_subscribe_t*)payload)->topics;
                while (topics->next)
                {
                    topics = topics->next;
                }
                topics->next = malloc (sizeof (payload_subscribe_topic_t));
                topics->next->topic_name = sub_topic_name;
                topics->next->qos = *sub_qos;
            }

            free (sub_qos);
        }

        handle_subscribe (mqtts,
                          sock,
                          fixed_header,
                          variable_header,
                          payload,
                          fixed_header_length, -1);
    }
}

static int handle_unsubscribe(mqtts_t* mqtts,
                     int sock,
                     fixed_header_t* fixed_header,
                     void* variable_header,
                     void* payload,
                     int fixed_header_length,
                     uint8_t step)
{
    int i;

    if (mqtts->conns[sock]->package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        topic_t* topic;
        topic_name_t* topics = ((payload_unsubscribe_t*)payload)->topics;

        mqtts->conns[sock]->fixed_header = fixed_header;
        mqtts->conns[sock]->variable_header = variable_header;
        mqtts->conns[sock]->payload = payload;

        while (topics)
        {
            topic = get_topic (mqtts->topics, topics->topic_name);

            if (topic)
            {
                unsubscribe_topic (mqtts->topics, &(mqtts->conns[sock]));
            }

            topics = topics->next;
        }

        mqtts->conns[sock]->mqtt_state = connect_mqtt_state_wait_unsuback;
        mqtts->conns[sock]->state = 1;
        memset (mqtts->conns[sock]->package_in.buf, 0, 2048);
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock]->package_in.pos = fixed_header_length;
        variable_header = malloc (sizeof (variable_header_unsubscribe_t));
        memset (variable_header, 0, sizeof (variable_header_unsubscribe_t));
        handle_unsubscribe (mqtts,
                            sock,
                            fixed_header,
                            variable_header,
                            payload,
                            fixed_header_length, 1);
    }
    if (step == 1)
    {
        /* get message id */
        uint16_t* sub;

        read_mqtt_byte2 (mqtts, sock, &sub);

        ((variable_header_unsubscribe_t*)variable_header)->message_id = *sub;

        payload = malloc (sizeof (payload_unsubscribe_t));
        memset (payload, 0, sizeof (payload_unsubscribe_t));
        handle_unsubscribe (mqtts,
                          sock,
                          fixed_header,
                          variable_header,
                          payload,
                          fixed_header_length, 2);
    }
    if (step == 2)
    {
        /* get topics */
        topic_name_t* topics = ((payload_unsubscribe_t*)payload)->topics;

        while (mqtts->conns[sock]->package_in.pos < fixed_header->remaining_length + fixed_header_length)
        {
            uint8_t* sub_topic_name;
            uint8_t msb;
            uint8_t lsb;

            read_mqtt_string (mqtts, sock, &sub_topic_name, &msb, &lsb);

            if (! ((payload_unsubscribe_t*)payload)->topics)
            {
                ((payload_unsubscribe_t*)payload)->topics = malloc (sizeof (payload_subscribe_topic_t));
                ((payload_unsubscribe_t*)payload)->topics->topic_name = sub_topic_name;
                ((payload_unsubscribe_t*)payload)->topics->next = NULL;
            }
            else
            {
                topics = ((payload_unsubscribe_t*)payload)->topics;
                while (topics->next)
                {
                    topics = topics->next;
                }
                topics->next = malloc (sizeof (topic_name_t));
                topics->next->topic_name = sub_topic_name;
            }
        }

        handle_unsubscribe (mqtts,
                          sock,
                          fixed_header,
                          variable_header,
                          payload,
                          fixed_header_length, -1);
    }
}

static int handle_pingreq(mqtts_t* mqtts,
                   int sock,
                   fixed_header_t* fixed_header,
                   void* variable_header,
                   void* payload,
                   int fixed_header_length,
                   uint8_t step)
{
    mqtts->conns[sock]->fixed_header = fixed_header;
    mqtts->conns[sock]->mqtt_state = connect_mqtt_state_wait_pingresp;
    mqtts->conns[sock]->state = 1;
    /* free (mqtts->conns[sock]->package_in.buf); */
    /* mqtts->conns[sock]->package_in.buf = malloc (2048); */
    memset (mqtts->conns[sock]->package_in.buf, 0, 2048);
    return 0;
}

static int handle_unknown(mqtts_t* mqtts,
                   int sock,
                   fixed_header_t* fixed_header,
                   void* variable_header,
                   void* payload,
                   int fixed_header_length,
                   uint8_t step)
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
    if ((command & 0xF0) == 0x30)
    {
        return handle_publish;
    }
    if ((command & 0xF0) == 0x40)
    {
        return handle_puback;
    }
    if ((command & 0xF0) == 0x80)
    {
        return handle_subscribe;
    }
    if ((command & 0xF0) == 0xa0)
    {
        return handle_unsubscribe;
    }
    if ((command & 0xF0) == 0xc0)
    {
        return handle_pingreq;
    }
    else
    {
        return handle_unknown;
    }
}


int handle(mqtts_t* mqtts, int sock)
{

    uint8_t byte1;
    long int remaining_length;
    int fixed_header_length;
    uint8_t* buf;
    uint8_t digit;
    long int multiplier = 1;
    int count = 1;

    buf = mqtts->conns[sock]->package_in.buf;
    byte1 = buf[0];

    remaining_length = 0;

    do
    {
        digit = buf[count++];
        remaining_length += (digit & 127) * multiplier;
        multiplier *= 128;
    } while ((digit & 128) != 0);

    fixed_header_length =  count;

    fixed_header_t* fixed_header = malloc (sizeof (fixed_header_t));

    fixed_header->message_type = (byte1 >> 4) & 0x0F;
    fixed_header->dup = (byte1 >> 3) & 0x08;
    fixed_header->qos = (byte1 >> 1) & 0x03;
    fixed_header->retain = byte1 & 0x01;
    fixed_header->remaining_length = remaining_length;
    printf ("remaining_length: %d\n", remaining_length);
    printf ("remaining_length: %d\n", fixed_header_length);
    get_handler(byte1)(mqtts,
                       sock,
                       fixed_header,
                       NULL, NULL,
                       fixed_header_length, 0);
}

int client_handle(mqtts_t* mqtts, int sock)
{

    switch (mqtts->conns[sock]->mqtt_state)
    {
        case connect_mqtt_state_wait_connack:
            return handle_connack (mqtts, sock);
        case connect_mqtt_state_wait_puback:
            return handle_puback_client (mqtts, sock);
        case connect_mqtt_state_wait_suback:
            return handle_suback (mqtts, sock);
        case connect_mqtt_state_wait_unsuback:
            return handle_unsuback (mqtts, sock);
        case connect_mqtt_state_wait_pingresp:
            return handle_pingresp (mqtts, sock);
        case connect_mqtt_state_wait_publish:
            return handle_to_publish (mqtts, sock);
    }
}

int callback_handle(mqtts_t* mqtts, int sock)
{
    uint8_t tfd_str[6];
    message_t* message;
    connection_linked_t* connections;
    int tfd = sock;

    sprintf (tfd_str, "%d", tfd);
    printf ("%s\n", tfd_str);
    message = hasht_get(mqtts->messages, tfd_str);

    printf ("%s\n", "fuckererererer");

    if (! message)
    {
        printf ("%s\n", "error fuck");
        return 1;
    }
    else
    {
        connections = message->connections;

        if (connections)
        {
            struct itimerspec newValue;
            struct itimerspec oldValue;

            bzero (&newValue,sizeof(newValue));
            bzero (&oldValue,sizeof(oldValue));

            struct timespec ts;

            ts.tv_sec = 2;
            ts.tv_nsec = 0;
            newValue.it_value = ts;
            newValue.it_interval = ts;

            if ( timerfd_settime (tfd, 0, &newValue, &oldValue) < 0)
            {
                printf ("settime error\n");
                abort ();
            }
        }

        while (connections)
        {
            printf ("%s\n", "daadaa");
            printf ("%s\n", message->data);
            send(connections->connection->fd, message->data, message->data_len, 0);
            connections = connections->next;
        }
    }
}
/* publish */
/* pubrel */
/* subscribe */
/* unsubscribe */
