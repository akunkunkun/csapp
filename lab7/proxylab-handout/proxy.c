#include <stdio.h>
#include <csapp.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE 10
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* some new struct */
typedef struct Uri{
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
}Uri;

struct rwlock_t{
    sem_t lock ;    // basic lock
    sem_t writelock ; // writer lock
    int readers;
};
struct rwlock_t *rw ;

typedef struct Cache {
    int used ;
    char key[MAXLINE];
    char value[MAX_OBJECT_SIZE];
}Cache;
Cache cache[MAX_CACHE];
int nowpointer;

/* start to create a server */
void doit(int fd);
void read_requesthdrs(rio_t *rp);
void build_header(char *server , Uri *uri_data , rio_t *rio);
void parse_uri(char *uri, Uri* uri_data);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/* end -------------------------------*/

/* thread func*/
void thread(void *v);
void rwlock_init();                                 //初始化读写者锁指针 
int  readcache(int fd, char* key);                  //读缓存 
void writecache(char* buf, char* key);              //写缓存 


int main(int argc, char **argv)
{
    rw = Malloc(sizeof(struct rwlock_t));
    pthread_t tid;
    rwlock_init();
    printf("%s", user_agent_hdr);
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    //signal(SIGPIPE,sigpipe_handler);
    listenfd = Open_listenfd(argv[1]);
    while (1) {
	    clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid,NULL,thread,(void*)&connfd);                                       //line:netp:tiny:doit
                                                   //line:netp:tiny:close
    }
    Close(connfd); 
    return 0;
}


/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE] , uritemp[MAXLINE];
    char server[MAXLINE];
    rio_t rio , server_rio;
    Uri * uri_data = (Uri*)malloc(sizeof(Uri));

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;

    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    strcpy(uritemp,uri);

    if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
        printf("Proxy does not implement the method");
        return;
    }                                                    //line:netp:doit:endrequesterr
    
    if (readcache(fd,uritemp) != 0)
        return ;

    parse_uri(uri,uri_data);

    build_header(server,uri_data,&rio);

    int serverfd = Open_clientfd(uri_data->host , uri_data->port);

    if (serverfd < 0){
        printf("connection failed\n");
        return ;
    }

    Rio_readinitb(&server_rio,serverfd);
    Rio_writen(serverfd,server , strlen(server));

    char cache[MAX_OBJECT_SIZE];
    size_t n;
    int sum = 0;
    while ((n = rio_readlineb(&server_rio , buf , MAXLINE)) != 0){
        Rio_writen(fd, buf, n);
        sum += n;
        strcat(cache,buf);
    }
    printf("proxy received %d bytes,then send\n", sum);
    if (sum < MAX_OBJECT_SIZE)
        writecache(cache,uritemp);
    Close(fd);
    return ;
}
/* $end doit */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
void parse_uri(char *uri, Uri * uri_data) 
{
    char* hostpose = strstr(uri, "//");
    if (hostpose == NULL){
        char* pathpose = strstr(uri, "/");
        if (pathpose != NULL)
            strcpy(uri_data->path, pathpose);
        strcpy(uri_data->port, "80");
        return;
    } else{
        char* portpose = strstr(hostpose + 2, ":");
        if (portpose != NULL){
            int tmp;
            sscanf(portpose + 1, "%d%s", &tmp, uri_data->path);
            sprintf(uri_data->port, "%d", tmp);
            *portpose = '\0';
            
        } else{
            char* pathpose = strstr(hostpose + 2, "/");
            if (pathpose != NULL){
                strcpy(uri_data->path, pathpose);
                strcpy(uri_data->port, "80");
                *pathpose = '\0'; 
            }
        }
        strcpy(uri_data->host, hostpose + 2);
    }
    return;
}
/* $end parse_uri */


void build_header(char *server , Uri *uri_data , rio_t *rio){
    static const char* Con_hdr = "Connection: close\r\n";
    static const char* Pcon_hdr = "Proxy-Connection: close\r\n";
    char buf[MAXLINE];
    char Reqline[MAXLINE], Host_hdr[MAXLINE], Cdata[MAXLINE];//分别为请求行，Host首部字段，和其他数据信息 
    sprintf(Reqline, "GET %s HTTP/1.0\r\n", uri_data->path);   //获取请求行 
    while (Rio_readlineb(rio, buf, MAXLINE) > 0){
        /*读到空行就算结束，GET请求没有实体体*/
        if (strcmp(buf, "\r\n") == 0){
            strcat(Cdata, "\r\n");
            break;          
        }
        else if (strncasecmp(buf, "Host:", 5) == 0){
            strcpy(Host_hdr, buf);
        }
        
        else if (!strncasecmp(buf, "Connection:", 11) && !strncasecmp(buf, "Proxy_Connection:", 17) &&!strncasecmp(buf, "User-agent:", 11)){
            strcat(Cdata, buf);
        }
    }
    if (!strlen(Host_hdr)){
        /*如果Host_hdr为空，说明该host被加载进请求行的URL中，我们格式读从URL中解析的host*/
        sprintf(Host_hdr, "Host: %s\r\n", uri_data->host); 
    }
    
    sprintf(server, "%s%s%s%s%s%s", Reqline, Host_hdr, Con_hdr, Pcon_hdr, user_agent_hdr, Cdata);
    return;
}

void thread(void *v){
    int confd = (int)*(int *)v; 
    Pthread_detach(Pthread_self());
    doit(confd);
    close(confd);
}

void rwlock_init(){
    rw->readers = 0;
    Sem_init(&rw->lock,0,1 );
    Sem_init(&rw->writelock , 0 , 1);
}

void writecache (char * buf , char *key){
    P(&rw->writelock);
    int index ;
    while (cache[nowpointer].used != 0){
        cache[nowpointer].used = 0;
        nowpointer = (nowpointer + 1 )%MAX_CACHE;
    }

    index = nowpointer;

    cache[index].used = 1;
    strcpy(cache[index].key , key);
    strcpy(cache[index].value , buf);
    V(&rw->writelock);
    return ;
}

int readcache(int fd , char * uri){
    P(&rw->lock);
    if (rw->readers == 0)
        P(&rw->writelock);
    rw->readers++;
    V(&rw->lock);
    int i , flag = 0;

    for (i = 0 ; i < MAX_CACHE ;i++){
        if (strcmp(uri,cache[i].key) == 0){
            Rio_writen(fd,cache[i].value,strlen(cache[i].value));
            printf("proxy send %d bytes to client\n", strlen(cache[i].value));
            cache[i].used = 1;
            flag = 1;
            break;
        }
    }

    P(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)
        V(&rw->writelock);
    V(&rw->lock);
    return flag;
}