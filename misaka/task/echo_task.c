#include "link.h"
#include "task.h"
#include "misaka.h"
#include <sys/time.h>
#include <math.h>

static int peer_id = 0;

//echo event, send this  packet back
void handle(struct stream *s)
{
    int tmp;
    tmp = s->src;
    s->src = s->dst;
    s->dst = s->src;
    s->type = EVENT_NET;
}

int init(void){
    return 0;
}

int deinit(void){
    return 0;
}
