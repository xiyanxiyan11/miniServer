#include <idmaker.h>

int idmaker_get(struct idmaker *m, int *flag)
{
    int ret;
    void *data;
    struct listnode *node;
    if(m->curr != m->stop){
        ret = m->curr++;
    }else{
        node = listhead(m->store);
        if(!node){
           *flag = 0;
           return 0;
        }
        data = listgetdata(node);
        ret = *((int *)data);
        list_delete_node (m->store, node);
        XFREE(MTYPE_MISAKA_MID, data);
    }
    *flag = 1;
    return ret;
}

int idmaker_put(struct idmaker *m, int i){
    int *data = (int *)XMALLOC(MTYPE_MISAKA_MID, sizeof(int));
    if(!data){
        return -1;
    }
    listnode_add(m->store, (void*)data);
    return 0;
}

struct idmaker * idmaker_new(int start, int stop){
    struct idmaker *mid = (struct idmaker *)XMALLOC(MTYPE_MISAKA_MID, sizeof(struct idmaker));
    if(!mid){
        return NULL;
    }
    mid->store = list_new();
    if(!mid->store){
        return NULL;
    }
    mid->start = start;
    mid->stop  = stop;
    return mid;
}



























