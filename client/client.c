#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pthread.h"
#include "spinlock.h"

struct spinlock packetlock;
int packetnum = 0;

#define MAX_BUF_SIZE 1024

void * test_main(void *val);


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


int main()
{
    int i = 0;
    pthread_t t;
    int pid = 0;
    int *key;
    spinlock_init(&packetlock);
    for(i = 1; i <= 10; i++){
        key = malloc(sizeof(int));
        *key = i;
        pid = fork();
        if(pid == 0)
            test_main(&pid);
        //pthread_create(&t, NULL, test_main, (void *)(key));
        //sleep(1);
    }

    while(1)
    sleep(1);
    return 0;
}


void * test_main(void *val) 
{
    int flag;
    int id;
    int s_fd;
    int w_fd;
    int data_len;
    int num;
    unsigned char  send_buf[MAX_BUF_SIZE];
    
    char path[MAX_BUF_SIZE] = "./r_test";
    char data_buf1[MAX_BUF_SIZE] = {0};
    

    int sockfd = 0;
    
    printf("thread with id %d start \n", id);

    id = (int)getpid();
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
        printf("UDP create server failed\n");
        return NULL;
    }


    s_fd = open("s_test", O_RDONLY, S_IRWXU);
    if (s_fd < 0)
    {
        printf("open file s_test failed\n");
        return NULL;
    }

    while(1) 
    {
            bzero(data_buf1, MAX_BUF_SIZE);
            data_len = read(s_fd, data_buf1, MAX_BUF_SIZE/4);
            if (data_len > 0)
            {

                write(sockfd, data_buf1, data_len);
            }
            num = read(sockfd, data_buf1, MAX_BUF_SIZE);
            if(num > 0){
                printf("read packet from tcp\n");
                write(w_fd, data_buf1, num);
            }

    } 
} 

