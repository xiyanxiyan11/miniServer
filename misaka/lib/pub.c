#include "zebra.h"
#include "stream.h"
#include "sockunion.h"
#include "log.h"

//dump sockunion
void sockunion_dump(union sockunion *su){
    char buf[200];
    sockunion2str (su, buf, 200);
    mlog_debug("packet address %s\n", buf);
}

//dump packet
void stream_dump(struct stream *s){
    mlog_debug("packet ptr  %p\n",  s);
    mlog_debug("packet type %d\n",  s->type);
    mlog_debug("packet src  %d\n",  s->src);
    mlog_debug("packet dst  %d\n",  s->dst);
    mlog_debug("packet getp %d\n",  s->getp);
    mlog_debug("packet endp  %d\n", s->endp);
    sockunion_dump(&s->dsu);
}

//dump hex
void hex_dump(const char *prompt, unsigned char *buf, int len)
{
	int i = 0;

	for (i = 0; i < len; i++) {
		if ((i & 0x0f) == 0) {
			mlog_debug("%s | 0x%04x", prompt, i);
		}
		mlog_debug(" %02x", *buf);
		buf++;
	}
	mlog_debug("\n");
}
