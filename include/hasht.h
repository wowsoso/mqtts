#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct hash_buckets_s {
    char*              key;
    void*              value;
    struct hash_buckets_s* next;
} hash_buckets_t;

typedef struct hasht_s {
    hash_buckets_t** buckets;
    size_t           size;
} hasht_t;

hasht_t* hasht_create(size_t size);
void hasht_free(hasht_t* hasht);
int hasht_insert(hasht_t* hasht, const char* key, void* value);
int hasht_remove(hasht_t* hasht, const char* key);
void* hasht_get(hasht_t* hasht, const char* key);
