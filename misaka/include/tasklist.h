#ifndef TASK_LIST_H
#define TASK_LIST_H

#include "misaka.h"
#include "spinlock.h"


//for task distribute and process for server
struct task_list{
    struct stream_fifo *fifo;    //task stream in fifo
    struct spinlock lock;       //task in lock 
};

struct task_list * tasklist_new();
void tasklist_push(struct task_list *m, struct stream *s);
struct stream * tasklist_pop(struct task_list *m);

#endif
