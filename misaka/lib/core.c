#include "misaka.h"
#include "common.h"
#include "network.h"
#include "string.h"
#include "math.h"
#include "thpool.h"
#include "shm.h"
#include "dlfcn.h"
#include "skynet_mq.h"
#include "msg.h"

//config manger
struct global_config misaka_config;    //local config handle

//handle manger
struct global_servant misaka_servant;  //local servant handle    

int misaka_start_jitter ( int time );
char *peer_uptime (time_t uptime2, char *buf, size_t len,int type);
int peer_old_time (time_t uptime2, int type);
void misaka_packet_add(struct stream_fifo* obuf, struct stream* s);
void misaka_packet_delete(struct stream_fifo *obuf);
void peer_uptime_reset ( struct peer *peer );
int misaka_stop ( struct peer *peer );
int misaka_reconnect ( struct peer *peer );
int peer_delete(struct peer* peer);
struct peer* peer_new(void);
struct peer* peer_lookup_su(struct list *list, union sockunion *su);
struct peer* peer_lookup_dsu(struct list *list, union sockunion *dsu);
struct peer* peer_lookup_drole(struct list *list, int drole);
struct peer* peer_lookup_role(struct list *list, int role);
int sockunion_udp_socket (union sockunion *su);
int sockunion_tcp_socket (union sockunion *su);
int misaka_start_success ( struct peer *peer );
int misaka_start(struct peer *peer);
int misaka_write_proceed(struct stream_fifo *obuf);
struct stream* misaka_write_packet(struct stream_fifo *obuf);
int read_io_action(int event, struct peer *peer);
int misaka_packet_route(struct stream *s);
struct stream * misaka_packet_process(struct stream *s);
struct stream * misaka_packet_task_route(struct stream *s);
void misaka_read(struct ev_loop *loop, struct ev_io *w, int events);
void misaka_write(struct ev_loop *loop, struct ev_io *handle, int events);

struct event_handle events[EVENT_MAX];      //events callback
struct message_queue *queues[EVENT_MAX];    //events queue, handles by only thread

//register api for c
int register_c_event(const char *path, int type){
    struct event_handle *handle = NULL;
    handle = &events[type];
    handle->plug = C_PLUGIN_TYPE;
    snprintf(handle->path, MISAKA_PATH_SIZE, "%s", path);
    return 0;
}

//work used as thread handle for all event
void *worker(void *arg){
    struct stream *s;
    int type;
    struct message_queue *q = NULL;
    int sands;
    uint32_t handle;

    mlog_debug("thread start!\n");
    sands = 15;
    for(;;){
            q = skynet_globalmq_pop();
            if(!q){
                mlog_debug("thread start fail by empty queue pop!\n");
                break;
            }

            handle = skynet_mq_handle(q);
            
            //TODO loadbalance policy
            for(; sands && 0 == skynet_mq_pop(q, &s); --sands){
                if(handle != EVENT_NET){
                    mlog_debug("thread active handle %d (task) success!\n", handle);
                    misaka_packet_process(s);
                }else{
                    mlog_debug("thread active handle %d (net) error!\n", handle);
                }
            }
            if(0 == sands){
                mlog_debug("thread stop by sands!\n");
                skynet_globalmq_push(q);
                break;
            }else{
                mlog_debug("thread stop by empty pop!\n");
            }
    }
}

//alloc peer hash
void *peer_hashalloc_func(void *data){
    return data;
}

//register peer key val
void peer_register(struct peer *peer){
    //trigger when connected, usr must set peer id
    if (peer->on_connect){
        peer->on_connect(peer);
    }
    hash_get(misaka_servant.peer_hash, (void *)peer, peer_hashalloc_func);
}

//unregister peer key val
void peer_unregister(struct peer *peer){
    //trigger on disconnect
    if(peer->on_disconnect){
        peer->on_disconnect(peer);
    }
    hash_release(misaka_servant.peer_hash, (void *)peer);
}

//loookup peer in hash table
void *peer_lookup(struct peer *peer){
    return hash_lookup(misaka_servant.peer_hash, (void *)peer);
} 

//just pop out this packet
int peer_default_unpack(struct stream *s, struct peer *peer){
    return IO_PACKET;
}

//do nothing
int  peer_default_pack(struct stream *s, struct peer *peer){
    return IO_PACKET;
}

//dump peer info
void peer_dump(struct peer *peer){
    mlog_debug("peer id: %d\n", peer->id);
    mlog_debug("peer fd: %d\n", peer->fd);
    mlog_debug("peer su: \n");
    sockunion_dump(&peer->su);
    mlog_debug("peer su: \n");
    sockunion_dump(&peer->dsu);
    mlog_debug("peer path: %s\n", peer->path);
    mlog_debug("peer type: %d\n", peer->type);
    mlog_debug("peer mode: %d\n", peer->mode);
    mlog_debug("peer port: %d\n", ntohs(peer->port));
    mlog_debug("peer drole: %d\n", peer->drole);
    mlog_debug("peer role: %d\n",  peer->role);
    mlog_debug("peer quick: %d\n",  peer->quick);
    mlog_debug("peer reconnect: %d\n",  peer->reconnect);
    mlog_debug("peer listens: %d\n",  peer->listens);
    mlog_debug("peer send: %d\n",     peer->scount);
    mlog_debug("peer recive: %d\n",   peer->rcount);
}

//process signal
void sighandle(int num){
        switch(num){
            case SIGINT:
            case SIGHUP:
            case SIGKILL:
                    if(misaka_servant.shm){
                        shm_delete(misaka_servant.shm);
                        misaka_servant.shm = NULL;
                    }
                    exit(0);
                break;
            case SIGPIPE:
                    mlog_debug("signal pip\n");
                break;
            default:
                break;
        }
}

//peer old time
 int peer_old_time (time_t uptime2, int type)
{
	time_t uptime1;
  	struct tm *tm;
  	uptime1 = time(NULL);
  	uptime1 -= uptime2;
  	tm = gmtime (&uptime1);
        return (tm->tm_sec > PEER_OLD_TIME);
}

//peer update time of peer
void peer_uptime_reset ( struct peer *peer )
{
    peer->uptime = time ( NULL );
}

// peer comparison function for sorting.
int peer_cmp(void* vp1, void * vp2)
{
    struct peer *p1;
    struct peer *p2;
    p1 = (struct peer *)vp1;
    p2 = (struct peer *)vp2;
    return (p1->drole == p2->drole ? 1 : 0);
}

// Key make function. 
unsigned int peer_key (void *vp){
    struct peer *p = (struct peer *)vp;
    return (unsigned int)p->drole;
}

//add packet into send fifo
void misaka_packet_add(struct stream_fifo* obuf, struct stream* s)
{
    stream_fifo_push(obuf, s);
}

//free first packet from fifo
void misaka_packet_delete(struct stream_fifo *obuf)
{
    stream_free(stream_fifo_pop(obuf));
}

//stop on peer 
int misaka_stop ( struct peer *peer )
{
    	if (peer->status == TAT_ESTA)
    	{ 
            //Reset uptime. 
            peer_uptime_reset ( peer );
	    peer->status = TAT_IDLE;
	}
        
	//Stop all thread active
	if(1 == ev_is_active(peer->t_read))
	    ev_io_stop(peer->loop, peer->t_read);

	if(1 == ev_is_active(peer->t_write))
	    ev_io_stop(peer->loop, peer->t_write);
    	
    	if(1 == peer->quick){   
             mlog_debug("stop peer fd %d in quick\n", peer->fd);    	
    	}else{
             mlog_debug("stop peer fd %d in normal\n", peer->fd);    	
    	    if(1 == ev_is_active(peer->t_connect))
	        ev_periodic_stop(peer->loop, peer->t_connect);
        }

    	//Stream reset. 
    	peer->packet_size = 0;
	peer->uptime = 0;
	
    	// Clear input and output buffer.  
    	if (peer->ibuf)
    	{
       	    stream_reset ( peer->ibuf );
	}
    	stream_fifo_clean (peer->obuf);

    	if (peer->fd >= 0)
    	{
       	    close (peer->fd);
            peer->fd = -1;
    	}
            return 0;
}

//udp reconnect or device reconnect 
int misaka_reconnect ( struct peer *peer )
{
    misaka_stop ( peer );
    misaka_start ( peer );
    return 0;
}

//delete peer 
int peer_delete(struct peer* peer)
{
        //remove this peer from peer_list
        listnode_delete(misaka_servant.peer_list,peer);
    
	//stop peer work
	misaka_stop(peer);

	//clean in buffer
	if (peer->ibuf)
	{
		stream_free(peer->ibuf);
		peer->ibuf = NULL;
	}
        
        //clean out buffer
	if (peer->obuf)
	{
		stream_fifo_free(peer->obuf);
		peer->obuf = NULL;
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

	XFREE(MTYPE_MISAKA_PEER, peer);
	return 0;
}

//new peer 
struct peer* peer_new()
{
	struct peer* peer;
	struct stream *s;
	
	//allocate new peer. 
	peer = XMALLOC(MTYPE_MISAKA_PEER, sizeof(struct peer));
	if (NULL == peer)
		return NULL;

	memset(peer, 0, sizeof(struct peer));

	//set default value
	peer->fd = -1;
	peer->v_connect = MISAKA_DEFAULT_CONNECT_RETRY;

	peer->status = TAT_IDLE;

	//create read buffer
	peer->ibuf = stream_new(MISAKA_MAX_PACKET_SIZE);
	if(NULL == peer->ibuf){
            mlog_debug("alloc ibuf fail\n");
	    XFREE(MTYPE_MISAKA_PEER, peer);
	    return NULL;
        }

	peer->ibuf->rlen = MISAKA_MAX_PACKET_SIZE;
	peer->packet_size = MISAKA_MAX_PACKET_SIZE;

	peer->obuf = stream_fifo_new();
	if(NULL == peer->obuf){
            mlog_debug("alloc obuf fail\n");
	    stream_free(peer->ibuf);
	    XFREE(MTYPE_MISAKA_PEER, peer);
	    return NULL;
	}

        peer->read  = NULL;
        peer->write = NULL;
        peer->start = NULL;
        peer->stop  = NULL;
	peer->unpack = peer_default_unpack;
	peer->pack  =  peer_default_pack;
	peer->on_connect = NULL;
	peer->on_disconnect = NULL;

        peer->quick = 0;
        peer->reconnect = 0;
	peer->maxcount = 50;
	peer->peer = NULL;
        
	//update time for peer
	peer_uptime_reset(peer);
	
	//last read time set
	peer->readtime = time(NULL);

	//last reset time set
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
        peer->loop = misaka_servant.loop;
        
        //add peer into peer_list
	listnode_add(misaka_servant.peer_list, peer);
	return peer;
}

//lookup su in list
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
	if(NULL == list)
	    return NULL;
	LIST_LOOP(list, peer, nn)
	{
            if(!sockunion_cmp (&peer->dsu, dsu));
                return peer;
	}
	return NULL;
}

//action when connect success
int misaka_start_success ( struct peer *peer )
{
        mlog_debug("bgs connect peer->fd: %d success\n", peer->fd);
  	peer->status = TAT_ESTA;				        
        
        //update time
        peer_uptime_reset(peer);

        //register this peer
        peer_register(peer);

        //set link nonblock
        if(set_nonblocking(peer->fd) <0 ){                          
            mlog_debug("set nonblock fail\n");
            if(peer->fd > 0){
                close(peer->fd);
            }
            peer->fd = 0;
        }

        //register read
        if(1 != ev_is_active(peer->t_read)){
            ev_io_init(peer->t_read, misaka_read, peer->fd, EV_READ);
            peer->t_read->data = peer; 
    	    ev_io_start(peer->loop, peer->t_read);
    	}

    	//register write when packeted in fifo
        if(misaka_write_proceed(peer->obuf)){
            if(peer->status == TAT_ESTA && 1 != ev_is_active(peer->t_write))
                ev_io_init(peer->t_write, misaka_write, peer->fd, EV_WRITE);
                peer->t_read->data = peer;
                ev_io_start(peer->loop, peer->t_write);
        }
    	return 0;
}

//action when connect progress
int misaka_start_progress ( struct peer *peer )
{
        mlog_debug("bgs connect progress\n");
  	//set establish flag
  	peer->status = TAT_IDLE;		    
        
        //update time
        peer_uptime_reset(peer);
	
	//set peer as nonblock
        if(set_nonblocking(peer->fd) <0 ){                             
                    if(peer->fd > 0)
                        close(peer->fd);
                    peer->fd = 0;
        }
        
        //register read trigger
        if(1 != ev_is_active(peer->t_read)){
                ev_io_init(peer->t_read, misaka_read, peer->fd, EV_READ);
                peer->t_read->data = peer;
                ev_io_start(peer->loop, peer->t_read);
        }
    	return 0;
}

//connect status check
void connect_status_trigger(int status, struct peer *peer){
        switch ( status )
    	{
        	case connect_success:	
        	        mlog_debug("connect in success\n");
        	        misaka_start_success(peer);
        	        if(1 == ev_is_active(peer->t_connect));
        	            ev_periodic_stop(peer->loop, peer->t_connect);
        	        break;
        	case connect_in_progress:	
        	        mlog_debug("connect in progress\n");
                        misaka_start_progress( peer);
        	        if(1 == ev_is_active(peer->t_connect));
        	            ev_periodic_stop(peer->loop, peer->t_connect);
                        break;
       	        case connect_error:
                default:
                        mlog_debug("connect error\n");
            		break;
    	}	
}

//action when after connect
void misaka_start_thread(struct ev_loop *loop, struct ev_periodic *handle, int events)
{
	int status;
	struct peer *peer;
        peer = (struct peer *)handle->data;
        if(NULL == peer)
            return;
  	if (peer->status == TAT_ESTA)
        {
      	    return;
        }
	status = peer->start ( peer );
        connect_status_trigger(status , peer);
    	return ;
}

//entry to active peer
int misaka_start(struct peer *peer){
        if(peer->quick){                
            //not connect, just init peer
            misaka_start_success(peer);
        }else{
            //init connect timer here
	    peer->t_connect  = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
	    if(!peer->t_connect)
	        return -1;
	    peer->t_connect->data = peer;
            ev_periodic_init(peer->t_connect, misaka_start_thread, \
                    fmod (ev_now (peer->loop), RECONNECT_INTERVAL), RECONNECT_INTERVAL, 0);
            ev_periodic_start(peer->loop, peer->t_connect);
        }
        return 0;
}

//Is there partially written packet or updates we can send right now.  
int misaka_write_proceed(struct stream_fifo *obuf)
{
        return ( stream_fifo_head(obuf) ? 1 : 0 );
}

//get packet from write fifo
struct stream* misaka_write_packet(struct stream_fifo *obuf)
{
	struct stream* s = NULL;
	s = stream_fifo_head(obuf);
	if (s)
	{
	    return s;
	}
	return NULL;
}

//write packet to the peer. 
void misaka_write(struct ev_loop *loop, struct ev_io *handle, int events)
{
	struct peer *peer;
	int count;
        
  	peer = (struct peer *)handle->data;
  	
  	if(!peer){
  	    mlog_debug("get empty peer from misaka_write\n");
  	    return;
  	}

        //mlog_debug("peer %d write is active %d\n", peer->drole, ev_is_active(peer->t_read));
  	//can't write when not connected
  	if (peer->status != TAT_ESTA)
        {
  	        mlog_debug("bgs peer fd %d not establish active\n", peer->fd);
      		return;
        }

        //record tot send packet
        count = peer->write(peer);  

        if(count < 0){
            mlog_debug("peer fd %d close in write process\n", peer->fd);
	    
	    //peer not healthy
	    if(peer->mode == MODE_PASSIVE){
                    peer->status = TAT_IDLE;
                    peer_unregister(peer);
		    peer_delete(peer);
            }else{
		    misaka_reconnect(peer);
	    }
            return;
        }

  	mlog_debug("bgs write to peer fd %d!\n", peer->fd);

        peer->scount += count;

        //write again if packets in fifo
	if (misaka_write_proceed (peer->obuf))			
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
    mlog_debug("read io action trigger");
    switch(ret){
        case IO_PACKET:
            s->flag = 0;   //mark as unused
            //@TODO redir for echoserver test
#if 1
            s->dst = misaka_config.role;
            s->type = EVENT_ECHO;
#endif
            mlog_debug("io packet trigger\n");
            //send to it itsself, stolen it and push into queue
            if(s->dst == misaka_config.role){
                rs = stream_clone_one(s);
                mlog_debug("thread route packet prepare\n");
                if(rs && rs->type > EVENT_NONE && rs->type < EVENT_NET)
                {
                    mlog_debug("task route packet start\n");
                    misaka_packet_task_route(rs);  
                }
            }else{  
                //to others, just route it
                rs = stream_clone_one(s);
                if(rs){
                    //route packet
                    misaka_packet_route(rs);
                }
            }
            stream_reset(s);
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
            misaka_reconnect(peer);
            break;
        case IO_PASSIVE_CLOSE:
            mlog_debug("peer passive delete peer fd %d\n", peer->fd);
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
void misaka_read(struct ev_loop *loop, struct ev_io *handle, int events)
{
    int length;
    int ret = 0;
    struct stream *s  = NULL;
    struct peer* peer;
    //get peer handle
    peer = (struct peer *)handle->data;

    if(NULL == peer){
        mlog_debug("empty peer get from misaka_read\n");
        return;
    }

    peer_uptime_reset(peer);			//up data read time

    peer->rcount++;

    if (peer->fd < 0)
    {
        mlog_debug("invalid peer can't read fd %d\n", peer->fd);
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
            
            if(misaka_write_proceed(peer->obuf)){
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
struct stream *  misaka_packet_process(struct stream *s)
{
    struct stream *curr = NULL;
    struct stream *t = NULL;
    struct event_handle* handle;
    struct listnode* nn;
    int type;
    type = s->type;

    if(type <= EVENT_NONE || type >= EVENT_NET)
            return NULL;

    LIST_LOOP(misaka_servant.event_list, handle, nn)
    {
            if(handle->type != type)
                    continue;
            if(handle->func){
                    handle->func(s);
            }

            switch(s->type){
                case EVENT_NET:
                    misaka_packet_net_route(s);
                    break;
                case EVENT_NONE:
                    stream_free(s);
                    break;
                default:
                    misaka_packet_task_route(s);
                    break;
            }
    }
    return NULL;
}

//packet where to route to
int misaka_packet_route(struct stream *s){
    struct peer *peer = NULL;
    struct peer p;
    void *data;
    struct stream *rs, *ts, *tt;

    if(!s)
        return 0;

    //ignore empty packet
    if(stream_get_endp(s) - stream_get_getp(s) == 0){
        stream_free(s);
        return 0;
    }

    //lookup route peer
    p.drole = s->dst;
    peer = (struct peer *)peer_lookup( (void *)&p);
    
    //drop it
    if(!peer){  
        mlog_debug("packet to unknown peer\n");
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
                    ev_io_init(peer->t_write, misaka_write, peer->fd, EV_WRITE);
                    peer->t_write->data = peer;
                    ev_io_start(peer->loop, peer->t_write);
        }else{
                //mlog_debug("misaka_write peer %d in running in route!!!\n", peer->drole);
        }
    }
    return 0;
}

//route into queue and call thread
struct stream * misaka_packet_task_route(struct stream *s){
    struct message_queue *q;
    int type = s->type;
    q = queues[type];
    skynet_mq_push(q, &s);
    
    //@TODO better policy
    thpool_add_work(misaka_servant.thpool, worker, NULL);
    return NULL;
}

//route task out packet into queue, before send it
struct stream * misaka_packet_net_route(struct stream *s){
    struct message_queue *q;
    s->type = EVENT_NET;
    q = queues[EVENT_NET];
    //mark as 1, never push into global queue!!
    skynet_mq_global(q, 1);
    skynet_mq_push(q, &s);
    return NULL;
}

//route task out packet into queue, before send it
struct stream * misaka_packet_timer_route(struct stream *s){
    struct message_queue *q;
    s->type = EVENT_TIMER;
    q = queues[EVENT_TIMER];
    //mark as 1, never push into global queue!!
    skynet_mq_global(q, 1);
    skynet_mq_push(q, &s);
    return NULL;
}

//look route packet
void misaka_packet_loop_route(void){
    int sands = 5;
    struct stream *rs;

    struct listnode *nn;
    struct peer *dpeer; 

    int type = EVENT_NET;
    struct stream *s;
    struct message_queue *q;
    q = queues[type];
    for(; sands && 0 == skynet_mq_pop(q, &s); --sands){
        misaka_packet_route(s);
    }
}

//look get timer
void misaka_packet_loop_timer(void){
    int sands = 5;
    int type = EVENT_TIMER;
    struct stream *s;
    struct message_queue *q;
    q = queues[type];
    for(; sands && 0 == skynet_mq_pop(q, &s); --sands){
            //TODO timer action
    }
}

//register task for evnet
int misaka_register_event(int type){
    //get event handle
    void *thandle;
    struct event_handle *handle = &events[type];
    if(NULL == handle){
        mlog_debug("alloc handle for callback fail\n");
        return -1;
    }
    //check event handle type
    if(type < EVENT_NONE || type >= EVENT_MAX)
        return -1;

    //set handle type
    handle->type = type;

    //register api
    switch(handle->plug){
        case NONE_PLUGIN_TYPE:
            return 0;
        case C_PLUGIN_TYPE:
            {
                thandle = dlopen(handle->path, RTLD_NOW);
                if(!thandle){
                    mlog_debug("open so in path %s fail\n", handle->path);
                    return -1;
                }

                handle->func   = (void (*)(struct stream *))dlsym(thandle, "handle");
                handle->init   = (int(*)(void))dlsym(thandle, "init");
                handle->deinit = (int(*)(void))dlsym(thandle, "deinit");

                if( !handle->init || !handle->deinit|| !handle->deinit){
                    dlclose(handle);    
                    return -1;
                }
            }
            break;
        case LUA_PLUGIN_TYPE:
            break;
        default:
            return -1;
            break;
    }
    listnode_add(misaka_servant.event_list, handle);
    return 0;
}

//watch bgs status
void misaka_core_watch(struct ev_loop *loop, struct ev_periodic *handle, int events){
    struct peer* peer;
    struct listnode* nn;
    mlog_debug("##################################Watch#####################\n");
    LIST_LOOP(misaka_servant.peer_list, peer, nn)
    {
        peer_dump(peer);
        mlog_debug("\n");
    }
}

//watch bgs status
void misaka_loop_watch(struct ev_loop *loop, struct ev_periodic *handle, int events){
    misaka_packet_loop_route();
}

//watch bgs status
void misaka_loop_old(struct ev_loop *loop, struct ev_periodic *handle, int events){
        struct peer* peer; 
   	struct listnode* nn; 
        mlog_debug("try to old all link\n");
        //TODO better old function for user
        LIST_LOOP(misaka_servant.peer_list, peer, nn)
	{
	        if(!peer)
	            continue;

	        if(peer->old == 0)
	            continue;

	        if(peer_old_time(peer->uptime, 0)){
	            //delete old timer
	            peer_unregister(peer);
	            peer_delete(peer);
	        }
	        mlog_debug("\n");
	}
}

//init core 
int core_init(void)
{
        int i;
        void *mem;
    	
    	//ignore pipe
   	signal(SIGPIPE,sighandle);
   	signal(SIGINT,sighandle);

        memset(events, 0, sizeof(events));

   	misaka_servant.loop = ev_default_loop(0);

	if (NULL == misaka_servant.loop)
	{
		mlog_debug("Create misaka_master faild!\r\n");
		return -1;
	}
        
        //init the peer list
	if( NULL == (misaka_servant.peer_list = list_new())){
		mlog_debug("Create peer_list failed!\r\n");
		return -1;
	}else{
	    misaka_servant.peer_list->cmp =  (int (*) (void *, void *)) peer_cmp;
	}

        //init hash peer
	misaka_servant.peer_hash = hash_create(peer_key, (int (*)(const void *, const void *))peer_cmp);

        //init event list
	misaka_servant.event_list = list_new();
        
        //init the event list
	if( NULL == misaka_servant.event_list){
		mlog_debug("Create event_list failed!\r\n");
		return -1;
	}else{
	    misaka_servant.event_list->cmp =  (int (*) (void *, void *)) peer_cmp;
	}

        //init global queue
        skynet_mq_init();

        //create message queue 
        for(i = 0; i <= EVENT_MAX; i++){
            queues[i] = skynet_mq_create(i);
        }

        misaka_servant.thpool = thpool_init(MISAKA_THREAD_NUM);
	if(!misaka_servant.thpool)
	    return -1;
	
	//init watch timer
	misaka_servant.t_watch = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
        ev_periodic_init(misaka_servant.t_watch, misaka_loop_watch, \
                fmod (ev_now (misaka_servant.loop), WATCH_INTERVAL), WATCH_INTERVAL, 0);
        ev_periodic_start(misaka_servant.loop, misaka_servant.t_watch);
	
	//init old timer
	misaka_servant.t_old = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
        ev_periodic_init(misaka_servant.t_old, misaka_loop_old, \
                fmod (ev_now (misaka_servant.loop), OLD_INTERVAL), OLD_INTERVAL, 0);
        //ev_periodic_start(misaka_servant.loop, misaka_servant.t_old);

	if(!misaka_servant.t_watch)
	    return -1;

	misaka_servant.t_watch->data = &misaka_servant;
  	misaka_servant.t_old->data   = &misaka_servant;

  	return 0;
}

//core run here
int core_run(){
        int i;
        int ret;
        mlog_info("misaka start\n");
        
        //register all event
        for (i = 0; i < EVENT_MAX; i++){
            ret = misaka_register_event(i);
            if(ret < 0){
                mlog_info("register api %d fail with plugin %d\n", i, events[i].plug);
                return -1;
            }   
        }

	while(1){
	    ev_loop(misaka_servant.loop, 0);
	}
        mlog_info("misaka stop\n");
        return 0;
}
