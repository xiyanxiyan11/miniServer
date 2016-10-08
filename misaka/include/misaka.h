#ifndef __MISAKA_H_
#define __MISAKA_H_

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <sys/un.h>
#include <pthread.h>
#include "zebra.h"
#include "ev.h"

#include "memory.h"
#include "stream.h"
#include "memtypes.h"
#include "linklist.h"
#include "hash.h"
#include "thpool.h"
#include "log.h"
#include "idmaker.h"

#include "spinlock.h"
#include "kmem.h"
#include "pub.h"
#include "msg.h"

//#include "lua.h"
//#include "lualib.h"
//#include "lauxlib.h"

#define	MISAKA_UPTIME_LEN	               25
#define MISAKA_MAX_QUEUE_PACKET                12
#define	MISAKA_WRITE_PACKET_MAX	               30

#define MISAKA_MEM_SIZE                        (1024*1024 * 500)
#define MISAKA_SHM_KEY                         1234
#define MISAKA_MEM_ALIGN                       8
#define MISAKA_MAX_PEER                        (10000)
#define MISAKA_MAX_STREAM                      (10000*6)
#define MISAKA_MAX_FIFO                        (10000)
#define MISAKA_MAX_DATA                        (MISAKA_MAX_STREAM)

//support thread
#define MISAKA_THREAD_NUM                        (7)
#define MISAKA_THREAD_SUPPORT                    (1)
#define MISAKA_THREAD_SANDS                      (16)

//support data cache
//#define MISAKA_CACHE_SUPPORT                    1


#define MISAKA_INIT_START_TIMER  		5
#define MISAKA_DEFAULT_HOLDTIME               	180
#define MISAKA_DEFAULT_KEEPALIVE              	60
#define MISAKA_DEFAULT_CONNECT_RETRY  	        30
#define MISAKA_DEFAULT_SCAN_TIMER		15	    
#define MISAKA_PATH_SIZE			128

/* link status */
enum MISAKA_TAT{
	   TAT_IDLE,  			        //idle                                                                   
	   TAT_ESTA,                            //established              
	   TAT_MAX,   
};

/*connect mode*/
enum MISAKA_MODE{
           MODE_NONE = 0,
	   MODE_CONNECT,                        //connect                                                             
	   MODE_LISTEN,                         //listen                     
	   MODE_PASSIVE,   	                //created by listen
           MODE_MAX,
};

                                                /* MISAKA finite state machine status.  */
enum MISAKA_TYPE{
           TYPE_NONE = 0,
	   TYPE_TCP ,                    
	   TYPE_UDP,
	   TYPE_LUDP,                           /*area udp support*/
	   TYPE_SDIO,
	   TYPE_UART,                               
	   TYPE_MAX,   
};

//read status 
enum IO_TAT{
    IO_ERROR = 0,                           //stream error
    IO_CLOSE,                               //stream close
    IO_PASSIVE_CLOSE,                       //stream passive close
    IO_CHECK,                               //stream read and need to unpack
    IO_PACKET,                              //get one packet from stream
    IO_ACCEPT,                              //accept one client
    IO_PARTIAL,                             //read partial
    IO_FULL,                                //io full
    IO_CLEAN,                               //need clean read buffer
};

/* addr in network byteorder */
#define MISAKANIPQUAD(addr) \
	((unsigned char *)&(addr))[0], \
	((unsigned char *)&(addr))[1], \
	((unsigned char *)&(addr))[2], \
	((unsigned char *)&(addr))[3]

#define RECONNECT_INTERVAL			 (0.5)
#define DISTRIBUTE_INTERVAL			 (0.001)
#define DISPATCH_INTERVAL			 (0.001)
#define WATCH_INTERVAL                           (0.001)
#define OLD_INTERVAL                             (1)
#define PEER_OLD_TIME                            (15)

/* bgs peer information*/
struct peer{
        struct peer *peer;                   /*point to real peer when used as virtual peer*/

	int fd;			             /* File descriptor */
	int id;
	
	union sockunion su;	             /* Sockunion address of the peer. */
	union sockunion dsu;	             /* Sockunion address to connect. */

	struct sockaddr_un lsu;              /*area udp*/
	struct sockaddr_un ldsu;             /*area local udp*/  

	unsigned short port;                 /* Destination port for peer */
	char path[MISAKA_PATH_SIZE];	     /*path buffer used for device */

        int old;                             /*mark when old function needed*/
  	int type;                   	     /*peer type*/
	int status;			     /*peer status*/
	int len;                             /*payload len*/
	int mode;                            /*work mode of this peer*/
	int src;                             /*which this peer link to*/
        int role;                            /*role of this*/
        int drole;                           /*role of this*/
        int quick;                           /*start quick*/
        int reconnect;                       /*reconnect flag*/
        int sys;                             /*system packet route into sys handle*/

        int listens;                         /*tot listen*/

        struct ev_io *t_read;                /*read event*/
        struct ev_io *t_write;               /*write event*/
        struct ev_periodic *t_connect;       /*connect event*/
        struct ev_periodic *t_keepalive;     /*keep alive event*/ 

   	u32 v_connect;			    //reconnect interval
   	u32 v_keepalive;                    //keepalve interval

    	struct stream *ibuf;                //used for read buffer
    	struct stream_fifo *obuf;           //used for real send fifo
    	struct stream *work;

    	int maxcount;                       //max stream in buffer

        int ( *start)  (struct peer *peer);                         //peer start
        int ( *stop )  (struct peer *peer);                         //peer stop
        int ( *write)  (struct peer *peer);                         //write handle
        int ( *read)   (struct peer *peer);                         //read handle
        int ( *unpack) (struct stream *s, struct peer *peer);       //unpack handle
        int ( *pack)   (struct stream *s, struct peer *peer);       //pack handle
        
        int on_connect;
        int on_disconnect;

	/* Whole packet size to be read. */
	unsigned long packet_size;

	time_t uptime;		            /* Last Up/Down time */
    	time_t readtime;		    /* Last read time */
    	time_t resettime;		    /* Last reset time */

    	int rcount;                         /* recive packet*/
    	int scount;                         /* send packet*/

        //global handle
        struct ev_loop *loop;
};

/*global config*/
struct global_config{
	int role;
};

/*global manager*/
struct global_servant{

    /*peer list*/
    struct list *peer_list;                 //peer list
    
    /*peer key val */
    struct hash *peer_hash;

    //used to process stream            
    struct list *event_list;                //event list
    
    //global handle
    struct ev_loop *loop;

    //task out
    struct task_list *task_out;                 /*dispatch message wait to send*/

    struct thpool_ *thpool;                     /*thread pool*/
    
    struct ev_periodic *t_distpatch;            /*distpatch msg thread*/ 
    
    struct ev_periodic *t_watch;                /*watch the bgs status*/
    
    struct ev_periodic *t_old;                  /*old link*/

    //lua_State *L;

    struct idmaker *mid;                        /*id maker used to allocate id*/

    struct shmhandle *shm;           
    struct kmem *kmem;                          /*manager all mem*/
    struct kmem_cache *peer_cache;              /*manager all peer cache*/ 
    struct kmem_cache *stream_cache;            /*manager all stream cache*/
    struct kmem_cache *data_cache;              /*manager all data in cache*/
    struct kmem_cache *fifo_cache;              /*manager all fifo in cache*/
};

//plugin type
enum PLUGIN_TYPE{
    NONE_PLUGIN_TYPE = 0,                       //none plugin type
    C_PLUGIN_TYPE,                              //plugin for c
    LUA_PLUGIN_TYPE,                            //plugin for lua
}; 

//event handle 
struct event_handle{
    char path[MISAKA_PATH_SIZE];                //lib path for event handle
    void (*func)(struct stream *);   //call back function register
    int (*init)(void);                          //init callback handle
    int (*deinit)(void);                        //deinit callback handle
    int (*connect)(struct peer *);              //deinit callback handle
    int (*disconnect)(struct peer *);           //deinit callback handle
    
    lua_State *lhandle;                         //lua handle;

    void *chandle;                              //handle for c

    int type;                                   //event type
    int plug;                                   //plug type
}event_handle_t;

//date time structure
struct date_time{
	u16 year;
	u8 month;
	u8 date;
	u8 hour;
	u8 minute;
	u8 second;
	u16 missecond;
};

int core_init(void);
int core_run(void);
void sighandle(int num);
void misaka_packet_init(void);
int misaka_start ( struct peer *peer );
int misaka_stop ( struct peer *peer );
int misaka_ignore ( struct peer *peer );
int misaka_connect_success ( struct peer *peer );
int misaka_connect_fail ( struct peer *peer );
int misaka_reconnect ( struct peer *peer );
int misaka_stop_with_error ( struct peer *peer );
void misaka_timer_set ( struct peer *peer );
struct peer* peer_new(void);
char *peer_uptime (time_t uptime2, char *buf, size_t len, int type);
int misaka_packet_route(struct stream *s);
void misaka_packet_delete(struct stream_fifo* obuf);
extern int misaka_register_evnet(int type);
extern struct stream  *stream_dclone(struct stream *s);
extern struct stream  *stream_clone(struct stream *s);
extern void stream_free (struct stream *s);
extern struct stream * stream_clone_one(struct stream *from);
extern void dump_sockunion(union sockunion *su);
extern void peer_register(struct peer *peer);
extern void peer_unregister(struct peer *peer);
extern void *peer_lookup(struct peer *peer);
extern void stream_dir_exchange(struct stream *s);
extern struct stream * misaka_packet_net_route(struct stream *s);
extern struct stream * misaka_packet_task_route(struct stream *s);
extern struct stream* misaka_write_packet(struct stream_fifo *obuf);
extern void misaka_packet_loop_route(void);
extern int register_c_event(const char *path, int type);


enum event_status{
    EVENT_STOP = 0,
    EVENT_LOAD,
    EVENT_RUN,
};


//@TODO support of code hotplug
extern void event_load(int type);
extern void event_start(int type);
extern int  event_tat(int type);
extern void event_stop(int type);
extern void event_plug(int type);

//config manger
extern struct global_config misaka_config;    //local config handle

//handle manger
extern struct global_servant misaka_servant;  //local servant handle    


#endif /* __MISAKA_H_ */

