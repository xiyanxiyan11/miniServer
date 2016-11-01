#include <misaka.h>

/*payload format
|-----------------------------------------------------
|magic   |nid|dst|type|sid|seq|payloadLen|payloadData|
|-----------------------------------------------------
* 8 +      4 + 4 + 4 +  4 + 4 +   4       =   32
*/

#define MAGIC_STR "misaka01"
#define MAGIC_STR_SIZE  (8)
#define MAGIC_HEADER_SIZE (32) 

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

//@TODO no pack, write header when writer trigger
int  normal_parser_pack(struct stream *s, struct peer *peer){
    return IO_PACKET;
}
