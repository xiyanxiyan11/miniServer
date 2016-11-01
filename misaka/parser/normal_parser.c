#include <misaka.h>

/*payload format
|-----------------------------------------------------
|magic   |nid|src|dst|type|sid|seq|payloadLen|payloadData|
|-----------------------------------------------------
* 8 +      4 + 4 + 4 + 4 +  4 + 4 +   4       =   32
*/

#define MAGIC_STR "miku1"
#define MAGIC_STR_SIZE  (4)
#define MAGIC_HEADER_SIZE (32) 

//init parser stream
int normal_parser_init(struct peer *peer){
    peer->header_size = MAGIC_HEADER_SIZE;
    peer->body_size = 0;
    peer->packet_size = MAGIC_HEADER_SIZE;
    return 0;
}

//just pop out this packet
int normal_parser_unpack(struct stream *s, struct peer *peer){
    int length;
    if(peer->body_size == 0){
        length = stream_get_endp(s) - stream_get_getp(s);
        if(length < MAGIC_HEADER_SIZE){
            return IO_PARTIAL; 
        }else{
            if(0 != memcmp(STREAM_PNT(s), MAGIC_STR, MAGIC_STR_SIZE)){
                return IO_CLOSE;
            }
        }
        s->getp += MAGIC_STR_SIZE;
        s->nid = stream_getl(s);
        s->src = stream_getl(s);
        s->dst = stream_getl(s);
        s->type = stream_getl(s);
        s->sid = stream_getl(s);
        s->seq = stream_getl(s);
        length = stream_getl(s);
        peer->body_size = length;
        peer->packet_size = length;
        return IO_PARTIAL;
    }
    return IO_PACKET;
}

//reverse header ofr pack
int normal_parser_reverse(struct stream *s){
    s->getp +=  MAGIC_HEADER_SIZE;
    return 0;
}

//init thread header for the packet wait to send
int normal_parser_pack(struct stream *s, struct peer *peer){
    int body_size = stream_get_endp(s) - stream_get_getp(s);
    s->getp = 0;
    stream_put (s, MAGIC_STR, MAGIC_STR_SIZE);
    stream_putl(s, s->nid);
    stream_putl(s, s->src);
    stream_putl(s, s->dst);
    stream_putl(s, s->type);
    stream_putl(s, s->sid);
    stream_putl(s, s->seq);
    stream_putl(s, body_size);
    s->getp = 0;
    return IO_PACKET;
}
