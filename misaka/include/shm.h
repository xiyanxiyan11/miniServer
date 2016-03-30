#ifndef SHM_H
#define SHM_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>

struct shmhandle{
    void *mem;      //mem ptr
    int size;       //size of the mem
    key_t key;      //key of the mem
    int running;    //in use?
    int shmid;
};


struct shmhandle *shm_new(key_t key, int size);
void *shm_open(struct shmhandle *handle);
int shm_close(struct shmhandle *handle);
int shm_delete(struct shmhandle *handle);


#endif
