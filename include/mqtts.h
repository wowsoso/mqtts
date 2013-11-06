#ifndef MQTTS_H
#define MQTTS_H

#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include "topic.h"

#define IMPORTANT_MACRO 1

/* max event count */
#define MQTTS_MAXEVENTS 20000

#define MQTTS_CONNACK_Accepted                      0x00
#define MQTTS_CONNACK_REFUSED_PROTOCOL_VERSION      0x01
#define MQTTS_CONNACK_REFUSED_IDENTIFIER_REJECTED   0x02
#define MQTTS_CONNACK_REFUSED_SERVER_UNAVAILABLE    0x03
#define MQTTS_CONNACK_REFUSED_BAD_USERNAME_PASSWORD 0x04
#define MQTTS_CONNACK_REFUSED_NOT_AUTHORIZED        0x05

enum connect_conn_state_e
{
    connect_conn_state_connectless = 0,
    connect_conn_state_connected = 1,
};

enum connect_mqtt_state_e
{
    connect_mqtt_state_wait_connect  = -2,
    connect_mqtt_state_connected     = -1,
    connect_mqtt_state_invalid       = 0,
    connect_mqtt_state_wait_connack  = 1,
    connect_mqtt_state_wait_puback   = 2,
    connect_mqtt_state_wait_pubrec   = 3,
    connect_mqtt_state_wait_pubrel   = 4,
    connect_mqtt_state_wait_pubcomp  = 5,
    connect_mqtt_state_wait_suback   = 6,
};

typedef struct string_s
{
    uint8_t  msb;
    uint8_t  lsb;
    uint8_t* str;
} string_t;

typedef struct fixed_header_s
{
    uint8_t             message_type : 4;
    uint8_t             dup          : 1;
    uint8_t             qos          : 2;
    uint8_t             retain       : 1;
    uint32_t            remaining_length;
} fixed_header_t;

typedef struct variable_header_connect_s
{
    uint8_t* protocol_name;
    uint8_t  protocol_version_number;

    /* connect flags*/
    uint8_t  username_flag :1;
    uint8_t  password_flag :1;
    uint8_t  will_retain   :1;
    uint16_t will_qos      :2;
    uint8_t  will_flag     :1;
    uint8_t  clean_session :1;
    uint8_t  reserved      :1;
    uint16_t  keep_alive_timer;
} variable_header_connect_t;

typedef struct variable_header_publish_s
{
    uint8_t* topic_name;
    uint16_t message_id;
} variable_header_publish_t;

typedef struct variable_header_subscribe_s
{
    uint16_t message_id;
} variable_header_subscribe_t;


typedef struct payload_connect_s
{
    string_t client_id;
    string_t will_topic;
    string_t will_message;
    string_t username;
    string_t password;
} payload_connect_t;

typedef struct payload_publish_s
{
    uint8_t* data;
} payload_publish_t;

typedef struct payload_subscribe_topic_s
{
    uint8_t*                          topic_name;
    uint8_t                           qos;
    struct payload_subscribe_topic_s* next;
} payload_subscribe_topic_t;

typedef struct payload_subscribe_s
{
    payload_subscribe_topic_t* topics;
} payload_subscribe_t;

typedef struct package_s
{
    uint8_t*    buf;
    size_t      pos;
} package_t;

typedef struct connection_s
{
    size_t                    fd;
    package_t                 package_in;
    package_t                 package_out;
    enum connect_conn_state_e conn_state;
    enum connect_mqtt_state_e mqtt_state;
    fixed_header_t*           fixed_header;
    void*                     variable_header;
    void*                     payload;
    bool                      state;   /* reactor read or write */
} connection_t;

typedef struct mqtts_s
{
    connection_t conns[MQTTS_MAXEVENTS];
    struct       available_connections_index_s
    {
        struct   available_event_index_s* next;
        size_t   index;
    }*           available_conns_index;
    topic_t*    topics;

} mqtts_t;

mqtts_t* init_mqtts(mqtts_t* mqtts);

#endif /* MQTTS_H */
