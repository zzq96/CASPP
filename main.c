#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
char s[10];
int main()
{
    // int fd = open("in.txt", O_RDWR, O_CREAT);
    // char s[10] = "22";
    // write(fd, s, strlen(s));
    // read(fd, s, 2);
    // puts(s);
    FILE *f = fopen("in.txt", "a+");
    fread(s, 1, 2, f);
    fwrite("aa", 1, 2, f);

    return 0;
}