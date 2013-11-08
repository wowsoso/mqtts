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

    *msb = mqtts->conns[sock].package_in.buf[
        mqtts->conns[sock].package_in.pos];
    *lsb = mqtts->conns[sock].package_in.buf[
        mqtts->conns[sock].package_in.pos + 1];

    str_len = (*msb << 8) + *lsb;

    *sub = malloc (str_len * sizeof (uint8_t) + 1);
    memset (*sub, 0, str_len * sizeof (uint8_t) + 1);

    strncpy (*sub,
             mqtts->conns[sock].package_in.buf +
             mqtts->conns[sock].package_in.pos + 2,
             str_len);

    mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos
        + 2 + str_len;
}

static void read_mqtt_byte(mqtts_t* mqtts,
                           int sock,
                           uint8_t** sub)
{
    *sub = malloc(sizeof(uint8_t) + 1);
    memset (*sub, 0, sizeof (uint8_t) + 1);

    strncpy (*sub,
             mqtts->conns[sock].package_in.buf +
             mqtts->conns[sock].package_in.pos, 1);

    mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos + 1;
}

static void read_mqtt_byte2(mqtts_t* mqtts,
                            int sock,
                            uint16_t** sub)
{
    uint8_t msb, lsb;
    uint16_t str_len;

    msb = mqtts->conns[sock].package_in.buf[
        mqtts->conns[sock].package_in.pos];
    lsb = mqtts->conns[sock].package_in.buf[
        mqtts->conns[sock].package_in.pos + 1];

    str_len = (msb << 8) + lsb;

    *sub = malloc (sizeof (uint16_t));
    memset (*sub, 0, sizeof (uint16_t) + 1);

    strncpy (*sub, &str_len, 2);

    mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos + 2;
}

static int publish_to_client(subscriber_t* subscribers,
                      fixed_header_t* fixed_header,
                      variable_header_publish_t* variable_header,
                      payload_publish_t* payload)
{
    uint8_t* a;
    int payload_len;
    int remaining_len;
    int bit_count;
    int digit;
    int pos;
    int i;

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

    encode_remaining_length(remaining_len, &digit, &bit_count);

    a = malloc (1 + bit_count + remaining_len);
    memset (a, 0, 1 + bit_count + remaining_len);

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

    if (fixed_header->qos > 0) {
        memcpy (a + pos, &(variable_header->message_id), 2);
        pos = pos + 2;
    }

    memcpy (a+pos, payload->data, payload_len);

    while (subscribers)
    {
        printf ("%s\n", "shit");
        send(((connection_t*)subscribers->connection)->fd,
             a,
             1 + bit_count + remaining_len, 0);
        subscribers = subscribers->next;
    }

    free (a);
}

int handle_connack(mqtts_t* mqtts, int sock)
{
    char a[4];
    a[0] = 32;
    a[1] = 2;
    a[2] = 0;
    a[3] = MQTTS_CONNACK_Accepted;

    send(mqtts->conns[sock].fd, a, 4, 0);
    mqtts->conns[sock].mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock].state = 0;

    free_fixed_header ((fixed_header_t*)(mqtts->conns[sock].fixed_header));
    free_variable_header_connect ((variable_header_connect_t*)(mqtts->conns[sock].variable_header));
    free_payload_connect ((payload_connect_t*)(mqtts->conns[sock].payload));
}

static int handle_puback(mqtts_t* mqtts, int sock)
{
    uint8_t a[4];
    int i;
    topic_t* topics;
    subscriber_t* subscribers;

    a[0] = 64; /* 0100 0000 */
    a[1] = 2;  /* 0000 0010 */
    a[2] = ((variable_header_publish_t*)(mqtts->conns[sock].variable_header))->message_id >> 8;   /*  msb  */
    a[3] = ((variable_header_publish_t*)(mqtts->conns[sock].variable_header))->message_id;        /*  lsb  */
    /* send(mqtts->conns[sock].fd, a, 4, 0); */
    mqtts->conns[sock].mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock].state = 0;
    topics = get_topic (mqtts->topics,
                        ((variable_header_publish_t*)(mqtts->conns[sock].variable_header))->topic_name);
    printf ("%s\n",
            ((variable_header_publish_t*)(mqtts->conns[sock].variable_header))->topic_name);
    subscribers = topics->subscribers;

    publish_to_client (subscribers,
                       mqtts->conns[sock].fixed_header,
                       ((variable_header_publish_t*)(mqtts->conns[sock].variable_header)),
                       ((payload_publish_t*)(mqtts->conns[sock].payload)));

    free_fixed_header ((fixed_header_t*)(mqtts->conns[sock].fixed_header));
    free_variable_header_publish ((variable_header_publish_t*)(mqtts->conns[sock].variable_header));
    free_payload_publish ((payload_publish_t*)(mqtts->conns[sock].payload));
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

    topics = ((payload_subscribe_t*)(mqtts->conns[sock].payload))->topics;

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

    topics = ((payload_subscribe_t*)(mqtts->conns[sock].payload))->topics;

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


    a[pos++] = ((variable_header_subscribe_t*)(mqtts->conns[sock].variable_header))->message_id >> 8;   /*  msb  */
    a[pos++] = ((variable_header_subscribe_t*)(mqtts->conns[sock].variable_header))->message_id;        /*  lsb  */

    while (topics)
    {
        a[pos++] = topics->qos;
        topics = topics->next;
    }

    send(mqtts->conns[sock].fd, a, 1 + bit_count + remaining_len, 0);
    mqtts->conns[sock].mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock].state = 0;

    free (a);
    free_fixed_header ((fixed_header_t*)(mqtts->conns[sock].fixed_header));
    free_variable_header_subscribe ((variable_header_subscribe_t*)(mqtts->conns[sock].variable_header));
    free_payload_subscribe ((payload_subscribe_t*)(mqtts->conns[sock].payload));
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

    if (mqtts->conns[sock].package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        mqtts->conns[sock].fixed_header = fixed_header;
        mqtts->conns[sock].variable_header = variable_header;
        mqtts->conns[sock].payload = payload;
        mqtts->conns[sock].mqtt_state = connect_mqtt_state_wait_connack;
        mqtts->conns[sock].state = 1;
        memset (mqtts->conns[sock].package_in.buf, 0, 2048);
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock].package_in.pos = fixed_header_length;
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

    if (mqtts->conns[sock].package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        mqtts->conns[sock].fixed_header = fixed_header;
        mqtts->conns[sock].variable_header = variable_header;
        mqtts->conns[sock].payload = payload;

        if (fixed_header->qos == 0) {
            /* FIXME return 0 directly */
            mqtts->conns[sock].mqtt_state = connect_mqtt_state_wait_puback;
        }

        if (fixed_header->qos == 1) {
            mqtts->conns[sock].mqtt_state = connect_mqtt_state_wait_puback;
        }

        int ss = 0;
        ss = insert_topic (mqtts->topics,
                          ((variable_header_publish_t*)(mqtts->conns[sock].variable_header))->topic_name,
                          ((payload_publish_t*)(mqtts->conns[sock].payload))->data);

        mqtts->conns[sock].state = 1;
        memset (mqtts->conns[sock].package_in.buf, 0, 2048);
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock].package_in.pos = fixed_header_length;

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

        sub = malloc (fixed_header->remaining_length - mqtts->conns[sock].package_in.pos + fixed_header_length);
        memset (sub, 0, fixed_header->remaining_length - mqtts->conns[sock].package_in.pos + fixed_header_length);
        strncpy(sub,
                mqtts->conns[sock].package_in.buf + mqtts->conns[sock].package_in.pos,
                fixed_header->remaining_length - mqtts->conns[sock].package_in.pos + fixed_header_length);
        mqtts->conns[sock].package_in.pos = fixed_header->remaining_length + fixed_header_length;

        ((payload_publish_t*)payload)->data = sub;
        handle_publish (mqtts,
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

    if (mqtts->conns[sock].package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        topic_t* topic;
        payload_subscribe_topic_t* topics = ((payload_subscribe_t*)payload)->topics;
        char* str_fd;

        mqtts->conns[sock].fixed_header = fixed_header;
        mqtts->conns[sock].variable_header = variable_header;
        mqtts->conns[sock].payload = payload;

        str_fd = malloc (6);

        while (topics)
        {
            topic = get_topic (mqtts->topics, topics->topic_name);

            if (! topic)
            {
                insert_topic(mqtts->topics, topics->topic_name, NULL);
                topic = get_topic (mqtts->topics, topics->topic_name);
            }
            printf ("%s\n", "hei");
            insert_subscriber(topic, &(mqtts->conns[sock]));
            topics = topics->next;
        }

        mqtts->conns[sock].mqtt_state = connect_mqtt_state_wait_suback;
        mqtts->conns[sock].state = 1;
        memset (mqtts->conns[sock].package_in.buf, 0, 2048);
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock].package_in.pos = fixed_header_length;
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

        while (mqtts->conns[sock].package_in.pos < fixed_header->remaining_length + fixed_header_length)
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
    if ((command & 0xF0) == 0x80)
    {
        return handle_subscribe;
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

    buf = mqtts->conns[sock].package_in.buf;
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

    switch (mqtts->conns[sock].mqtt_state)
    {
        case connect_mqtt_state_wait_connack:
            return handle_connack (mqtts, sock);
        case connect_mqtt_state_wait_puback:
            return handle_puback (mqtts, sock);
        case connect_mqtt_state_wait_suback:
            return handle_suback (mqtts, sock);
    }
}
