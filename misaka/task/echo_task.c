#include "link.h"
#include "task.h"
#include "misaka.h"
#include <sys/time.h>
#include <math.h>

static int peer_id = 0;

//echo event, send this  packet back
void misaka_handle(struct stream *s)
{
    int tmp;
    tmp = s->src;
    s->src = s->dst;
    s->dst = tmp;
    s->type = EVENT_NET;
}

int misaka_connect(struct peer *p)
{
    //p->drole = ++peer_id;
    return 0;
}

int misaka_disconnect(struct peer *p)
{
    return 0;
}

int misaka_init(void){
    return 0;
}

int misaka_deinit(void){
    return 0;
}
