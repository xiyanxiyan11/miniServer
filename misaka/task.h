#ifndef TASK_H
#define TASK_H

#include "bgs.h"

int  echo_unpack(struct stream *s, struct peer *peer);
int  echo_pack(struct stream *s, struct peer *peer);
int  echo_register(void);
void debug_event(struct stream *s);

#endif
