#include "cachelab.h"
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
typedef unsigned long long ULL;
typedef unsigned char UINT8;
typedef struct Cache
{
    ULL tag;
    clock_t last_used_time;
    UINT8 valid;
} Cache;
int LoadArgs(int *ps, int *pE, int *pb, char *file, char **argv, int idx);
void Check(Cache *cache, int s, int E, int b, ULL address);

int hit_count, miss_count, eviction_count;
int main(int argc, char *argv[])
{
    int s = 0, E = 0, b = 0;
    char file_name[100];
    for (int i = 1; i < argc; i += 2)
    {
        if (LoadArgs(&s, &E, &b, file_name, argv, i) == 0)
            return -1;
    }
    Cache *cache = (Cache *)malloc(sizeof(Cache) * (1 << s) * E);
    memset(cache, 0, sizeof(Cache) * (1 << s) * E);

    FILE *in_file = fopen(file_name, "r");
    if (in_file == NULL)
        return -1;
    char one_line[100];
    char type=0;
    ULL address;
    hit_count = 0, miss_count = 0, eviction_count = 0;
    while (fgets(one_line, 99, in_file))
    {
        if (one_line[0] == 'I')
            continue;
        sscanf(one_line, " %c %llx", &type, &address);
        Check(cache, s, E, b, address);
        if (type == 'M')
            Check(cache, s, E, b, address);
    }
    printSummary(hit_count, miss_count, eviction_count);
    // printf("hits:%d misses:%d evictions:%d\n", hit_count, miss_count, eviction_count);
    free(cache);
    fclose(in_file);
    return 0;
}

void ParseAddress(ULL address, ULL *tag, ULL *set_index, ULL *block_offset, int s, int E, int b)
{
    int t = sizeof(ULL) * 8 - s - b;
    *tag = (address >> (s + b));
    *block_offset = ((address << (t + s)) >> (t + s));
    *set_index = ((address >> b) & ((1ull << s) - 1));
}

void Check(Cache *cache, int s, int E, int b, ULL address)
{
    ULL set_index = 0, tag = 0, block_offset = 0;
    ParseAddress(address, &tag, &set_index, &block_offset, s, E, b);

    int ind = E * set_index;
    clock_t oldest_time = 0;
    int is_hit = 0, oldest_time_i = 0;
    int has_invalid = 0, invalid_i = 0;
    for (int i = 0; i < E; i++)
    {
        // printf("E:%d\n", E);
        if (cache[ind + i].valid)
        {
            if (cache[ind + i].tag == tag)
            {
                hit_count++;
                cache[ind + i].last_used_time = clock();
                // printf("address:%llx,idx: %d, clock_idx:%lld, clock_now: %lld\n ", address, i, tmp, cache[ind + i].last_used_time);
                is_hit = 1;
                break;
            }
            else
            {
                if (oldest_time == 0)
                    oldest_time = cache[ind + i].last_used_time, oldest_time_i = i;
                else
                {
                    if (oldest_time > cache[ind + i].last_used_time)
                        oldest_time = cache[ind + i].last_used_time, oldest_time_i = i;
                }
            }
        }
        else
            has_invalid = 1, invalid_i = i;
    }
    if (is_hit == 0)
    {
        int idx = oldest_time_i;
        miss_count++;
        if (has_invalid == 0)
        {
            eviction_count++;
        }
        else
            idx = invalid_i;
        cache[ind + idx].valid = 1;
        cache[ind + idx].tag = tag;
        cache[ind + idx].last_used_time = clock();
        // printf("address:%llx, has_inidx:%d, clock_idx:%lld, clock_now: %lld\n ", address, idx, oldest_time, cache[ind + idx].last_used_time);
    }
}

int LoadArgs(int *ps, int *pE, int *pb, char *file, char **argv, int idx)
{
    if (argv[idx][1] == 's')
    {
        *ps = atoi(argv[idx + 1]);
    }
    else if (argv[idx][1] == 'E')
    {
        *pE = atoi(argv[idx + 1]);
    }
    else if (argv[idx][1] == 'b')
    {
        *pb = atoi(argv[idx + 1]);
    }
    else if (argv[idx][1] == 't')
    {
        file[0] = '.';
        file[1] = '/';
        strcpy(file + 2, argv[idx + 1]);
        // memcpy(file + 2, argv[idx + 1], strlen(argv[idx + 1]));
    }
    else
        return 0;
    return 1;
}