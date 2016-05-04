#ifndef SKYNET_MESSAGE_QUEUE_H
#define SKYNET_MESSAGE_QUEUE_H

#include <stdlib.h>
#include <stdint.h>
#include <stream.h>

// type is encoding in stream.sz high 8bit
#define MESSAGE_TYPE_MASK (SIZE_MAX >> 8)
#define MESSAGE_TYPE_SHIFT ((sizeof(size_t)-1) * 8)

struct message_queue;

typedef void (*message_drop)(struct stream **, void *);


void  skynet_globalmq_push(struct message_queue * queue);
struct message_queue * skynet_globalmq_pop();
void mq_globalset(struct message_queue *q, int val);
struct message_queue * skynet_mq_create(uint32_t handle);
uint32_t skynet_mq_handle(struct message_queue *q);
int skynet_mq_length(struct message_queue *q);
int skynet_mq_overload(struct message_queue *q);
int skynet_mq_pop(struct message_queue *q, struct stream **message);
void skynet_mq_push(struct message_queue *q, struct stream **message);
void skynet_mq_init();
void  skynet_mq_mark_release(struct message_queue *q);
void  skynet_mq_release(struct message_queue *q, message_drop drop_func, void *ud);



















#endif
