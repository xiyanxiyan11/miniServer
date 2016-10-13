#ifndef MTIMER_H
#define MTIMER_H

void    server_timer_init(void);
void    server_timer_updatetime(void);
int     server_timer_timeout(int time, void *handle);

#endif
