#include <time.h>
#include "csapp.h"
typedef struct
{
    int size;
    char *value;
    char *key;
    time_t last_time;
} cache_item_t;

typedef struct
{
    int n;
    int limit;
    cache_item_t **items;
    int cache_size;
    int reader_cnt;
    sem_t mutex, w;

} cache_t;
void cache_item_free(cache_item_t *item);

void cache_init(cache_t *cache, int n);
void cache_free(cache_t *cache);

int cache_insert(cache_t *cache, const char *url, const char *object, int object_size);
void cache_remove(cache_t *cache, int index);
int cache_query(cache_t *cache, const char *key, char *object, int *object_size);

int reader( cache_t *cache, const char *url, char *object_buf, int *object_size);
int writer(cache_t *cache, const char *url, const char *object_buf, int object_size);