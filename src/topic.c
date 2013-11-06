#include "topic.h"

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

    printf ("--%s\n", saveptr);
    printf ("%d\n", strlen (saveptr));
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


    _topic->topic_message = malloc (sizeof (topic_message_t));
    _topic->topic_message->message = message;

    return topic;
}

topic_t* get_topic(topic_t* topics, uint8_t* topic_str)
{
    uint8_t* buf;
    uint8_t* _topic_str;
    uint8_t* saveptr;
    char* delim = "/";

    if (strlen (topic_str))
    {
        _topic_str = malloc (strlen (topic_str) + 1);
        memset (_topic_str, 0, strlen (topic_str) + 1);
        strcpy (_topic_str, topic_str);
        buf = strtok_r(topic_str, delim, &saveptr);
    }
    else
    {
        return NULL;
    }

    while (topics)
    {
        if (! strcmp(topics->name, buf))
        {
            if (strlen (_topic_str))
            {
                buf = strtok_r(saveptr, delim, &saveptr);
            }
            else
            {
                free (_topic_str);
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

    free (_topic_str);
    return NULL;
}
