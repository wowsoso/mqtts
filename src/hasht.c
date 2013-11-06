#include"hasht.h"

unsigned long hashfunc(const char* key)
{
    /* DJB Hash */

    unsigned long hash = 5381;
    int c;
    char* _key;

    if (! (_key = malloc (strlen(key) + 1)))
    {
        return -1;
    }

    strcpy(_key, key);

    while (c = *_key++)
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

static char* strcopy(const char* key)
{
    char* _key;

    if (! (_key = malloc (strlen (key) + 1)))
    {
        return NULL;
    }

    strcpy(_key, key);

    return _key;
}

hasht_t* hasht_create(size_t size)
{
    hasht_t *hasht;

    if(!( hasht = malloc(sizeof (hasht_t)))) return NULL;

    if(!(hasht->buckets = calloc (size, sizeof (hash_buckets_t*)))) {
        free(hasht);
        return NULL;
    }

    hasht->size=size;

    return hasht;
}

void hasht_free(hasht_t* hasht)
{
	size_t i;
	hash_buckets_t* bucket;
    hash_buckets_t* old_bucket;

	for(i=0; i < hasht->size; i++) {
		bucket = hasht->buckets[i];
		while(bucket)
        {
			free(bucket->key);
			old_bucket = bucket;
			bucket = bucket->next;
			free (old_bucket);
		}
	}
	free (hasht->buckets);
	free (hasht);
}

int hasht_insert(hasht_t* hasht, const char* key, void* value)
{
    hash_buckets_t* bucket;
    size_t hash = hashfunc (key) % hasht->size;

    bucket = hasht->buckets[hash];

    while (bucket) {
        if(!strcmp(bucket->key, key)) {
            bucket->value = value;
            return 0;
        }
        else
        {
            bucket = bucket->next;
        }
    }

    /* FIXME: there is so slow, may be make a memory poll that is a great idea... */
    if(! (bucket = malloc (sizeof (hash_buckets_t))))
    {
        return -1;
    }

    if (! (bucket->key = strcopy (key))) {
        free (bucket);
        return -1;
    }

    bucket->value = value;
    bucket->next = hasht->buckets[hash];
    hasht->buckets[hash] = bucket;

    return 0;
}

int hasht_remove(hasht_t* hasht, const char *key)
{
    hash_buckets_t* bucket;
    hash_buckets_t* prev_bucket = NULL;
    size_t hash = hashfunc (key) % hasht->size;

    bucket = hasht->buckets[hash];

    while (bucket)
    {
        if(! strcmp (bucket->key, key))
        {
            free (bucket->key);
            if (prev_bucket) prev_bucket->next = bucket->next;
            else hasht->buckets[hash] = bucket->next;
            free (bucket);
            return 0;
        }
        else
        {
            prev_bucket = bucket;
            bucket = bucket->next;
        }
    }

    return -1;
}

void* hasht_get(hasht_t* hasht, const char *key)
{
    hash_buckets_t* bucket;
    size_t hash= hashfunc (key) % hasht->size;

    bucket = hasht->buckets[hash];

    while(bucket) {
        if(! strcmp (bucket->key, key))
        {
            return bucket->value;
        }
        bucket = bucket->next;
    }

    return NULL;
}
