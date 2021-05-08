#ifdef RUNTIME
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
void *malloc(size_t size) {
    void*(*mallocp)(size_t size);
    char *error;
    mallocp = dlsym(RTLD_NEXT,"malloc");
    assert(dlerror() == NULL);
    char *ptr = mallocp(size);
    return ptr;
}
void free(void *ptr) {
    assert(ptr != NULL);
    char *error;
    void (*freep)(void *ptr);
    freep = dlsym(RTLD_NEXT,"free");
    assert(dlerror() == NULL);
    freep(ptr);
}
#endif