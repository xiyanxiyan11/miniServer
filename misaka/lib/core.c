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
#include "timer.h"

 //local config handle
struct global_config misaka_config;   

//local servant handle    
struct global_servant misaka_servant;  

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
struct stream * misaka_packet_sys_route(struct stream *s);

//events callback
struct event_handle events[EVENT_MAX];   
//events queue, handles by only thread
struct message_queue *queues[EVENT_MAX];    

//register api from c
int register_c_event(const char *path, int type){
    struct event_handle *handle = NULL;
    handle = &events[type];
    handle->plug = C_PLUGIN_TYPE;
    snprintf(handle->path, MISAKA_PATH_SIZE, "%s", path);
    return 0;
}

//register api from lua
int register_lua_event(const char *path, int type){
    struct event_handle *handle = NULL;
    handle = &events[type];
    handle->plug = LUA_PLUGIN_TYPE;
    snprintf(handle->path, MISAKA_PATH_SIZE, "%s", path);
    return 0;
}

//register api from python
int register_python_event(const char *path, int type){
    struct event_handle *handle = NULL;
    handle = &events[type];
    handle->plug = PYTHON_PLUGIN_TYPE;
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

    //mlog_debug("thread start!");
    sands = MISAKA_TASK_SANDS;
    for(;;){
            q = skynet_globalmq_pop();
            if(!q){
                //mlog_debug("thread start fail by empty queue pop!");
                break;
            }
            handle = skynet_mq_handle(q);
            
            //TODO loadbalance policy
            for(; sands && 0 == skynet_mq_pop(q, &s); --sands){
                if(handle != EVENT_NET){
                    //mlog_debug("thread active handle %d (task) success!", handle);
                    misaka_packet_process(s);
                }else{
                    //mlog_debug("thread active handle %d (net) error!", handle);
                }
            }
            if(0 == sands){
                //mlog_debug("thread stop by sands!");
                skynet_globalmq_push(q);
                break;
            }else{
                //mlog_debug("thread stop by empty pop!");
            }
    }
}

//alloc peer hash
void *peer_hashalloc_func(void *data){
    return data;
}

//register peer key val
void peer_register(struct peer *peer){
    int flag;
    struct event_handle *handle;
    struct listnode* nn; 
    peer->drole = idmaker_get(misaka_servant.mid, &flag);
    //trigger when connected, usr must set peer id
    if (peer->on_connect){
	LIST_LOOP(misaka_servant.event_list , handle, nn)
	{
	    mlog_debug("register handle %d create peer %d", handle->type, peer->drole);
	    if(handle->connect)
	        handle->connect(peer);
	}
    }
    hash_get(misaka_servant.peer_hash, (void *)peer, peer_hashalloc_func);
}

//unregister peer key val
void peer_unregister(struct peer *peer){
    //trigger on disconnect
    struct event_handle *handle;
    struct listnode* nn; 
    idmaker_put(misaka_servant.mid, peer->drole);
    if (peer->on_disconnect){
	LIST_LOOP(misaka_servant.event_list, handle, nn)
	{
	    if(handle->disconnect)
	        handle->disconnect(peer);
	}
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
    mlog_debug("peer id: %d", peer->id);
    mlog_debug("peer fd: %d", peer->fd);
    mlog_debug("peer su: ");
    sockunion_dump(&peer->su);
    mlog_debug("peer su: ");
    sockunion_dump(&peer->dsu);
    mlog_debug("peer path: %s", peer->path);
    mlog_debug("peer type: %d", peer->type);
    mlog_debug("peer mode: %d", peer->mode);
    mlog_debug("peer port: %d", ntohs(peer->port));
    mlog_debug("peer drole: %d", peer->drole);
    mlog_debug("peer role: %d",  peer->role);
    mlog_debug("peer quick: %d",  peer->quick);
    mlog_debug("peer reconnect: %d",  peer->reconnect);
    mlog_debug("peer listens: %d",  peer->listens);
    mlog_debug("peer send: %d",     peer->scount);
    mlog_debug("peer recive: %d",   peer->rcount);
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
                    mlog_debug("signal pip");
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
             mlog_debug("stop peer fd %d in quick", peer->fd);    	
    	}else{
             mlog_debug("stop peer fd %d in normal", peer->fd);    	
    	    if(1 == ev_is_active(peer->t_connect))
	        ev_periodic_stop(peer->loop, peer->t_connect);
        }

    	//Stream reset. 
    	peer->packet_size = 0;
	peer->uptime = 0;

        peer->disparser(peer);
	
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
            mlog_debug("alloc ibuf fail");
	    XFREE(MTYPE_MISAKA_PEER, peer);
	    return NULL;
        }

	peer->ibuf->rlen = MISAKA_MAX_PACKET_SIZE;
	peer->packet_size = MISAKA_MAX_PACKET_SIZE;

	peer->obuf = stream_fifo_new();
	if(NULL == peer->obuf){
            mlog_debug("alloc obuf fail");
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
	peer->on_connect = 0;
	peer->on_disconnect = 0;

        peer->quick = 0;
        peer->reconnect = 0;
	peer->maxcount = 50;
	peer->peer = NULL;
        peer->body_size = 0;

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
        mlog_debug("bgs connect peer->fd: %d success", peer->fd);
  	peer->status = TAT_ESTA;				        
        
        //update time
        peer_uptime_reset(peer);

        //register this peer
        peer_register(peer);

        //set link nonblock
        if(set_nonblocking(peer->fd) <0 ){                          
            mlog_debug("set nonblock fail");
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
        //mlog_debug("bgs connect progress");
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
        	        mlog_debug("connect in success");
        	        misaka_start_success(peer);
        	        if(1 == ev_is_active(peer->t_connect));
        	            ev_periodic_stop(peer->loop, peer->t_connect);
        	        break;
        	case connect_in_progress:	
        	        mlog_debug("connect in progress");
                        misaka_start_progress( peer);
        	        if(1 == ev_is_active(peer->t_connect));
        	            ev_periodic_stop(peer->loop, peer->t_connect);
                        break;
       	        case connect_error:
                default:
                        mlog_debug("connect error");
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

//action when timer register
void misaka_timer_thread(struct ev_loop *loop, struct ev_periodic *handle, int events)
{
	int status;
	int tmp;
	struct stream *s;
        s = (struct stream *)handle->data;
        if(NULL == s)
            return;
            
        tmp = s->stype;
        s->stype = s->type;
        s->type = tmp;
        
        //route the packet to the task
        misaka_packet_task_route(s);
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
        peer->parser(peer);
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
  	    mlog_debug("get empty peer from misaka_write");
  	    return;
  	}

        //mlog_debug("peer %d write is active %d", peer->drole, ev_is_active(peer->t_read));
  	//can't write when not connected
  	if (peer->status != TAT_ESTA)
        {
  	        //mlog_debug("bgs peer fd %d not establish active", peer->fd);
      		return;
        }

        //record tot send packet
        count = peer->write(peer);  

        if(count < 0){
            //mlog_debug("peer fd %d close in write process", peer->fd);
	    
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

  	//mlog_debug("bgs write to peer fd %d!", peer->fd);
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
    //mlog_debug("read io action packet from %d", s->src);
    switch(ret){
        case IO_PACKET:
            s->flag = 0;   //mark as unused
            //@TODO redir for echoserver test
#if 1
            s->dst = misaka_config.role;
            s->type = EVENT_ECHO;
#endif
            //mlog_debug("io packet trigger");
            //send to it itsself, stolen it and push into queue
            if(s->nid == misaka_config.role){
                rs = stream_clone_one(s);
                //mlog_debug("thread route packet prepare");
                if(rs && rs->type > EVENT_NONE && rs->type < EVENT_NET)
                {
                    //mlog_debug("task route packet start");
                    
                    //system packet route here!!!
                    if(peer->sys){
                        misaka_packet_sys_route(rs);
                    }else{
                        misaka_packet_task_route(rs);  
                    }
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
            peer->parser(peer);
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
        //mlog_debug("empty peer get from misaka_read");
        return;
    }

    peer_uptime_reset(peer);			//up data read time

    peer->rcount++;

    if (peer->fd < 0)
    {
        mlog_debug("invalid peer can't read fd %d", peer->fd);
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
    const char *str;
    char payload[MISAKA_MAX_PACKET_SIZE];
    size_t len;
    int ret;
    int type;
    type = s->type;

    if(type <= EVENT_NONE || type >= EVENT_NET)
            return NULL;

    LIST_LOOP(misaka_servant.event_list, handle, nn)
    {
            if(handle->type != type)
                    continue;
            
            switch(handle->plug){
                case C_PLUGIN_TYPE:
                    if(handle->func){
                        handle->func(s);
                    }
                    break;

                case LUA_PLUGIN_TYPE:
                    {
                    
                    //mlog_info("action handle in lua!");
                    //mlog_info("lua call type %d", s->type);
                    //mlog_info("lua call src %d", s->src);
                    //mlog_info("lua call dst %d", s->dst);
                    //call function
                    lua_getglobal(handle->lhandle, "misaka_handle");
                    lua_pushinteger(handle->lhandle, s->type);
                    lua_pushinteger(handle->lhandle, s->src);
                    lua_pushinteger(handle->lhandle, s->dst);
                    //set 0 in tail
                    *(STREAM_PNT(s) + stream_get_endp(s)) = 0;
                    lua_pushstring(handle->lhandle, STREAM_PNT(s));

                    //mlog_info("call lua start");
                    ret = lua_pcall(handle->lhandle, 4, 4, 0);
                    
                    if(ret != 0){
                        mlog_info("call lua fail");
                        stream_reset(s);
                        return s;
                    }

                    stream_reset(s);

                    str = lua_tolstring (handle->lhandle, -1, &len);
                    stream_put(s, (void*)str, len);
                    lua_pop(handle->lhandle, 1);

                    s->dst = lua_tointeger(handle->lhandle,  -1);
                    //mlog_info("lua return dst %d", s->dst);
                    lua_pop(handle->lhandle, 1);

                    s->src = lua_tointeger(handle->lhandle,  -1);
                    //mlog_info("lua return src %d", s->src);
                    lua_pop(handle->lhandle, 1);
                    
                    s->type = lua_tointeger(handle->lhandle, -1);
                    //mlog_info("lua return type", s->type);
                    lua_pop(handle->lhandle, 1);
                    
                    }
                    break;
                case PYTHON_PLUGIN_TYPE:
                    {
                        PyObject * pyParams = PyTuple_New(4);
                        PyTuple_SetItem(pyParams, 0, Py_BuildValue("i", s->type));
                        PyTuple_SetItem(pyParams, 1, Py_BuildValue("i", s->src));
                        PyTuple_SetItem(pyParams, 2, Py_BuildValue("i", s->dst));
                        *(STREAM_PNT(s) + stream_get_endp(s)) = 0;

                        mlog_info("stream payload %s", STREAM_PNT(s));

                        PyTuple_SetItem(pyParams, 3, Py_BuildValue("s", STREAM_PNT(s)));
                        PyObject * pyValue = PyObject_CallObject(handle->pFunc, pyParams);
                        stream_reset(s);
                        PyArg_ParseTuple(pyValue, "i|i|i|s", &s->type, &s->src, &s->dst, payload);
                        mlog_info("return type %d, src %d, dst %d, %sfrom python", s->type, s->src, s->dst, payload);
                        stream_put(s, (void*)payload, strlen(payload) + 1);
                    }
                    break;
                default:
                    break;
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

    if(!s){
        return 0;
    }

    //ignore empty packet
    if(stream_get_endp(s) - stream_get_getp(s) == 0){
        stream_free(s);
        return 0;
    }

    //lookup route peer
    p.drole = s->nid;
    peer = (struct peer *)peer_lookup( (void *)&p);
    
    //drop it
    if(!peer){  
        mlog_debug("packet to unknown peer %d", s->dst);
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
                //mlog_debug("misaka_write peer %d in running in route!!!", peer->drole);
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
    thpool_add_work(misaka_servant.thpool, worker, NULL);
    return NULL;
}

//route task out packet into queue, before send it
struct stream * misaka_packet_net_route(struct stream *s){
    struct message_queue *q;
    s->type = EVENT_NET;
    q = queues[EVENT_NET];
    skynet_mq_global(q, 1);
    skynet_mq_push(q, &s);
    return NULL;
}

//route task out packet into queue, before send it
struct stream * misaka_packet_sys_route(struct stream *s){
    struct message_queue *q;
    s->type = EVENT_SYS;
    skynet_mq_global(q, 1);
    skynet_mq_push(q, &s);
    return NULL;
}

//look route packet
void misaka_packet_loop_route(void){
    int sands = MISAKA_ROUTE_SANDS;
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

//loop timer
void misaka_packet_loop_timer(void){
    int sands = MISAKA_TIMER_SANDS;
    int type;
    int tmp;
    struct stream *s;
    struct message_queue *q;
	
    type = EVENT_SYS;
    q = queues[type];
    for(; sands && 0 == skynet_mq_pop(q, &s); --sands){
        //try register timer event 
        tmp = s->type;
        s->type = s->stype;
        s->stype = tmp; 
        server_timer_timeout(s->interval, s);
    }
    //try to old all timer
    server_timer_updatetime();
}

//register task for evnet
int misaka_load_event(int type){
    //get event handle
    int ret;
    void *tchandle;
    PyObject *pDict = NULL;
    lua_State *tlhandle = NULL;
    struct event_handle *handle = &events[type];
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
                tchandle = dlopen(handle->path, RTLD_NOW);
                if(!tchandle){
                    mlog_debug("open so in path %s fail", handle->path);
                    return -1;
                }
                handle->func   = (void (*)(struct stream *))dlsym(tchandle, "misaka_handle");
                handle->init   = (int(*)(void))dlsym(tchandle, "misaka_init");
                handle->deinit = (int(*)(void))dlsym(tchandle, "misaka_deinit");
                handle->connect = (int(*)(struct peer *))dlsym(tchandle, "misaka_connect");
                handle->disconnect = (int(*)(struct peer *))dlsym(tchandle, "misaka_disconnect");
                if( !handle->init || !handle->deinit|| !handle->deinit || !handle->connect || !handle->disconnect){
                    dlclose(tchandle);  
                    handle->chandle = NULL;
                    return -1;
                }
                handle->chandle = tchandle;

                handle->init();         
                //handle->deinit();
            }
            break;
        case LUA_PLUGIN_TYPE:
            {
                tlhandle = lua_open();
                
                //open libs
                luaL_openlibs(tlhandle); 
                luaopen_base(tlhandle);
                luaopen_string(tlhandle);
                luaopen_math(tlhandle);
                
                //open lua file
                luaL_dofile(tlhandle, handle->path);
                handle->lhandle = tlhandle;

                handle->init   = NULL;      //lua init by lua
                handle->deinit = NULL;
                handle->connect = NULL;     //lua ignore connect and disconnect
                handle->disconnect = NULL;

                lua_getglobal(handle->lhandle, "misaka_init");
                ret = lua_pcall(handle->lhandle, 0, 1, 0);
                    
                if(ret != 0){
                        mlog_info("call lua init fail");
                }
                ret = lua_tointeger (handle->lhandle, -1);
                lua_pop(handle->lhandle, 1);
            }
            break;
        case PYTHON_PLUGIN_TYPE:
            {
                PyRun_SimpleString("import sys");
                PyRun_SimpleString("sys.path.append('./')");
                PyRun_SimpleString("sys.path.append('/')");
                handle->pModule = PyImport_ImportModule(handle->path);
                if(!handle->pModule){
                    mlog_info("load module load fail");
                    return -1;
                }

                pDict = PyModule_GetDict(handle->pModule);
                if(!pDict){
                    mlog_info("load module load fail");
                    return -1;
                }
                
                handle->pFunc = PyDict_GetItemString(pDict, "misaka_handle");  
                if ( !handle->pFunc || !PyCallable_Check(handle->pFunc) ) {  
                        mlog_info("can't find function [misaka_handle]");  
                        return -1;  
                }  

                handle->pInit = PyDict_GetItemString(pDict, "misaka_init");  
                if ( !handle->pInit || !PyCallable_Check(handle->pInit) ) {  
                        mlog_info("can't find function [misaka_init]");  
                        return -1;  
                }  

                handle->pDeinit = PyDict_GetItemString(pDict, "misaka_deinit");  
                if ( !handle->pDeinit || !PyCallable_Check(handle->pDeinit) ) {  
                        mlog_info("can't find function [misaka_deinit]");  
                        return -1;  
                }  
                PyObject * pyValue = PyObject_CallObject(handle->pInit, NULL);
                PyArg_ParseTuple(pyValue, "i", &ret);
                mlog_info("function [misaka_init] return ret %d", ret);  
            }
            break;
        default:
            return -1;
            break;
    }
    listnode_add(misaka_servant.event_list, handle);
    return 0;
}

//register task for evnet
int misaka_disload_event(int type){
    //get event handle
    struct event_handle *handle = &events[type];
    //check event handle type
    if(type < EVENT_NONE || type >= EVENT_MAX)
        return -1;

    //register api
    switch(handle->plug){
        case NONE_PLUGIN_TYPE:
            return 0;
        case C_PLUGIN_TYPE:
            {
                handle->func   = NULL;
                handle->init   = NULL;
                handle->deinit = NULL;
                handle->connect = NULL;
                handle->disconnect = NULL;
                dlclose(handle->chandle);
                handle->chandle = NULL;
            }
            break;
        case LUA_PLUGIN_TYPE:
            {
                 lua_close(handle->lhandle);
                 handle->lhandle = NULL;
            }
            break;
        case PYTHON_PLUGIN_TYPE:
            {
                //@TODO python handle close callback
            }
            break;
        default:
            return -1;
            break;
    }
    listnode_delete(misaka_servant.event_list, handle);
    return 0;
}

//watch bgs status
void misaka_core_watch(struct ev_loop *loop, struct ev_periodic *handle, int events){
    struct peer* peer;
    struct listnode* nn;
    LIST_LOOP(misaka_servant.peer_list, peer, nn)
    {
        peer_dump(peer);
        mlog_debug("");
    }
}

//watch bgs status
void misaka_loop_watch(struct ev_loop *loop, struct ev_periodic *handle, int events){
    misaka_packet_loop_route();
}

//watch bgs status
void misaka_loop_sys(struct ev_loop *loop, struct ev_periodic *handle, int events){
    misaka_packet_loop_timer();
}

//init core 
int core_init(void)
{
        int i;
        void *mem;
        Py_Initialize();
    	//ignore pipe
   	signal(SIGPIPE,sighandle);
   	signal(SIGINT,sighandle);

        memset(events, 0, sizeof(events));

   	misaka_servant.loop = ev_default_loop(0);

	if (NULL == misaka_servant.loop)
	{
	    mlog_debug("Create misaka_master faild!");
	    return -1;
	}

	//register timer manager
	server_timer_init();
        
        //init the peer list
	if( NULL == (misaka_servant.peer_list = list_new())){
	    mlog_debug("Create peer_list failed!");
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
		mlog_debug("Create event_list failed!");
		return -1;
	}else{
	    misaka_servant.event_list->cmp =  (int (*) (void *, void *)) peer_cmp;
	}

        misaka_servant.mid = idmaker_new(MISAKA_MAX_NODE, MISAKA_MAX_ID);
        if (!misaka_servant.mid){
	    mlog_debug("Create id maker failed!");
            return -1;
        }

        //init global queue
        skynet_mq_init();

        //create message queue 
        for(i = 0; i < EVENT_MAX; i++){
            queues[i] = skynet_mq_create(i);
        }

        misaka_servant.thpool = thpool_init(MISAKA_THREAD_NUM);
	if(!misaka_servant.thpool)
	    return -1;
	
	//init net timer
	misaka_servant.t_watch = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
        ev_periodic_init(misaka_servant.t_watch, misaka_loop_watch, \
                fmod (ev_now (misaka_servant.loop), WATCH_INTERVAL), WATCH_INTERVAL, 0);
        ev_periodic_start(misaka_servant.loop, misaka_servant.t_watch);

	//init sys timer
	misaka_servant.t_old = (struct ev_periodic *)malloc(sizeof(struct ev_periodic));
        ev_periodic_init(misaka_servant.t_old, misaka_loop_sys, \
                fmod (ev_now (misaka_servant.loop), OLD_INTERVAL), OLD_INTERVAL, 0);
        ev_periodic_start(misaka_servant.loop, misaka_servant.t_old);

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
        mlog_info("misaka start");
        
        //load all event
        for (i = 0; i < EVENT_MAX; i++){
            ret = misaka_load_event(i);
            if(ret < 0){
                mlog_info("register api %d fail with plugin %d", i, events[i].plug);
                return -1;
            }   
        }

	while(1){
	    ev_loop(misaka_servant.loop, 0);
	}

	Py_Finalize();
        mlog_info("misaka stop");
        return 0;
}
