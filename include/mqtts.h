#ifndef MQTTS_H
#define MQTTS_H

#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#define IMPORTANT_MACRO 1

/* max event count */
#define MQTTS_MAXEVENTS 20000

typedef struct string_s
{
    uint16_t length;
    uint8_t* text;
} string_t;


typedef struct fixed_header_s
{
    uint8_t  message_type      : 4;
    uint8_t  dup_flag          : 1;
    uint8_t  qos_level         : 2;
    uint8_t  retain            : 1;
    uint32_t remainning_length;
} fixed_header_t;

typedef struct variable_header_connect_s
{
    string_t protocol_name;
    uint8_t  protocol_version;
    uint8_t  username_flag   : 1;
    uint8_t  password_flag   : 1;
    uint8_t  will_retain     : 1;
    uint8_t  will_qos        : 2;
    uint8_t  will_flag       : 1;
    uint8_t  clean_session   : 1;
    uint8_t  reserved        : 1;
} variable_header_connect_t;


typedef struct payload_connect_s
{
    uint16_t client_id;
    string_t will_topic;
    string_t will_message;
    string_t username;
    string_t password;
} payload_connect_t;


typedef struct package_buf_s
{
    size_t pos;
    uint8_t* package;
} package_buf_t;


enum connect_conn_state_e {
    connect_conn_state_connectless = 0,
    connect_conn_state_connected = 1,
};

enum connect_mqtt_state_e {
    connect_mqtt_state_wait_connect  = -2,
    connect_mqtt_state_connected     = -1,
    connect_mqtt_state_invalid       = 0,
    connect_mqtt_state_wait_puback   = 1,
    connect_mqtt_state_wait_pubrec   = 2,
    connect_mqtt_state_wait_pubrel   = 3,
    connect_mqtt_state_wait_pubcomp  = 4,
};

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
} connection_t;


typedef struct mqtts_s
{
    connection_t conns[MQTTS_MAXEVENTS];
    struct available_connections_index_s
    {
        struct available_event_index_s* next;
        size_t index;
    }* available_conns_index;
} mqtts_t;

mqtts_t* init_mqtts(mqtts_t* mqtts);

/* enum variable_header_type */
/* { */
/*     "variable_header_connect_t, */
/* } */
/* p */
/* enum payload_type */
/* { */
/*     payload_connect_t; */
/* } */

/* struct mqtts_message */
/* { */
/*     fixed_header_t fixed_header; */
/*     union { */
/*         variable_header_connect_t variable_header_connect; */
/*     }; */
/*     union { */
/*         payload_connect_t payload_connect; */
/*     } */
/* }; */


#endif /* MQTTS_H */
