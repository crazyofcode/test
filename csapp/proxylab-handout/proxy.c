#include <stdio.h>
#include "csapp.h"


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define URI_MAXSIZE 100
#define HDR_MAXSIZE 2048
#define SBUF_SIZE 16
#define NTHREADS 4
#define READER_SAME_TIME 3

typedef struct{
    int* buf;   //缓存数组
    int n;      //最大容量，也就是一次性运行线程的数量
    int front;      //最前面一个
    int rear;       //最后面一个
    sem_t mutex;        //控制对缓存数组的访问权限
    sem_t slots;        //可用的位置数量
    sem_t items;        //已经被用的位置数量
} sbuf_t;

//使用双向连表存储缓存数据，方便增删改查

typedef struct _node_t {
    char flag;
    char uri[URI_MAXSIZE];
    char respHeader[HDR_MAXSIZE];
    char respBody[MAX_OBJECT_SIZE];
    int respHeaderLen;
    int respBodyLen;
    struct _node_t* prev;
    struct _node_t* next;
}node_t;

typedef struct {
    node_t* head;
    node_t* tail;
    int nitems;
}link_t;


sbuf_t sbuf;
link_t cache;
int cacheSize, readcnt;
sem_t mutex_reader, mutex_writer;

void doit(int fd);
void build_requesthdrs(rio_t *rp, char* clientrequest, char* hostname, char* port);
void parse_uri(char* uri, char* hostname, char* port, char* filepath);
void read_response(int fd, char* hostName, char* port, char* clientRequest,char* uri);
void sbuf_init(sbuf_t* sp, int n);
void sbuf_insert(sbuf_t* sp, int item);
int sbuf_remove(sbuf_t* sp);
void* thread(void* vargp);
void cache_init();
node_t* readItem(char* uri, int fd);
void link_add(node_t* node);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char* connection_hdr = "Connection: close\r\n";
static const char* proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char* uri_prefix = "http://";
static const char* Space = "\r\n";

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];

    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if(argc != 2)
    {
        fprintf(stderr,"usage: %s <port>\n",argv[0]);
        exit(1);
    }

    pthread_t tidp;
    for(int i=0; i < NTHREADS; ++i)
    {
        pthread_create(&tidp,NULL,thread, NULL);
    }

    sbuf_init(&sbuf, SBUF_SIZE);

    /*初始化缓存链表*/
    cacheSize = 0;
    cache_init();

    readcnt = 0;
    Sem_init(&mutex_reader,0,READER_SAME_TIME);     //控制读的人数
    Sem_init(&mutex_writer,0,1);        //同一时间只能有一个人访问内存


    listenfd = Open_listenfd(argv[1]);   //创建一个监听描述符，准备好接受请求   -> 打开和返回一个监听描述符，这个描述符准备在端口port上接收连接请求  
    //建立与服务器的连接，该服务器运行在hostname上，并在端口号port上监听连接请求，返回一个套接字描述符

    while(1){
        clientlen = sizeof(clientaddr);     //计算客户端地址的长度
        connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);        //等待来自客户端的连接请求到达监听描述符listenfd，然后在addr中填写客户端的套接字地址，并返回一个一连接的描述符
        Getnameinfo((SA *)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0);       //将一个套接字地址结构转换成相应的主机和服务名字符串
        printf("Accept connection from (%s,%s)\n",hostname,port);
        sbuf_insert(&sbuf, connfd);
    }
    return 0;
}

void doit(int fd)
{
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    char hostname[MAXLINE], port[MAXLINE], filepath[MAXLINE];
    char clientrequest[MAXLINE];

    rio_t rio;

    Rio_readinitb(&rio, fd);        // 函数用于初始化一个 rio_t 结构，该结构用于高效地从文件描述符（fd）读取数据
                                    // rio_t 结构通常用于包装标准I/O库函数，以便更高效地进行输入操作。
                                    //这个结构可以跟踪已读取和未读取的字节数，以减少系统调用的次数，从而提高性能
    if(!Rio_readlineb(&rio, buf, MAXLINE))  return;
          //从输入流中读取一行文本，并将其存储到一个缓冲区中，同时处理不同操作系统的换行符（\n 或 \r\n）以及缓冲区不足的情况
    printf("get buf : %s\n",buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(strcasecmp(method, "GET")){
        clienterror(fd,method,"501","Not implemented","Tiny does not implement this method");
        return;
    }

    parse_uri(uri,hostname,port, filepath);

    //构建转发申请
    sprintf(clientrequest, "GET %s HTTP/1.0\r\n",filepath);

    build_requesthdrs(&rio,clientrequest,hostname,port);

    node_t* cacheData;
    if(cacheData = readItem(uri, fd))
    {
        printf("> use cache data. <\n");
        Rio_writen(fd, cacheData->respHeader,cacheData->respHeaderLen);
        Rio_writen(fd, Space , strlen(Space));
        Rio_writen(fd, cacheData->respBody,cacheData->respBodyLen);
        return;
    }

    //接受申请
    printf("no cache\n");
    read_response(fd, hostname, port, clientrequest,uri);
}

void build_requesthdrs(rio_t *rp, char* clientrequest, char* hostname, char* port)
{
    char buf[MAXLINE];

    ssize_t n = Rio_readlineb(rp,buf,MAXLINE);
    //printf("receive buf %s n = %ld\n",buf,  n);

    while(Rio_readlineb(rp,buf,MAXLINE) > 0 && strcmp(buf ,"\r\n"))
    {
        if(strstr(buf, "Host:") != NULL) continue;
        if(strstr(buf, "User-Agent:") != NULL) continue;
        if(strstr(buf, "Connection:") != NULL) continue;
        if(strstr(buf,"Proxy-connection:") != NULL) continue;

        sprintf(clientrequest,"%s%s",clientrequest,buf);
    }
    sprintf(clientrequest,"%sHost: %s:%s\r\n",clientrequest,hostname,port);
    sprintf(clientrequest,"%s%s%s%s",clientrequest,user_agent_hdr,connection_hdr,proxy_connection_hdr);
    sprintf(clientrequest,"%s\r\n",clientrequest);

    printf("formatted request : \n%s \n",clientrequest);
}

void parse_uri(char* uri, char* hostname, char* port, char* filepath)
/*将URI解析为一个个文件名和一个可任选的CGI参数字符串*/
{
    char *temp = strstr(uri, uri_prefix) + strlen(uri_prefix);

    char* colon = strstr(temp, ":");

    char* slash = strstr(temp, "/");

    strncpy(hostname,temp,colon - temp);

    strncpy(port,colon+1,slash - colon - 1);

    strcpy(filepath, slash);
}

void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg)
{
    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.0%s %s\r\n",errnum,shortmsg);
    Rio_writen(fd, buf,strlen(buf));
    sprintf(buf,"Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf,strlen(buf));
    sprintf(buf,"<html><title>Tiny  Error</title>");
    Rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"<body bgcolor = ""ffffff>"">\r\n");
    Rio_writen(fd,buf,strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}


void read_response(int fd, char* hostName, char* port, char* clientRequest, char* uri)
{
    int clientfd = Open_clientfd(hostName,port);

    printf("hostName: %s, clientfd = %d\n",hostName,clientfd);

    rio_t rp;

    Rio_readinitb(&rp,clientfd);
    Rio_writen(rp.rio_fd, clientRequest, strlen(clientRequest));

    char tinyResponse[MAXLINE];

    //对传输来的字符串进行写会操作
    int n, totalBytes = 0;

    node_t* node = Malloc(sizeof(node_t));
    node->flag = '0';
    strcpy(node->uri, uri);
    *node->respHeader = 0;
    *node->respBody = 0;
    while(n = Rio_readnb(&rp,tinyResponse,MAXLINE))
    {
        Rio_writen(fd, tinyResponse, n);
        if(!strcmp(tinyResponse,"\r\n")) break;

        strcat(node->respHeader,tinyResponse);
        totalBytes += n;
    }

    node->respHeaderLen = totalBytes;
    totalBytes = 0;

    /*对信息body进行读写*/
    while(n = Rio_readnb(&rp,tinyResponse,MAXLINE))
    {
        Rio_writen(fd, tinyResponse, n);
        strcat(node->respBody, tinyResponse);
        totalBytes += n;
    }

    node->respBodyLen = totalBytes;

    //如果信息量太大就放弃缓存

    if(totalBytes > MAX_CACHE_SIZE)
    {
        Free(node);
        return;
    }

    P(&mutex_writer);

    /*将数据添加到链表中*/
    link_add(node);

    V(&mutex_writer);

}

void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = 0;
    sp->rear = 0;
    Sem_init(&sp->mutex, 0,1);
    Sem_init(&sp->slots,0,n);
    Sem_init(&sp->items,0,0);
}


/*Insert item onto the rear of shared buffer sp*/
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);
    P(&sp->mutex);

    sp->buf[(++sp->rear)%(sp->n)] = item;

    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);      //等待可以访问item
    P(&sp->mutex);      //锁上当前缓冲区
    item = sp->buf[(++sp->front)%(sp->n)];
    V(&sp->mutex);
    V(&sp->slots);      //Announce available slot

    return item;
}


void *thread(void* vargp)
{
    Pthread_detach(Pthread_self());     //脱离主线程
    while(1)
    {
        int connfd = sbuf_remove(&sbuf);    //状态改为忙碌 

        //完成之前的任务
        doit(connfd);
        Close(connfd);
    }
}

node_t* readItem(char* uri, int fd)
{
    P(&mutex_reader);       //如果还有reader位置就进入，否则就等待

    readcnt++;

    if(readcnt == 0)
    {
        P(&mutex_writer);
    }

    V(&mutex_reader);

    node_t* p = cache.head->next;
    int bFound = 0;

    rio_t rio;
    Rio_readinitb(&rio,fd);

    while(p->flag != '@')
    {
        if(!strcmp(uri, ((node_t *)p)->uri)){
            bFound = 1;
            break;
        }
        p = p->next;
    }

    P(&mutex_reader);
    readcnt--;
    if(readcnt == 0)
    {
        V(&mutex_writer);
    }

    V(&mutex_reader);

    return bFound ? p:NULL;
}

void link_add(node_t *node)
{
    while (node->respBodyLen + cacheSize > MAX_CACHE_SIZE && cache.head->next != cache.tail) {
        node_t* last = cache.tail->prev;
        last->next->prev = last->prev;
        last->prev->next = last->next;

        last->next = NULL;
        last->prev = NULL;
        Free(last);

        cache.nitems--;
    }

    // 双链表的插入操作
    node->next = cache.head->next;
    node->prev = cache.head;
    cache.head->next->prev = node;
    cache.head->next = node;
    cacheSize += node->respBodyLen;
    cache.nitems++;
}


void cache_init()
{
    cache.nitems = 0;
    cache.head = Malloc(sizeof(node_t));
    cache.tail = Malloc(sizeof(node_t));
    cache.head->flag = '0';
    cache.head->next = cache.tail;
    cache.head->prev = NULL;
    cache.tail->flag = '@';
    cache.tail->next = NULL;
    cache.tail->prev = cache.head;
}