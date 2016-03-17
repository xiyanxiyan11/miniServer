#include "shm.h"

struct shmhandle *shm_new(key_t key, int size){
    struct shmhandle *handle = malloc(sizeof(struct shmhandle));
    if(NULL == handle){
        return NULL;
    }
    handle->size = size;
    handle->key = key;
    handle->running = 0;
    return handle;
}

void *shm_open(struct shmhandle *handle){
    int shmid;
    void *mem;
    shmid = shmget((key_t)handle->key, handle->size, 0666|IPC_CREAT|IPC_EXCL);
    if(shmid < 0){
        return NULL;
    }
    mem = shmat(shmid, 0, 0);
    if(!mem){
        return NULL;
    }
    handle->shmid = shmid;
    handle->mem = mem;
    handle->running = 1;
    return mem;
}

int shm_close(struct shmhandle *handle){
    if(NULL == handle->mem)
            return 0;
    
    if(shmdt(handle->mem) < 0)
    {
	    return -1;
    }
    
    if(shmctl(handle->shmid, IPC_RMID, 0) < 0)
    {
	    return -2;
    }
    handle->running = 0;
    handle->mem = NULL;

    return 0;
}

int shm_delete(struct shmhandle *handle){
    int ret;
    ret = shm_close(handle);
    if(ret < 0)
            return -1;
    free(handle);
    return 0;
}
