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


static void read_mqtt_string(mqtts_t* mqtts, int sock, uint8_t** sub)
{
    uint8_t msb, lsb;
    uint16_t str_len;

    msb = mqtts->conns[sock].package_in.buf[mqtts->conns[sock].package_in.pos];
    lsb = mqtts->conns[sock].package_in.buf[mqtts->conns[sock].package_in.pos + 1];
    str_len = (msb << 8) + lsb;
    *sub = malloc (str_len * sizeof (uint8_t) + 1);
    memset (*sub, 0, str_len * sizeof (uint8_t) + 1);
    strncpy (*sub, mqtts->conns[sock].package_in.buf + mqtts->conns[sock].package_in.pos + 2, str_len);
    mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos + 2 + str_len;
}

static void read_mqtt_byte(mqtts_t* mqtts, int sock, uint8_t** sub)
{
    *sub = malloc(sizeof(uint8_t) + 1);
    memset (*sub, 0, sizeof (uint8_t) + 1);
    strncpy (*sub, mqtts->conns[sock].package_in.buf + mqtts->conns[sock].package_in.pos, 1);
    mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos + 1;
}

static void read_mqtt_byte2(mqtts_t* mqtts, int sock, uint16_t** sub)
{
    uint8_t msb, lsb;
    uint16_t str_len;

    msb = mqtts->conns[sock].package_in.buf[mqtts->conns[sock].package_in.pos];
    lsb = mqtts->conns[sock].package_in.buf[mqtts->conns[sock].package_in.pos + 1];
    str_len = (msb << 8) + lsb;
    *sub = malloc(sizeof(uint16_t));
    memset (*sub, 0, sizeof (uint16_t) + 1);
    strncpy (*sub, &str_len, 2);
    mqtts->conns[sock].package_in.pos = mqtts->conns[sock].package_in.pos + 2;
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
}

int handle_puback(mqtts_t* mqtts, int sock)
{
    uint8_t a[4];
    uint8_t msb;
    uint8_t lsb;

    a[0] = 64; /* 0100 0000 */
    a[1] = 2;  /* 0000 0010 */
    a[2] = ((variable_header_publish_t*)(mqtts->conns[sock].variable_header))->message_id >> 8;   /*  msb  */
    a[3] = ((variable_header_publish_t*)(mqtts->conns[sock].variable_header))->message_id;        /*  lsb  */
    send(mqtts->conns[sock].fd, a, 4, 0);
    mqtts->conns[sock].mqtt_state = connect_mqtt_state_connected;
    mqtts->conns[sock].state = 0;
}


int handle_connect(mqtts_t* mqtts, int sock, fixed_header_t* fixed_header, void* variable_header, void* payload, int fixed_header_length, uint8_t step)
{
    int i;

    if (mqtts->conns[sock].package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        mqtts->conns[sock].fixed_header = fixed_header;
        mqtts->conns[sock].variable_header = variable_header;
        mqtts->conns[sock].payload = payload;
        mqtts->conns[sock].mqtt_state = connect_mqtt_state_wait_connack;
        mqtts->conns[sock].state = 1;
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock].package_in.pos = fixed_header_length;
        variable_header = malloc (sizeof (variable_header_connect_t));
        memset (variable_header, 0, sizeof (variable_header_connect_t));
        handle_connect (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 1);
    }

    if (step == 1)
    {
        /* get protocol name */
        uint8_t* sub;
        read_mqtt_string (mqtts, sock, &sub);
        ((variable_header_connect_t*)variable_header)->protocol_name = sub;
        handle_connect (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 2);
    }

    if (step == 2)
    {
        /* get protocol version number */
        uint8_t* sub;
        read_mqtt_byte (mqtts, sock, &sub);
        ((variable_header_connect_t*)variable_header)->protocol_version_number = sub;
        handle_connect (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 3);
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

        handle_connect (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 4);
    }

    if (step == 4)
    {
        /* get keep alive timer */
        uint16_t* sub;

        read_mqtt_byte2 (mqtts, sock, &sub);
        ((variable_header_connect_t*)variable_header)->keep_alive_timer = *sub;
        handle_connect(mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 5);
    }

    if (step == 5)
    {
        /* get payload */
        uint8_t* sub;

        read_mqtt_string (mqtts, sock, &sub);
        handle_connect(mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, -1);
        free (sub);
    }

    return -1;
}

int handle_publish(mqtts_t* mqtts, int sock, fixed_header_t* fixed_header, void* variable_header, void* payload, int fixed_header_length, uint8_t step)
{
    int i;

    if (mqtts->conns[sock].package_in.pos == fixed_header->remaining_length + fixed_header_length)
    {
        mqtts->conns[sock].fixed_header = fixed_header;
        mqtts->conns[sock].variable_header = variable_header;
        mqtts->conns[sock].payload = payload;

        if (fixed_header->qos == 0) {
            return 0;
        }

        if (fixed_header->qos == 1) {
            mqtts->conns[sock].mqtt_state = connect_mqtt_state_wait_puback;
        }

        mqtts->topics = make_topic(((variable_header_publish_t*)(mqtts->conns[sock].variable_header))->topic_name, ((payload_publish_t*)(mqtts->conns[sock].payload))->data);
        mqtts->conns[sock].state = 1;

        printf ("%s\n", mqtts->topics->name);
        printf ("%s\n", mqtts->topics->sub->name);
        printf ("%s\n", mqtts->topics->sub->topic_message->message);
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock].package_in.pos = fixed_header_length;
        variable_header = malloc (sizeof (variable_header_publish_t));
        memset (variable_header, 0, sizeof (variable_header_publish_t));
        handle_publish(mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 1);
    }
    if (step == 1)
    {
        /* get topic name */
        uint8_t* sub;

        read_mqtt_string (mqtts, sock, &sub);
        ((variable_header_publish_t*)variable_header)->topic_name = sub;
        handle_publish (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 2);
    }
    if (step == 2)
    {
        /* get message id */
        uint16_t* sub;

        read_mqtt_byte2 (mqtts, sock, &sub);

        ((variable_header_publish_t*)variable_header)->message_id = *sub;
        payload = malloc (sizeof (payload_publish_t));
        memset (payload, 0, sizeof (payload_publish_t));
        handle_publish (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 3);
    }
    if (step == 3)
    {
        /* get payload */
        uint8_t* sub;

        sub = malloc (fixed_header->remaining_length - mqtts->conns[sock].package_in.pos + 3);
        memset (sub, 0, fixed_header->remaining_length - mqtts->conns[sock].package_in.pos + 3);
        strncpy(sub, mqtts->conns[sock].package_in.buf + mqtts->conns[sock].package_in.pos, fixed_header->remaining_length - mqtts->conns[sock].package_in.pos + fixed_header_length);
        mqtts->conns[sock].package_in.pos = fixed_header->remaining_length + fixed_header_length;

        ((payload_publish_t*)payload)->data = sub;
        handle_publish (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, -1);
    }

}

int handle_subscribe(mqtts_t* mqtts, int sock, fixed_header_t* fixed_header, void* variable_header, void* payload, int fixed_header_length, uint8_t step)
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
        sprintf(str_fd, "%d", sock);
        while (topics)
        {
            topic = get_topic (mqtts->topics, topics->topic_name);

            if (! topic)
            {
                /* FIXME  maybe to send a error msg to client??? */
                return -1;
            }
            topic->subscribers = hasht_create (TOPIC_HASH_CONNECTION_INCREMENT);
            printf ("%s\n", "hehh");
            if(!(topic->subscribers = hasht_create (TOPIC_HASH_CONNECTION_INCREMENT))) {
                fprintf(stderr, "ERROR: hasht_create() failed\n");
                /* FIXME: do something */
                abort ();
            }

            hasht_insert (topic->subscribers, str_fd, &(topics->qos));
            topics = topics->next;
        }

        mqtts->conns[sock].mqtt_state = connect_mqtt_state_wait_suback;

        mqtts->conns[sock].state = 1;
        return 0;
    }

    if (step == 0)
    {
        mqtts->conns[sock].package_in.pos = fixed_header_length;
        variable_header = malloc (sizeof (variable_header_subscribe_t));
        memset (variable_header, 0, sizeof (variable_header_subscribe_t));
        handle_subscribe (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 1);
    }
    if (step == 1)
    {
        /* get message id */
        uint16_t* sub;

        read_mqtt_byte2 (mqtts, sock, &sub);

        ((variable_header_publish_t*)variable_header)->message_id = *sub;

        payload = malloc (sizeof (payload_subscribe_t));
        memset (payload, 0, sizeof (payload_subscribe_t));
        handle_subscribe (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, 2);
    }
    if (step == 2)
    {
        /* get topics */
        payload_subscribe_topic_t* topics = ((payload_subscribe_t*)payload)->topics;

        while (mqtts->conns[sock].package_in.pos < fixed_header->remaining_length + fixed_header_length)
        {
            uint8_t* sub_topic_name;
            uint8_t* sub_qos;

            read_mqtt_string (mqtts, sock, &sub_topic_name);
            read_mqtt_byte (mqtts, sock, &sub_qos);

            topics = malloc (sizeof (payload_subscribe_topic_t));
            topics->topic_name = sub_topic_name;
            topics->qos = *sub_qos;
            free (sub_qos);
            printf ("---%s\n", topics->topic_name);
            printf ("---%d\n", topics->qos);
            printf ("----%d\n", mqtts->conns[sock].package_in.pos);
            printf ("----%d\n", fixed_header->remaining_length + fixed_header_length);
        }
        ((payload_subscribe_t*)payload)->topics = topics;
        handle_subscribe (mqtts, sock, fixed_header, variable_header, payload, fixed_header_length, -1);
    }
}

int handle_unknown(mqtts_t* mqtts, int sock, fixed_header_t* fixed_header, void* variable_header, void* payload, int fixed_header_length, uint8_t step)
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

    buf = mqtts->conns[sock].package_in.buf;
    byte1 = buf[0];

    long int multiplier = 1;
    int count = 1;
    remaining_length = 0;

    uint8_t digit;

    do
    {
        digit = buf[count++];
        remaining_length += (digit & 127) * multiplier;
        multiplier *= 128;
    } while ((digit & 128) != 0);

    fixed_header_length =  count;

    fixed_header_t fixed_header;
    fixed_header.message_type = (byte1 >> 4) & 0x0F;
    fixed_header.dup = (byte1 >> 3) & 0x08;
    fixed_header.qos = (byte1 >> 1) & 0x03;
    fixed_header.retain = byte1 & 0x01;
    fixed_header.remaining_length = remaining_length;
    printf ("remaining_length: %d\n", remaining_length);
    printf ("remaining_length: %d\n", fixed_header_length);
    get_handler(byte1)(mqtts, sock, &fixed_header, NULL, NULL, fixed_header_length, 0);
}


int client_handle(mqtts_t* mqtts, int sock)
{

    switch (mqtts->conns[sock].mqtt_state)
    {
        case connect_mqtt_state_wait_connack:
            return handle_connack (mqtts, sock);
        case connect_mqtt_state_wait_puback:
            return handle_puback (mqtts, sock);
    }
}
