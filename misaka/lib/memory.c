#include "memory.h"

void *misakamalloc(int mtype, int size){
    void *mem = NULL;

    switch(mtype){
#ifdef MISAK_CACHE_SUPPORT
        case MTYPE_MISAKA_PEER:
            mem = kmem_cache_alloc(misaka_servant.peer_cache);
            break;
        case MTYPE_STREAM:
            mem = kmem_cache_alloc(misaka_servant.stream_cache);
            break;
        case MTYPE_STREAM_FIFO:
            mem = kmem_cache_alloc(misaka_servant.fifo_cache);
            break;
        case MTYPE_MISAKA_DATA:
            mem = kmem_cache_alloc(misaka_servant.data_cache);
            break;
#endif
        default:
            mem = malloc((size_t)size);
            break;
    }
    if(mem == NULL){
        zlog_err("malloc ptr fail with type %d\n", mtype);
    }
    return mem;
}

void *misakacalloc(int mtype, int size){
    return calloc ((1), ((size_t)size));
}

void *misakarealloc(int mtype, void *ptr, int size){
    return realloc ((ptr), ((size_t)size));
}

void misakafree(int mtype, void *ptr){
    if(!ptr){
        zlog_err("free empty ptr type %d\n", mtype);
        return;
    }
    switch(mtype){
#ifdef MISAK_CACHE_SUPPORT
        case MTYPE_MISAKA_PEER:
            kmem_cache_free(misaka_servant.peer_cache, ptr); 
            break;
        case MTYPE_STREAM:
            kmem_cache_free(misaka_servant.stream_cache, ptr); 
            break;
        case MTYPE_STREAM_FIFO:
            kmem_cache_free(misaka_servant.fifo_cache, ptr); 
            break;
        case MTYPE_MISAKA_DATA:
            kmem_cache_free(misaka_servant.data_cache, ptr); 
            break;
#endif
        default:
            free(ptr);
            break;
    }
}
