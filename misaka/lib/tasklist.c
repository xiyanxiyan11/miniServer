#include "tasklist.h"

//init task mag
struct task_list * tasklist_new(){
    struct task_list *m = malloc(sizeof(struct task_list));
    if(NULL == m){
        return NULL;
    }
    memset(m, 0, sizeof(struct task_list));
    spinlock_init(&m->lock);
    m->fifo = stream_fifo_new();
    if(NULL == m->fifo)
        return NULL;

    return m;
}

//push event packet into task_list;
void tasklist_push(struct task_list *m, struct stream *s){
    printf("push s into stream list\n");
    stream_fifo_push(m->fifo, s);
}

//pop event packet from task_list;
struct stream * tasklist_pop(struct task_list *m){
    struct stream *s = NULL;

        printf("%d packet left in task\n", m->fifo->count);
        if (stream_fifo_head(m->fifo)){
            s = stream_fifo_pop(m->fifo);
        }else{
    
        }
    return s;
}


