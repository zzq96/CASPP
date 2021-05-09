#include "cache.h"
#include "csapp.h"
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
static void cache_place_item(cache_t *cache, const char *url, const char *object, int object_size);
static int cache_LRU(cache_t *cache);
static int cache_is_exist(cache_t *cache, const char *key);

void cache_item_free(cache_item_t *item)
{
    Free(item->key);
    Free(item->value);
    item->value = NULL;
    item->key = NULL;
    item->size = 0;
    item->last_time = 0;
}

void cache_init(cache_t *cache, int n)
{
    cache->n = 0;
    cache->limit = n;
    cache->items = (cache_item_t **)calloc(n, sizeof(cache_item_t *));
    cache->cache_size = 0;
    cache->reader_cnt = 0;
    Sem_init(&cache->mutex, 0, 1);
    Sem_init(&cache->w, 0, 1);
}
void cache_free(cache_t *cache)
{
    for (int i = 0; i < cache->n; i++)
    {
        cache_item_free(cache->items[i]);
        Free(cache->items[i]);
    }
    Free(cache->items);
    cache->n = 0;
    cache->cache_size = 0;
}

int cache_insert(cache_t *cache, const char *url, const char *object, int object_size)
{
    int index = -1;
    //判断是否存在并remove, 应该在判断size是否大于MAX之前
    if ((index = cache_is_exist(cache, url)) >= 0)
    {
        cache_remove(cache, index);
    }
    if (object_size > MAX_OBJECT_SIZE)
        return -1;
    //给new cache腾出空间
    while (cache->n == cache->limit - 1 || MAX_CACHE_SIZE - cache->cache_size < object_size)
    {
        int index = cache_LRU(cache);
        cache_remove(cache, index);
    }
    cache_place_item(cache, url, object, object_size);
    return 1;
}
void cache_remove(cache_t *cache, int index)
{
    cache->cache_size -= cache->items[index]->size;
    --cache->n;
    cache_item_free(cache->items[index]);
    Free(cache->items[index]);
    cache->items[index] = NULL;
}

static void cache_place_item(cache_t *cache, const char *url, const char *object, int object_size)
{
    for (int i = 0; i < cache->limit; i++)
        if (cache->items[i] == NULL)
        {
            cache->items[i] = (cache_item_t *)calloc(1, sizeof(cache_item_t));
            cache->items[i]->size = object_size;
            cache->items[i]->key = (char *)malloc(strlen(url) + 1);
            strcpy(cache->items[i]->key, url);
            cache->items[i]->value = (char *)malloc(object_size);
            memcpy(cache->items[i]->value, object, object_size);

            cache->items[i]->last_time = time(NULL);
            return;
        }
    fprintf(stderr, "Error: placing cache item failed due to no free slot\n");
}

static int cache_LRU(cache_t *cache)
{
    time_t oldest = time(NULL);
    int index = 0;
    for (int i = 0; i < cache->limit; i++)
        if (cache->items[i])
        {
            if (cache->items[i]->last_time < oldest)
                oldest = cache->items[i]->last_time, index = i;
        }
    return index;
}

int cache_query(cache_t *cache, const char *key, char *object, int *object_size)
{
    int index = -1;
    if ((index = cache_is_exist(cache, key)) < 0)
        return -1;
    *object_size = cache->items[index]->size;
    memcpy(object, cache->items[index]->value, *object_size);
    cache->items[index]->last_time = time(NULL);
    return 1;
}

static int cache_is_exist(cache_t *cache, const char *key)
{
    for (int i = 0; i < cache->limit; i++)
        if (cache->items[i] != NULL && strcmp(cache->items[i]->key, key) == 0)
            return i;
    return -1;
}

int reader(cache_t *cache, const char *url, char *object_buf, int *object_size)
{
    int status = -1;
    P(&cache->mutex);
    if (++cache->reader_cnt == 1)
        P(&cache->w);
    V(&cache->mutex);

    status= cache_query(cache, url, object_buf, object_size);

    P(&cache->mutex);
    if (--cache->reader_cnt == 0)
        V(&cache->w);
    V(&cache->mutex);
    return status;
}
int writer(cache_t *cache, const char *url, const char *object_buf, int object_size)
{
    int status = -1;
    P(&cache->w);
    status = cache_insert(cache, url, object_buf, object_size);
    V(&cache->w);
    return status;
}