#ifndef TASK_H
#define TASK_H

#include "misaka.h"

int  echo_unpack(struct stream *s, struct peer *peer);
int  echo_pack(struct stream *s, struct peer *peer);
int  echo_register(void);
int  echo_on_connect(struct peer *peer);
int  echo_on_disconnect(struct peer *peer);
void debug_event(struct stream *s);

#endif
