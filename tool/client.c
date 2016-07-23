#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pthread.h"

#define MAX_BUF_SIZE 1024

void * test_main(void *val);
int read_block(int fd, char *buf, int nbytes, int *end);
int write_block(int fd, char *buf, int nbytes);

// read in block
int read_block(int fd, char *buf, int nbytes, int *end){
    int num;
    int tot = 0;
    *end = 0;
    while(tot != nbytes){
        num = read(fd, buf + tot, nbytes - tot);
        if(num == 0){
            *end = 1;
            close(fd);
            return tot;
        }
        if(num < 0){
            continue;
        }else{
            tot += num;
        }
    }
    return tot;
}

// write in block
int write_block(int fd, char *buf, int nbytes){
    int num;
    int tot = 0;
    while(tot != nbytes){
        num = write(fd, buf + tot, nbytes - tot);
        if(num < 0){
            continue;
        }else{
            tot += num;
        }
    }
    return tot;
}

int tcp_server_creat(const char *address, int port)
{
    int s_fd; 
    struct sockaddr_in      addr; 
     
    s_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(s_fd < 0) 
    { 
        printf("Socket Error:%s\n",strerror(errno)); 
        return -1; 
    } 
    
    bzero(&addr, sizeof(struct sockaddr_in)); 
    addr.sin_family = AF_INET; 
    addr.sin_port = htons(port); 
    if(inet_aton(address, &addr.sin_addr) < 0) 
    { 
        printf("Ip error:%s, port %d\n", strerror(errno), port); 
        return 0; 
    }
    if(connect(s_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))  < 0){
        printf("connect fail");
    }
    return s_fd;
}

void help(void){
    const char *str = "need thread num!\n";
    printf("%s", str);
}

void * test_main(void *val) 
{
    int id;
    int s_fd;
    int w_fd;
    int data_len;
    int num;
    int endf;
    int ends;
    char path[MAX_BUF_SIZE] = "./r_test";
    char data_buf[MAX_BUF_SIZE] = {0};

    int sockfd = 0;
    
    id = (int)getpid();

    printf("thread with id %d start \n", id);
    sprintf(path + strlen(path), "%d", id);
    w_fd = open(path, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

    if (w_fd < 0)
    {
        printf("open file %s failed\n", path);
        exit(0);
    }

    sockfd = tcp_server_creat("127.0.0.1", 11111);
    if (sockfd < 0)
    {
        printf("tcp create server failed\n");
        return NULL;
    }

    s_fd = open("s_test", O_RDONLY, S_IRWXU);
    if (s_fd < 0)
    {
        printf("id %d open file s_test failed\n", id);
        return NULL;
    }

    while(1)
    {
            bzero(data_buf, MAX_BUF_SIZE);

            num = read_block(s_fd, data_buf, MAX_BUF_SIZE/4, &endf);
            data_len = num;

            num = write_block(sockfd, data_buf, data_len);
            data_len = num;

            num = read_block(sockfd, data_buf, data_len, &ends);
            
            write_block(w_fd, data_buf, num);

            if(endf){
                close(s_fd);
                close(w_fd);
                close(sockfd);
                return NULL;
            }
    } 
}

int main(int argc, char *argv[])
{
    int i = 0;
    int tots;
    int pid = 0;
    int *key;
    pthread_t t;
    
    if(argc != 2){
        help();
        return 0;
    }
    tots = atoi(argv[1]);
    for(i = 1; i <= tots; i++){
        key = malloc(sizeof(int));
        *key = i;
        pid = fork();
        if(pid == 0){
            test_main(&pid);
            break;
        }

    }
    while(1)
        sleep(1);
    return 0;
}

