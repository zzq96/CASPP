#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
int bss_var;
void fun(int b)
{
    static a = 1;
}
int main(int argc, char **argv)
{
    char *p = NULL, *q;
    printf("The location of bss segment is %p\n", &bss_var);
    printf("_________________________________\n");
    p = (char *)malloc(32);
    q = p + 64;
    printf("Addresses of heap start: %p\n", p);
    printf("_________________________________\n");
    brk(q);
    printf("Address of heap : %p\n", sbrk(0));
    printf("The range of heap is %d\n", (char *)sbrk(0) - p);
    free(p);
    p = (char *)malloc(100);
    printf("Addresses of heap start: %p\n", p);
    printf("Address of heap now : %p\n", sbrk(0));
    printf("The range of heap is %d\n", (char *)sbrk(0) - p);
    return 0;
}