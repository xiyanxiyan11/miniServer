#include "bgs.h"
#include "common.h"
#include "network.h"
#include "string.h"
#include "math.h"
#include "thpool.h"
#include "tasklist.h"

//config manger
struct global_config bgs_config;    //local config handle

//handle manger
struct global_servant bgs_servant;  //local servant handle    

int peer_debug_timer(struct thread* t);
struct stream *stream_clone_one(struct stream *from);
struct stream * stream_clone(struct stream *from);
int stream_count(struct stream *s);
void stream_dir_exchange(struct stream *s);
void dump_packet(struct stream *s);
int bgs_start_jitter ( int time );
void reverse_stream(struct stream *s, int size);
char *peer_uptime (time_t uptime2, char *buf, size_t len,int type);
int peer_old_time (time_t uptime2, int type);
time_t bgs_clock (int type);
void bgs_packet_add(struct stream_fifo* obuf, struct stream* s);
void bgs_packet_delete(struct stream_fifo *obuf);
void bgs_uptime_reset ( struct peer *peer );
int bgs_stop ( struct peer *peer );
int bgs_reconnect ( struct peer *peer );
int peer_delete(struct peer* peer);
struct peer* peer_new(void);
struct peer* peer_lookup_su(struct list *list, union sockunion *su);
struct peer* peer_lookup_dsu(struct list *list, union sockunion *dsu);
struct peer* peer_lookup_drole(struct list *list, int drole);
struct peer* peer_lookup_role(struct list *list, int role);
int sockunion_udp_socket (union sockunion *su);
int sockunion_tcp_socket (union sockunion *su);
int bgs_connect_success ( struct peer *peer );
int bgs_start(struct peer *peer);
int bgs_write_proceed(struct stream_fifo *obuf);
struct stream* bgs_write_packet(struct stream_fifo *obuf);
int read_io_action(int event, struct peer *peer);
int bgs_packet_route(struct stream *s);
struct stream * bgs_packet_process(struct stream *s, struct peer *peer);
void bgs_read(struct ev_loop *loop, struct ev_io *w, int events);
void bgs_write(struct ev_loop *loop, struct ev_io *handle, int events);

static struct event_handle *events[EVENT_MAX];

//alloc peer hash
void *phash_alloc_func(void *data){
    return data;
}

//register peer key val
void peer_register(struct peer *peer){
    hash_get(bgs_servant.peer_hash, (void *)peer, phash_alloc_func);
}

//unregister peer key val
void peer_unregister(struct peer *peer){
    hash_release(bgs_servant.peer_hash, (void *)peer);
}

//loookup peer in hash table
void *peer_lookup(struct peer *peer){
    hash_lookup(bgs_servant.peer_hash, (void *)peer);
} 

//just pop out this packet
int default_unpack(struct stream *s, struct peer *peer){
    return IO_PACKET;
}

//do nothing
int  default_pack(struct stream *s, struct peer *peer){
    return IO_PACKET;
}

void dump_peer(struct peer *peer){
    //debug_event(NULL);
    zlog_debug("peer id: %d\n", peer->id);
    zlog_debug("peer fd: %d\n", peer->fd);
    zlog_debug("peer su: \n");
    dump_sockunion(&peer->su);
    zlog_debug("peer su: \n");
    dump_sockunion(&peer->dsu);
    zlog_debug("peer path: %s\n", peer->path);
    zlog_debug("peer type: %d\n", peer->type);
    zlog_debug("peer mode: %d\n", peer->mode);
    zlog_debug("peer port: %d\n", ntohs(peer->port));
    zlog_debug("peer drole: %d\n", peer->drole);
    zlog_debug("peer role: %d\n",  peer->role);
    zlog_debug("peer quick: %d\n",  peer->quick);
    zlog_debug("peer reconnect: %d\n",  peer->reconnect);
    zlog_debug("peer listens: %d\n",  peer->listens);
    zlog_debug("peer send: %d\n",     peer->scount);
    zlog_debug("peer recive: %d\n",   peer->rcount);
    zlog_debug("peer obuf count: %d\n",   peer->obuf->count);
}

//color one packet
void stream_color_one(struct stream *to, struct stream *from){
        to->type = from->type;
        to->src = from->src; 
        to->dst = from->dst;
        
        to->mark= from->mark;         
        to->su   = from->su;
        to->dsu  = from->dsu;
}

//clone one not in deep
struct stream * stream_clone_one(struct stream *from){
        struct stream *to = stream_new(BGS_MAX_PACKET_SIZE);
        stream_color_one(to, from);
        stream_put(to, STREAM_PNT(from), stream_get_endp(from) - stream_get_getp(from));
        return to;
}

//clone list not in deep
struct stream * stream_clone(struct stream *from){
    struct stream *to = NULL;
    struct stream *head = NULL;
    struct stream *t;
    t = from;
    while(t){
        if(!head){
            head = stream_clone_one(t);
            to = head;
        }else{
            to->next = stream_clone_one(t);
            to= to->next;
        }
        t = t->next;
    }
    return head;
}

//color all packet
void stream_color(struct stream *to, struct stream *from){
    struct stream *t;
    int count = 0;
    t = to;
    while(t){
        stream_color_one(t, from);
        t = t->next;
    }
}

//count stream in list
int stream_count(struct stream *s){
    struct stream *t;
    int count = 0;
    t = s;
    while(t){
        ++count;
        t = t->next;
    }
    return count;
}

//process signal
void sighandle(int num){
        switch(num){
            case SIGINT:
            case SIGHUP:
                    exit(0);
                break;
            case SIGPIPE:
                    zlog_debug("signal pip\n");
                break;
            default:
                break;
        }
}

//exchage src and dst
void stream_dir_exchange(struct stream *s){
    int tmp;
    union sockunion tsu;	
    //reverse src and dst id
    tmp = s->src;
    s->src = s->dst;
    s->dst = tmp;

    //reverse su and dsu
    tsu = s->su;
    s->su = s->dsu;
    s->dsu = tsu;
}

//dump packet in 16 hex
void dump_hex(const char *prompt, unsigned char *buf, int len)
{
	int i = 0;

	for (i = 0; i < len; i++) {
		if ((i & 0x0f) == 0) {
			zlog_debug("%s | 0x%04x", prompt, i);
		}
		zlog_debug(" %02x", *buf);
		buf++;
	}
	zlog_debug("\n");
}

//dump sockunion
void dump_sockunion(union sockunion *su){
    char buf[200];
    sockunion2str (su, buf, 200);
    zlog_debug("packet address %s\n", buf);
}

//dump packet
void dump_packet(struct stream *s){
    zlog_debug("packet ptr  %p\n", s);
    zlog_debug("packet type %d\n",  s->type);
    zlog_debug("packet src  %d\n",  s->src);
    zlog_debug("packet dst  %d\n",  s->dst);
    zlog_debug("packet getp %d\n", s->getp);
    zlog_debug("packet endp  %d\n", s->endp);
    //dump_sockunion(&s->su);
    dump_sockunion(&s->dsu);
}

/* BGS start timer jitter. */
int bgs_start_jitter ( int time )
{
    return ( ( rand () % ( time + 1 ) ) - ( time / 2 ) );
}

//reverse header
void reverse_stream(struct stream *s, int size){
    char buf[BGS_HEADER_SIZE] = {0};          //empty buffer
    unsigned char *from, *to;
    int i = 0;
    /*reverse header*/
    stream_put(s, buf, size);

    from = STREAM_PNT(s) + stream_get_endp(s);
    to = from + size + stream_get_endp(s);
    for(i = 0; i < size; i++)
        *to-- = *from--;
}

/* Display peer uptime.*/
/* XXX: why does this function return char * when it takes buffer? */
char *peer_uptime (time_t uptime2, char *buf, size_t len,int type)
{
	time_t uptime1;
  	struct tm *tm;

  	/* Check buffer length. */
  	if (len < BGS_UPTIME_LEN)
        {
      		/* XXX: should return status instead of buf... */
      		snprintf (buf, len, "<error> "); 
      		return buf;
        }

  	/* If there is no connection has been done before print `never'. */
  	if (uptime2 == 0)
        {
      		snprintf (buf, len, "never   ");
      		return buf;
        }

  	/* Get current time. */
  	uptime1 = bgs_clock (type);
  	uptime1 -= uptime2;
  	tm = gmtime (&uptime1);

	/* Making formatted timer strings. */
#define ONE_DAY_SECOND 60*60*24
#define ONE_WEEK_SECOND 60*60*24*7

  	if (uptime1 < ONE_DAY_SECOND)
    		snprintf (buf, len, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	else if (uptime1 < ONE_WEEK_SECOND)
    		snprintf (buf, len, "%dd%02dh%02dm", tm->tm_yday, tm->tm_hour, tm->tm_min);
	else
    		snprintf (buf, len, "%02dw%dd%02dh", tm->tm_yday/7, tm->tm_yday - ((tm->tm_yday/7) * 7), tm->tm_hour);

	return buf;
}

//cal peer old time
 int peer_old_time (time_t uptime2, int type)
{
        //return 1;

	time_t uptime1;
  	struct tm *tm;
  	uptime1 = bgs_clock (type);
  	uptime1 -= uptime2;
  	tm = gmtime (&uptime1);
        return (tm->tm_hour > PEER_OLD_TIME);
}

//get clock
time_t bgs_clock (int type)
{
	struct timeval tv;
  	quagga_gettime(type, &tv);
  	return tv.tv_sec;
}

//add packet into send fifo
void bgs_packet_add(struct stream_fifo* obuf, struct stream* s)
{
	//Add packet to the end of list.
	stream_fifo_push(obuf, s);
}

//free first packet from fifo
void bgs_packet_delete(struct stream_fifo *obuf)
{
	stream_free(stream_fifo_pop(obuf));
}

//peer old  timer
void bgs_uptime_reset ( struct peer *peer )
{
    peer->uptime = time ( NULL );
}

//stop on peer 
int bgs_stop ( struct peer *peer )
{
	/* Increment Dropped count. */
    	if ( peer->status == TAT_ESTA)
    	{ 
		// set last reset time 
        	peer->resettime = time ( NULL );
        	//Reset uptime. 
        	bgs_uptime_reset ( peer );
		peer->status = TAT_IDLE;
	}
        
	//Stop all thread active
	if(1 == ev_is_active(peer->t_read))
	    ev_io_stop(peer->loop, peer->t_read);

	if(1 == ev_is_active(peer->t_write))
	    ev_io_stop(peer->loop, peer->t_write);
    	

    	if(1 == peer->quick){   
             zlog_debug("stop peer fd %d in quick\n", peer->fd);    	
    	}else{
             zlog_debug("stop peer fd %d in normal\n", peer->fd);    	
    	    if(1 == ev_is_active(peer->t_connect))
	        ev_periodic_stop(peer->loop, peer->t_connect);
        }

    	//Stream reset. 
    	peer->packet_size = 0;
	peer->uptime = 0;
	
    	// Clear input and output buffer.  
    	if ( peer->ibuf )
    	{
       	    stream_reset ( peer->ibuf );
	}
    	stream_fifo_clean (peer->obuf);

    	if ( peer->fd >= 0)
    	{
       	        close (peer->fd);
        	peer->fd = -1;
    	}
	return 0;
}

//udp reconnect or device reconnect 
int bgs_reconnect ( struct peer *peer )
{
    bgs_stop ( peer );
    bgs_start ( peer );
    return 0;
}

/* Peer comparison function for sorting.  */
int peer_cmp(void* vp1, void * vp2)
{
    struct peer *p1;
    struct peer *p2;
    p1 = (struct peer *)vp1;
    p2 = (struct peer *)vp2;
    if(p1->drole == p2->drole)
            return 1;
    return 0;
}

/* Key make function. */
unsigned int peer_key (void *vp){
    struct peer *p = (struct peer *)vp;
    return (unsigned int)p->drole;
}

/* Delete peer from confguration. */
int peer_delete(struct peer* peer)
{

        //remove this peer from peer_list
        listnode_delete(bgs_servant.peer_list,peer);

	//stop peer work
	bgs_stop(peer);

	/* Buffer.  */
	if (peer->ibuf)
	{
		stream_free(peer->ibuf);
		peer->ibuf = NULL;
	}
	if (peer->obuf)
	{
		stream_fifo_free(peer->obuf);
		peer->obuf = NULL;
	}
	/* Free allocated host character. */
	if (peer->host)
	{
		XFREE(MTYPE_STR, peer->host);
		peer->host = NULL;
	}
        
        //free io read handle
	if(peer->t_read)
	    free(peer->t_read);

        //free io write handle
	if(peer->t_write)
	    free(peer->t_write);

        //free io connect handle
	if(peer->quick){
	
	}else{
	    if(peer->t_connect)
	        free(peer->t_connect);
        }
	/* Free peer structure. */
	XFREE(MTYPE_BGS_PEER, peer);
	return 0;
}

// Allocate new peer object. 
struct peer* peer_new()
{
        static int count = 0;
	struct peer* peer;
	struct stream *s;
	
	/* Allocate new peer. */
	peer = XMALLOC(MTYPE_BGS_PEER, sizeof(struct peer));
	if (NULL == peer)
		return NULL;

	memset(peer, 0, sizeof(struct peer));

	listnode_add(bgs_servant.peer_list, peer);

	/* Set default value. */
	peer->fd = -1;
	peer->v_connect = BGS_DEFAULT_CONNECT_RETRY;

	peer->status = TAT_IDLE;

	/* Create buffers.  */
	peer->ibuf = stream_new(BGS_MAX_PACKET_SIZE);
	peer->ibuf->rlen = BGS_MAX_PACKET_SIZE;
	peer->packet_size = BGS_MAX_PACKET_SIZE;
	peer->obuf = stream_fifo_new();

        peer->read = NULL;
        peer->write = NULL;
        peer->start = NULL;
        peer->stop = NULL;
	peer->unpack = default_unpack;
	peer->pack  = default_pack;

        peer->quick = 0;
        peer->reconnect = 0;
	peer->maxcount = 50;
	peer->peer = NULL;
        
	/*init time*/
	bgs_uptime_reset(peer);
	
	/* Last read time set */
	peer->readtime = time(NULL);

	/* Last reset time set */
	peer->resettime = time(NULL);

	peer->t_read     = (struct ev_io    *)malloc(sizeof(struct ev_io));
	if(!peer->t_read)
	    return NULL;
	peer->t_read->data = peer;

	peer->t_write    = (struct ev_io    *)malloc(sizeof(struct ev_io));
	if(!peer->t_write)
	    return NULL;
	peer->t_write->data = peer;
	
    
        peer->rcount = 0;
        peer->scount = 0;
        peer->role = peer->drole = 0;

        //register loop handle
        peer->loop = bgs_servant.loop;

	return peer;
}

//look up su in list
struct peer* peer_lookup_su(struct list *list, union sockunion *su)
{
	struct peer* peer;
	struct listnode* nn;

	if(list == NULL)
		return NULL;

	LIST_LOOP(list, peer, nn)
	{
                if(!sockunion_cmp (&peer->su, su));
		        return peer;
	}
	return NULL;
}

//lookup dsu in list
struct peer* peer_lookup_dsu(struct list *list, union sockunion *dsu)
{
	struct peer* peer;
	struct listnode* nn;

	if(list == NULL)
		return NULL;
	
	LIST_LOOP(list, peer, nn)
	{
                if(!sockunion_cmp (&peer->dsu, dsu));
		        return peer;
	}
	return NULL;
}

//lookup the drole
struct peer* peer_lookup_drole(struct list *list, int drole)
{
	struct peer* peer;
	struct listnode* nn;

	if(list == NULL)
		return NULL;
	
	LIST_LOOP(list, peer, nn)
	{
	       if(peer->drole == drole) 
		    return peer;
	}
	return NULL;
}

//lookup the role
struct peer* peer_lookup_role(struct list *list, int role)
{
	struct peer* peer;
	struct listnode* nn;

	if(list == NULL)
		return NULL;
	
	LIST_LOOP(list, peer, nn)
	{
	       if(peer->role == role) 
		    return peer;
	}
	return NULL;
}

//init socket
int sockunion_create_socket (union sockunion *su)
{
  int sock;
  sock = socket (su->sa.sa_family, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      return -1;
    }

  return sock;
}

//action when connect success
int bgs_connect_success ( struct peer *peer )
{
        zlog_debug("bgs connect peer->fd: %d success\n", peer->fd);
	//set connect status;
  	peer->status = TAT_ESTA;				        //set establish flag
        
        //update time
	peer->uptime = bgs_clock (QUAGGA_CLK_MONOTONIC);
	

        if(set_nonblocking(peer->fd) <0 ){                             //set non block
            zlog_debug("set nonblock fail\n");
                    if(peer->fd > 0){
                        close(peer->fd);
                    }
                    peer->fd = 0;
                    
        }

        //register read
        if(1 != ev_is_active(peer->t_read)){
            ev_io_init(peer->t_read, bgs_read, peer->fd, EV_READ);
            peer->t_read->data = peer;
    	    ev_io_start(peer->loop, peer->t_read);
    	}

    	//register send event if stream in send fifo
        if(bgs_write_proceed(peer->obuf)){
            if(peer->status == TAT_ESTA && 1 != ev_is_active(peer->t_write))
                ev_io_init(peer->t_write, bgs_write, peer->fd, EV_WRITE);
                peer->t_read->data = peer;
                ev_io_start(peer->loop, peer->t_write);
        }
    	return 0;
}


//action when connect progress
int bgs_connect_progress ( struct peer *peer )
{

        zlog_debug("bgs connect progress\n");
	//set connect status;
	//set change stat by read
  	peer->status = TAT_IDLE;				        //set establish flag
        
        //update time
	peer->uptime = bgs_clock (QUAGGA_CLK_MONOTONIC);
	
        if(set_nonblocking(peer->fd) <0 ){                             //set non block
                    if(peer->fd > 0)
                        close(peer->fd);
                    peer->fd = 0;
        }
        
        if(1 != ev_is_active(peer->t_read)){
                ev_io_init(peer->t_read, bgs_read, peer->fd, EV_READ);
                peer->t_read->data = peer;
                ev_io_start(peer->loop, peer->t_read);
        }
    	return 0;
}

//callback connect return status 
void connect_status_trigger(int status, struct peer *peer){
        switch ( status )
    	{
        	case connect_success:	
        	        zlog_debug("connect in success\n");
        	        bgs_connect_success(peer);
        	        if(1 == ev_is_active(peer->t_connect));
        	            ev_periodic_stop(peer->loop, peer->t_connect);
        	        break;
        	case connect_in_progress:	
        	        zlog_debug("connect in progress\n");
                        bgs_connect_progress( peer);
        	        if(1 == ev_is_active(peer->t_connect));
        	            ev_periodic_stop(peer->loop, peer->t_connect);
                        break;
       	        case connect_error:
                default:
                        zlog_debug("connect error\n");
            		break;
    	}	
}

//action when after connect
void bgs_start_thread(struct ev_loop *loop, struct ev_periodic *handle, int events)
{
	int status;
	struct peer *peer;
        
        peer = (struct peer *)handle->data;

        if(NULL == peer)
                return;

        //check if establish
  	if (peer->status == TAT_ESTA)
        {
      		return;
        }

	//try to connect peer
	status = peer->start ( peer );

        //connect status check handle
        connect_status_trigger(status , peer);
    	return ;
}

//entry to active peer
int bgs_start(struct peer *peer){
        if(peer->quick){                //not connect, just build
            bgs_connect_success(peer);
        }else{
            //init timer here
	    peer->t_connect  = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
	    if(!peer->t_connect)
	        return -1;
	    peer->t_connect->data = peer;
            ev_periodic_init(peer->t_connect, bgs_start_thread, fmod (ev_now (peer->loop), RECONNECT_INTERVAL), RECONNECT_INTERVAL, 0);
            ev_periodic_start(peer->loop, peer->t_connect);
        }
        return 0;
}

/* Is there partially written packet or updates we can send right now.  */
int bgs_write_proceed(struct stream_fifo *obuf)
{
	if (stream_fifo_head(obuf))
		return 1;
	return 0;
}

/* get packet from write fifo*/
struct stream* bgs_write_packet(struct stream_fifo *obuf)
{
	struct stream* s = NULL;
	s = stream_fifo_head(obuf);
	if (s)
	{
		return s;
	}
	return NULL;
}

/* Write packet to the peer. */
void bgs_write(struct ev_loop *loop, struct ev_io *handle, int events)
{
	struct peer *peer;
	int count;
        
  	peer = (struct peer *)handle->data;
  	
  	if(!peer){
  	    zlog_debug("get empty peer from bgs_write\n");
  	    return;
  	    
  	}

        //zlog_debug("peer %d write is active %d\n", peer->drole, ev_is_active(peer->t_read));
  	//can't write when not connected
  	if (peer->status != TAT_ESTA)
        {
  	        zlog_debug("bgs peer fd %d not establish active\n", peer->fd);
      		return;
        }

        //record tot send packet
        count = peer->write(peer);  
        
        if(count < 0){
            zlog_debug("peer fd %d close in write process\n", peer->fd);
	    
	    //peer not healthy
	    if(peer->mode == MODE_PASSIVE){
                    peer->status = TAT_IDLE;
                    peer_unregister(peer);
		    peer_delete(peer);
            }else{
		    bgs_reconnect(peer);
	    }
            return;
        }

        peer->scount += count;

        //write again if packets in fifo
	if (bgs_write_proceed (peer->obuf))			
    	{
    	    if(1 != ev_is_active(handle)){
    	        ev_io_start(loop, handle);
    	    }
    	//need write again
	}else{
	    if(1 == ev_is_active(handle)){
	        ev_io_stop(loop, handle);
	    }
	}
	return;
}

//read io end, and choose action to process
int read_io_action(int event, struct peer *peer){
    static int tot = 0;

    struct stream *s = NULL;
    struct stream *rs = NULL;
    int ret = event;
    s = peer->ibuf;
    s->src = peer->drole;
    switch(ret){
        case IO_PACKET:
            s->flag = 0;   //mark as unused
            
            //@TODO redir for hahaha
#if 1
            s->dst = bgs_config.role;
            s->type = EVENT_ECHO;
#endif
            //clone this packet, and clean the old packet
            
            //send to it itsself, stolen it
            if(s->dst == bgs_config.role){
                //push stream into task list
#ifdef BGS_THREAD_SUPPORT
                    spinlock_lock(&bgs_servant.task_in->lock);
                        rs = stream_clone_one(s);
                        if(rs){
                            tasklist_push(bgs_servant.task_in, rs);
                        }else{
                        
                        }
                        stream_reset(s);
                    spinlock_unlock(&bgs_servant.task_in->lock);
#else
                    rs = stream_clone_one(s);
                    if(rs){
                        bgs_packet_process(rs, peer);  
                    }
                    stream_reset(s);
#endif
            }else{  //to others, just route it
                    rs = stream_clone_one(s);
                    if(rs){
                        bgs_packet_route(rs);
                    }
                    stream_reset(s);
            }
            break;
        case IO_PARTIAL: 
            break;
        case IO_ACCEPT:
            break;
        case IO_CLEAN:
            if (s)
                stream_reset(s);
            break;
        case IO_ERROR:   
            break;
        case IO_FULL:
            break;
        case IO_CLOSE:
            bgs_reconnect(peer);
            break;
        case IO_PASSIVE_CLOSE:
            zlog_debug("peer passive delete peer fd %d\n", peer->fd);
            //unregister peer in hash
            peer_unregister(peer);
            peer_delete(peer);
            break;
        default:
            break;
    }
    return 0;
}

/** @brief Starting point of packet process function. 
 *  @param[in] thread virtual thread handle 
 *
 */
void bgs_read(struct ev_loop *loop, struct ev_io *handle, int events)
{
    int length;
    int ret = 0;
    struct stream *s  = NULL;
    struct peer* peer;
    
    //get peer handle
    peer = (struct peer *)handle->data;

    if(NULL == peer){
            zlog_debug("empty peer get from bgs_read\n");
            return;
    }

    //zlog_debug("peer %d is read active %d\n", peer->drole, ev_is_active(peer->t_read));
    
    /*reset time*/
    bgs_uptime_reset(peer);			//up data read time

    peer->rcount++;

    if (peer->fd < 0)
    {
            zlog_debug("invalid peer can't read fd %d\n", peer->fd);
            return;
    }
    
    //real read, 
    ret = peer->read(peer);

    //check packet by unpack
    if(ret == IO_CHECK){

        s = peer->ibuf;
        if(peer->unpack){
            ret = peer->unpack(s, peer);
        }else{
            ret = IO_PACKET;
        }

        //connect in progress support
        if(peer->status != TAT_ESTA){
            
            //set status as establish
            peer->status = TAT_ESTA;
            
            if(bgs_write_proceed(peer->obuf)){
                if(peer->status == TAT_ESTA && 1 != ev_is_active(peer->t_write))
                    ev_io_init(peer->t_write, NULL, peer->fd, EV_WRITE);
                    peer->t_write->data = peer;
                    ev_io_start(peer->loop, peer->t_write);
            }
        }

    }
    read_io_action(ret, peer);
    return;
}

//process distribute here;
struct stream *  bgs_packet_process(struct stream *s, struct peer *peer)
{
    struct stream *curr = NULL;
    struct stream *t = NULL;
	
    struct event_handle* handle;
    struct listnode* nn;
    int type;

    type = s->type;

    //zlog_debug("packet process\n");
    //dump_packet(s);
    if(type <= EVENT_NONE || type >= EVENT_MAX)
            return NULL;

    LIST_LOOP(bgs_servant.event_list, handle, nn)
    {
            if(handle->type != type)
                    continue;
            if(handle->func){
                    handle->func(s);
            }
    }
    return NULL;
}

/*packet where to route to*/
int bgs_packet_route(struct stream *s){
    struct peer *peer = NULL;
    struct peer p;
    struct stream *rs, *ts, *tt;

    if(!s)
        return 0;

    //ignore empty packet
    if(stream_get_endp(s) - stream_get_getp(s) == 0){
        stream_free(s);
        return 0;
    }

    //lookup route peer
    //peer = peer_lookup_drole(bgs_servant.peer_list, s->dst);
    p.drole = s->dst;
    peer = (struct peer *)peer_lookup( (void *)&p);
        
    //drop it
    if(!peer){  
        zlog_debug("packet to unknown peer\n");
        stream_free(s);
        return -1;
    }

    ts = s;
    while(ts){
        tt = ts;
        ts = ts->next;
        tt->next = NULL;
        
        //pack this packet by callback
        peer->pack(tt, peer);

        stream_fifo_push (peer->obuf, tt);
        if(peer->status == TAT_ESTA && 1 != ev_is_active(peer->t_write)){
                    ev_io_init(peer->t_write, bgs_write, peer->fd, EV_WRITE);
                    peer->t_write->data = peer;
                    ev_io_start(peer->loop, peer->t_write);
        }else{
                //zlog_debug("bgs_write peer %d in running in route!!!\n", peer->drole);
        }
    }
    return 0;
}

int bgs_packet_thread_route(struct stream *s){
    spinlock_lock(&bgs_servant.task_out->lock);
    tasklist_push(bgs_servant.task_out, s);
    spinlock_unlock(&bgs_servant.task_out->lock);
    return 0;
}

//register task for evnet
int bgs_register_evnet( void (*func)(struct stream *), int type){
    struct event_handle *handle = (struct event_handle *)malloc(sizeof(struct event_handle));
    if(type <= EVENT_NONE || type >= EVENT_MAX)
            return -1;
    handle->func = func;
    handle->type = type;
    bgs_servant.event_list;  
    listnode_add(bgs_servant.event_list, handle);
    events[type] = handle;
    return 0;
}

//task distribue  handle
void bgs_task_distribute(struct ev_loop *loop, struct ev_periodic *handle, int events){
    struct stream *s = NULL;
    struct global_servant *servant   = (struct global_servant *)(handle->data);
    spinlock_lock(&bgs_servant.task_in->lock);
    {
    s = tasklist_pop(servant->task_in);
    if(s){
        //maybe push stream into fifo by user
        //start task thread  to load user func
        zlog_debug("distribute one packet to thread\n");
        if ( thpool_add_work(servant->thpool, (void *(*)(void*))bgs_packet_process, (void* )s) < 0)
                zlog_debug("thread create fail\n");
    }
    }
    spinlock_unlock(&bgs_servant.task_in->lock);
}

//task dispatch handle
void bgs_task_distpatch(struct ev_loop *loop, struct ev_periodic *handle, int events){
    struct stream *s = NULL;
    struct global_servant *servant   = (struct global_servant *)(handle->data);
    spinlock_lock(&bgs_servant.task_out->lock);
    {
    
    s = tasklist_pop(servant->task_out);
    if(s){
        bgs_packet_route(s);
    }
    
    }
    spinlock_unlock(&bgs_servant.task_out->lock);
}

//watch bgs status
void bgs_core_watch(struct ev_loop *loop, struct ev_periodic *handle, int events){
	struct peer* peer;
	struct listnode* nn;

        zlog_debug("##################################Watch#####################\n");
	LIST_LOOP(bgs_servant.peer_list, peer, nn)
	{
	        dump_peer(peer);
	        zlog_debug("\n");
	}
}

//init core 
int core_init(void)
{
        int i;
        INIT_DEBUG();
    	
    	//ignore pipe
   	signal(SIGPIPE,sighandle);
   	signal(SIGINT,sighandle);

#if 0
   	//init mem manager
   	void *mem = malloc(BGS_MEM_SIZE);
   	if(!mem){
   	    zlog_err("alloc mem for cache fail\n");
   	}

   	bgs_servant.kmem = init_kmem(mem, BGS_MEM_SIZE, BGS_MEM_ALIGN);

   	bgs_servant.stream_cache = kmem_cache_create(bgs_servant.kmem, "stream", sizeof(struct stream), BGS_MAX_STREAM);
   	if(bgs_servant.stream_cache){
   	    zlog_err("alloc cache for stream fail\n");
   	    return -1;
   	}

   	bgs_servant.data_cache   = kmem_cache_create(bgs_servant.kmem, "data",   BGS_MAX_PACKET_SIZE,   BGS_MAX_DATA);
   	if(bgs_servant.data_cache){
   	    zlog_err("alloc cache for data fail\n");
   	    return -1;
   	}

   	bgs_servant.peer_cache   =  kmem_cache_create(bgs_servant.kmem, "peer", sizeof(struct peer),    BGS_MAX_PEER);
   	if(bgs_servant.peer_cache){
   	    zlog_err("alloc cache for peer fail\n");
   	    return -1;
   	}
#endif

   	bgs_servant.loop = ev_default_loop(0);

	if (NULL == bgs_servant.loop)
	{
		zlog_debug("Create bgs_master faild!\r\n");
		return -1;
	}
        
        //init the peer list
	if( NULL == (bgs_servant.peer_list = list_new())){
		zlog_debug("Create peer_list failed!\r\n");
		return -1;
	}else{
	    bgs_servant.peer_list->cmp =  (int (*) (void *, void *)) peer_cmp;
	}

        //init hash peer
	bgs_servant.peer_hash = hash_create(peer_key, (int (*)(const void *, const void *))peer_cmp);

        //init the peer list
	if( NULL == (bgs_servant.event_list = list_new())){
		zlog_debug("Create event_list failed!\r\n");
		return -1;
	}else{
	    bgs_servant.event_list->cmp =  (int (*) (void *, void *)) peer_cmp;
	}

#ifdef BGS_THREAD_SUPPORT
        //init task in list
	if( NULL == (bgs_servant.task_in = tasklist_new())){
		zlog_debug("Create task_list in failed!\r\n");
		return -1;
	}

        //init task out list
	if( NULL == (bgs_servant.task_out = tasklist_new())){
		zlog_debug("Create task_list out failed!\r\n");
		return -1;
	}

	bgs_servant.t_distribute  = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
	if(!bgs_servant.t_distribute)
	    return -1;
	bgs_servant.t_distribute->data = &bgs_servant;

        ev_periodic_init(bgs_servant.t_distribute, bgs_task_distribute, fmod (ev_now (bgs_servant.loop), DISTRIBUTE_INTERVAL), DISTRIBUTE_INTERVAL, 0);
        ev_periodic_start(bgs_servant.loop, bgs_servant.t_distribute);

	bgs_servant.t_distpatch = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
	if(!bgs_servant.t_distpatch)
	    return -1;
	bgs_servant.t_distpatch->data = &bgs_servant;

        ev_periodic_init(bgs_servant.t_distpatch, bgs_task_distpatch, fmod (ev_now (bgs_servant.loop), DISPATCH_INTERVAL), DISTRIBUTE_INTERVAL, 0);
        ev_periodic_start(bgs_servant.loop, bgs_servant.t_distpatch);


	bgs_servant.thpool = thpool_init(BGS_THREAD_NUM);

	if(!bgs_servant.thpool)
	    return -1;
#endif
	bgs_servant.t_watch = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
	
       // ev_periodic_init(bgs_servant.t_watch, bgs_core_watch, fmod (ev_now (bgs_servant.loop), WATCH_INTERVAL), WATCH_INTERVAL, 0);
        //ev_periodic_start(bgs_servant.loop, bgs_servant.t_watch);
	
	if(!bgs_servant.t_watch)
	    return -1;
	bgs_servant.t_watch->data = &bgs_servant;
  	
  	return 0;
}

//core run here
int core_run(){
        zlog_info("proxy start\n");
	while(1){
	    ev_loop(bgs_servant.loop, 0);
	}
        zlog_info("proxy stop\n");
        return 0;
}
