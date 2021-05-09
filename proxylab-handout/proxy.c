#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define SBUFSIZE 16
#define NTHREADS 4
#define NCACHEITEM 100

typedef struct
{
    char key[MAXLINE];
    char value[MAXLINE];
} ReqHead;

typedef struct
{
    ReqHead require_heads[20];
    char require_line[MAXLINE];
    char method[MAXLINE];
    char uri[MAXLINE];
    char url[MAXLINE];
    char port[MAXLINE];
    char http_version[MAXLINE];
    char hostname[MAXLINE];
    int head_num;
} Require;

typedef struct
{
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;

static void init_run_nthreads(void);
void *thread(void *vargp);
void sbuf_insert(sbuf_t *sbuf, int connfd);
int sbuf_remove(sbuf_t *sbuf);
int doit(int connfd);
void send_require_to_server(int serverfd, const Require *req);
void print_heads(const Require *req);
void set_head_if_noexist(Require *req, const char *key, const char *value);
void parse_require(Require *req, rio_t *rio);
void parse_url(Require *req);

void sbuf_init(sbuf_t *sbuf, int n)
{
    sbuf->n = n;
    sbuf->buf = calloc(n, sizeof(int));
    sbuf->front = sbuf->rear = 0;
    Sem_init(&sbuf->slots, 0, n);
    Sem_init(&sbuf->items, 0, 0);
    Sem_init(&sbuf->mutex, 0, 1);
}
void sbuf_free(sbuf_t *sbuf)
{
    Free(sbuf->buf);
}

sbuf_t sbuf;
cache_t cache;
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";

int main(int argc, char *argv[])
{
    printf("hello world\n");
    fflush(stdout);
    int listenfd, connfd;
    struct sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    sbuf_init(&sbuf, SBUFSIZE);
    cache_init(&cache, NCACHEITEM);
    pthread_t tid;
    int tnum[NTHREADS];

    for (int i = 0; i < NTHREADS; i++)
    {
        tnum[i] = i;
        Pthread_create(&tid, NULL, thread, &tnum[i]);
    }

    listenfd = Open_listenfd(argv[1]);
    // printf("hello world 2\n");
    // fflush(stdout);
    while (1)
    {
        // printf("hello world 3\n");
        // fflush(stdout);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        // printf("hello world 4\n");
        // fflush(stdout);
        sbuf_insert(&sbuf, connfd);
    }
    sbuf_free(&sbuf);
    cache_free(&cache);
    return 0;
}
// sem_t run_nthreads_mutex;
// static void init_run_nthreads(void)
// {
//     Sem_init(&run_nthreads_mutex, 0, 1);
// }
void *thread(void *vargp)
{
    // static int run_nthreads = 0;
    // static pthread_once_t once = PTHREAD_ONCE_INIT;
    int connfd = -1;
    int tnum = *(int *)vargp;

    // Pthread_once(&once, init_run_nthreads);
    Pthread_detach(pthread_self());
    while (1)
    {
        connfd = sbuf_remove(&sbuf);
        // printf("Thread %d accepted connection\n", tnum);
        // fflush(stdout);

        // P(&run_nthreads_mutex);
        // printf("%d threads are runing\n", ++run_nthreads);
        // fflush(stdout);
        // V(&run_nthreads_mutex);

        doit(connfd);
        Close(connfd);
        // sleep(15);

        // printf("Thread %d finished\n", tnum);
        // fflush(stdout);
        // P(&run_nthreads_mutex);
        // printf("%d threads are runing\n", --run_nthreads);
        // fflush(stdout);
        // V(&run_nthreads_mutex);
    }
}
void sbuf_insert(sbuf_t *sbuf, int connfd)
{
    P(&sbuf->slots);
    P(&sbuf->mutex);
    sbuf->buf[(++sbuf->rear) % sbuf->n] = connfd;
    V(&sbuf->mutex);
    V(&sbuf->items);
}
int sbuf_remove(sbuf_t *sbuf)
{
    int connfd = -1;
    P(&sbuf->items);
    P(&sbuf->mutex);
    connfd = sbuf->buf[(++sbuf->front) % sbuf->n];
    V(&sbuf->mutex);
    V(&sbuf->slots);
    return connfd;
}

int doit(int connfd)
{
    rio_t rio;
    Require req;
    Rio_readinitb(&rio, connfd);
    parse_require(&req, &rio);

    char object_buf[MAX_OBJECT_SIZE];
    int object_size;

    if ((reader(&cache, req.url, object_buf, &object_size) >= 0))
    {
        Rio_writen(connfd, object_buf, object_size);
        return 0;
    }
    //set some heads if no exist
    set_head_if_noexist(&req, "Host", req.hostname);
    set_head_if_noexist(&req, "User-Agent", user_agent_hdr);
    set_head_if_noexist(&req, "Connection", "close");
    set_head_if_noexist(&req, "Proxy-Connection", "close");

    int serverfd = Open_clientfd(req.hostname, req.port);
    send_require_to_server(serverfd, &req);

    Rio_readinitb(&rio, serverfd);
    int n = 0;
    char buf[MAXLINE];
    char *object_buf_ = object_buf;
    object_size = 0;
    while ((n = rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        Rio_writen(connfd, buf, n);
        object_size += n;
        if (object_size < MAX_OBJECT_SIZE) //如果大于限制了, 就不往buf写入了
        {
            memcpy(object_buf_, buf, n);
            object_buf_ += n;
        }
    }
    if (object_size <= MAX_OBJECT_SIZE)
    {
        writer(&cache, req.url, object_buf, object_size);
    }
    return 1;

    // print_heads(&req);
}

void send_require_to_server(int serverfd, const Require *req)
{
    char buf[MAXLINE];
    sprintf(buf, "%s %s %s\r\n", req->method, req->uri, "HTTP/1.0");
    for (int i = 0; i < req->head_num; i++)
    {
        sprintf(buf + strlen(buf), "%s: %s\r\n", req->require_heads[i].key, req->require_heads[i].value);
    }
    sprintf(buf + strlen(buf), "\r\n");
    // printf("send require header: \n%s", buf);

    Rio_writen(serverfd, buf, strlen(buf));
}
void print_heads(const Require *req)
{
    puts("heads");
    for (int i = 0; i < req->head_num; i++)
    {
        printf("%s: %s\n", req->require_heads[i].key, req->require_heads[i].value);
    }
}

void set_head_if_noexist(Require *req, const char *key, const char *value)
{
    int head_index = -1;
    for (int i = 0; i < req->head_num; i++)
        if (strcmp(req->require_heads[i].key, key) == 0)
        {
            head_index = i;
            break;
        }
    if (head_index != -1)
        strcpy(req->require_heads[head_index].value, value);
    else
    {
        strcpy(req->require_heads[req->head_num].key, key);
        strcpy(req->require_heads[req->head_num].value, value);
        req->head_num++;
    }
}

void parse_require(Require *req, rio_t *rio)
{
    char buf[MAXLINE];
    size_t n = 0;
    if ((n = Rio_readlineb(rio, req->require_line, MAXLINE)) != 0)
    {
        // printf("%p\n", strstr(req->require_line, "\r\n"));
        *(strstr(req->require_line, "\r\n")) = 0;
        printf("require_line: %s\n", req->require_line);
        sscanf(req->require_line, "%s %s %s", req->method, req->url, req->http_version);
        parse_url(req);
    }
    else
    {
        fprintf(stderr, "find EOF when read require line");
        exit(0);
    }

    // printf("require header\n");
    req->head_num = 0;
    while ((n = Rio_readlineb(rio, buf, MAXLINE)) != 0)
    {
        if (strcmp(buf, "\r\n") == 0)
            break;
        // printf("%p\n", strstr(buf, "\r\n"));
        *(strstr(buf, "\r\n")) = 0;
        char *ptr = strstr(buf, ":");
        *ptr = 0;
        strcpy(req->require_heads[req->head_num].key, buf);
        strcpy(req->require_heads[req->head_num].value, ptr + 2);
        // printf("key: %s; value: %s;\n", req->require_heads[req->head_num].key, req->require_heads[req->head_num].value);

        req->head_num++;
    }
}

//get and set hostname, uri, port
void parse_url(Require *req)
{
    char *url = req->url;
    char *ptr = NULL;
    if ((ptr = strstr(url, "http://")) == NULL)
    {
        fprintf(stderr, "Error: invalid url! :%s\n", url);
        exit(0);
    }
    url += strlen("http://");
    //port
    if ((ptr = strstr(url, ":")) == NULL)
    {
        strcpy(req->port, "80");
    }
    else
    {
        char *ptr_ = strstr(url, "/");
        *ptr_ = 0;
        strcpy(req->port, ptr + 1);
        *ptr_ = '/';
    }
    //host
    if ((ptr = strstr(url, ":")) == NULL)
    {
        ptr = strstr(url, "/");
        *ptr = 0;
        strcpy(req->hostname, url);
        *ptr = '/';
    }
    else
    {
        *ptr = 0;
        strcpy(req->hostname, url);
        *ptr = ':';
    }

    //uri
    ptr = strstr(url, "/");
    strcpy(req->uri, ptr);
    // printf(" host: %s\n uri: %s\n port: %s\n", req->hostname, req->uri, req->port);
}