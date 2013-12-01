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

#include "mqtts.h"

mqtts_t* init_mqtts(mqtts_t* mqtts)
{

    mqtts = malloc(sizeof (mqtts_t));
    mqtts->available_conns_index = NULL;
    mqtts->topics =  malloc (sizeof (topic_t));
    mqtts->topics->name = "#";
    mqtts->topics->sub_size = 0;
    mqtts->topics->sub = NULL;
    mqtts->topics->next = NULL;
    mqtts->topics->topic_message = NULL;
    mqtts->messages = hasht_create(1000);
    mqtts->last_mid = 0;

    return mqtts;
}

message_t* insert_message(mqtts_t* mqtts, uint8_t* data, uint16_t mid, uint32_t data_len, subscriber_t* subscribers, int tfd)
{
    uint8_t* tfd_str = malloc (6);
    uint8_t* _data;
    connection_linked_t* connections = malloc (sizeof (connection_linked_t));
    message_t* message;
    message_linked_t* messages;

    memset (tfd_str, 0, 6);
    _data = malloc (data_len);
    memcpy (_data, data, data_len);
    _data[0] = _data[0] | 0x08;

    sprintf (tfd_str, "%d", tfd);

    message = malloc (sizeof (message_t));
    memset (message, 0, sizeof (message_t));
    messages = malloc (sizeof (message_linked_t));
    memset (messages, 0, sizeof (message_linked_t));
    messages->message = message;

    while (subscribers)
    {
        connections->connection = subscribers->connection;
        messages->next = connections->connection->messages;
        connections->connection->messages = messages;
        subscribers = subscribers->next;

        if (subscribers)
        {
            connections->next = malloc (sizeof (connection_linked_t));
            connections = connections->next;
        }
    }

    message->connections = connections;
    message->data = _data;
    message->mid = mid;
    message->data_len = data_len;
    message->tfd = tfd;
    message->tfd_str = tfd_str;

    hasht_insert(mqtts->messages, tfd_str, message);
    printf ("%xbbb\n", message->data);
    return message;
}

void free_fixed_header(fixed_header_t* fixed_header)
{
    free (fixed_header);
}

void free_variable_header_connect(variable_header_connect_t* variable_header)
{
    free (variable_header->protocol_name);
    free (variable_header);
}

void free_payload_connect(payload_connect_t* payload)
{
    free (payload);
}

void free_variable_header_publish(variable_header_publish_t* variable_header)
{
    free (variable_header);
}

void free_payload_publish(payload_publish_t* payload)
{
    free (payload->data);
    free (payload);
}

void free_variable_header_subscribe(variable_header_subscribe_t* variable_header)
{
    free (variable_header);
}

void free_payload_subscribe(payload_subscribe_t* payload)
{
    payload_subscribe_topic_t* topics;

    while (payload->topics)
    {
        topics = payload->topics;

        if (! topics->next)
        {
            free (topics);
            break;
        }
        else
        {
            payload->topics = topics->next;
            free (topics);
        }
    }

    free (payload);
}
