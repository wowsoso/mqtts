#include<stdlib.h>
#include <inttypes.h>
#include "hasht.h"

#define TOPIC_HASH_CONNECTION_INCREMENT 256

typedef struct topic_message_s
{
    uint8_t*                message;
    struct topic_message_s* next;
} topic_message_t;

typedef struct subscriber_s
{
    uint8_t              qos;
    void*                connection;  /* connection_t */
    struct subscriber_s* next;
} subscriber_t;

typedef struct topic_s
{
    uint8_t*         name;
    struct topic_s*  sub;
    struct topic_s*  next;
    size_t           sub_size;
    topic_message_t* topic_message;
    subscriber_t*    subscribers;
} topic_t;


topic_t* make_topic(uint8_t* topic_str, uint8_t* message);
topic_t* get_topic(topic_t* topics, uint8_t* topic_str);
int insert_subscriber(topic_t* topic, void* connection);
int unsubscribe_topic(topic_t* topics, void* connection);
