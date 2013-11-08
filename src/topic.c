#include "topic.h"

int set_topic(topic_t* topics, topic_t* topic)
{

}


int insert_topic(topic_t* topics, uint8_t* topic_str, uint8_t* message)
{
    uint8_t* buf;
    uint8_t* saveptr;
    topic_t* topic;
    topic_t* _topic;
    topic_t* _topics = topics;
    topic_t* __topics;
    uint8_t* _buf;

    char* delim = "/";

    if (! topics)
    {
        return -1;
    }

    if (strlen (topic_str))
    {
        _buf = malloc(strlen (topic_str) + 1);
        memset (_buf, 0, strlen (topic_str) + 1);
        strcpy (_buf, topic_str);
        buf = strtok_r (_buf, delim, &saveptr);
    }
    else
    {
        return -1;
    }

    while (_topics)
    {
        __topics = _topics;

        if (! strcmp (_topics->name, buf))
        {
            if (! strlen (saveptr))
            {
                /* topics->name = make_topic (saveptr, message); */
                /* topics->sub_size++; */

                while (_topics->topic_message)
                {
                    if (_topics->topic_message->next)
                    {
                        _topics->topic_message = _topics->topic_message->next;
                    }

                    break;
                }

                if (message)
                {
                    if (! _topics->topic_message)
                    {
                        _topic->topic_message = malloc (sizeof (topic_message_t));
                    }

                    _topics->topic_message->message = message;
                    _topics->topic_message->next = NULL;
                }
                free (_buf);
                return 1;
            }
            else {
                if (strlen (saveptr))
                {
                    buf = strtok_r (saveptr, delim, &saveptr);
                }
                else
                {
                    free (_buf);
                    return -1;
                }

                _topics = _topics->sub;
            }
        }
        else
        {
            _topics = _topics->next;
        }
    }

    __topics->next = make_topic (_buf, message);
    free (_buf);
    return 0;
}


topic_t* make_topic(uint8_t* topic_str, uint8_t* message)
{
    uint8_t* buf;
    uint8_t* saveptr;
    topic_t* topic;
    topic_t* _topic;
    char* delim = "/";

    if (strlen (topic_str))
    {
        buf = strtok_r(topic_str, delim, &saveptr);
    }
    else
    {
        return NULL;
    }

    if (! (topic = malloc (sizeof (topic_t))))
    {
        return NULL;
    }

    topic->name = buf;
    topic->sub_size = 0;
    topic->sub = NULL;
    topic->next = NULL;
    topic->topic_message = NULL;
    _topic = topic;

    while (strlen (saveptr))
    {
        if (strlen (saveptr))
        {
            buf = strtok_r(saveptr, delim, &saveptr);
        }
        else
        {
            break;
        }

        if (! (_topic->sub = malloc (sizeof( topic_t))))
        {
            free (topic);
            return NULL;
        }

        _topic->sub_size++;
        _topic->sub->name = buf;
        _topic->sub->sub_size = 0;
        _topic->sub->sub = NULL;
        _topic->sub->next = NULL;
        _topic->topic_message = NULL;
        _topic = _topic->sub;
    }

    while (_topic->topic_message)
    {
        _topic->topic_message = _topic->topic_message->next;
    }

    if (message)
    {
        _topic->topic_message = malloc (sizeof (topic_message_t));
        _topic->topic_message->message = message;
        _topic->topic_message->next = NULL;
    }

    return topic;
}

topic_t* get_topic(topic_t* topics, uint8_t* topic_str)
{
    uint8_t* buf;
    uint8_t* _buf;
    uint8_t* saveptr;
    char* delim = "/";

    if (strlen (topic_str))
    {
        _buf = malloc (strlen (topic_str) + 1);
        memset (_buf, 0, strlen (topic_str) + 1);
        strcpy (_buf, topic_str);

        buf = strtok_r(_buf, delim, &saveptr);
        free (_buf);
    }
    else
    {
        return NULL;
    }

    while (topics)
    {
        if (! strcmp(topics->name, buf))
        {
            if (strlen (saveptr))
            {
                buf = strtok_r(saveptr, delim, &saveptr);
            }
            else
            {
                return topics;
            }
            topics = topics->sub;
            continue;
        }
        else
        {
            topics = topics->next;
            continue;
        }
    }

    return NULL;
}

int insert_subscriber(topic_t* topic, void* connection)
{
    subscriber_t* subscribers;

    if (! topic)
    {
        return -1;
    }

    if (! topic->subscribers)
    {
        topic->subscribers = malloc (sizeof (subscriber_t));
        memset (topic->subscribers, 0, sizeof (subscriber_t));
        topic->subscribers->connection = connection;
        topic->subscribers->next = NULL;
        return 0;
    }

    subscribers = malloc (sizeof (subscriber_t));
    memset (subscribers, 0, sizeof (subscriber_t));
    subscribers->connection = connection;
    subscribers->next = topic->subscribers;
    topic->subscribers = subscribers;

    return 0;
}
