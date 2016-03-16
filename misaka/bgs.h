#ifndef __BGS_H_
#define __BGS_H_

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <sys/un.h>
#include <pthread.h>
#include "zebra.h"
#include "ev.h"

#include "thread.h"
#include "memory.h"
#include "stream.h"
#include "memtypes.h"
#include "linklist.h"
#include "hash.h"
#include "thpool.h"
#include "log.h"

#include "kmem.h"
#include "gcp.h"

#define	BGS_UPTIME_LEN	                   25
#define BGS_MAX_QUEUE_PACKET               12
#define	BGS_WRITE_PACKET_MAX	           30

#define BGS_MEM_SIZE                        (1024)
#define BGS_MEM_ALIGN                       8
#define BGS_MAX_PEER                        (20)
#define BGS_MAX_STREAM                      (80)
#define BGS_MAX_DATA                        (BGS_MAX_STREAM)



#define BGS_THREAD_NUM                        1
//#define BGS_THREAD_SUPPORT                  1


#define BGS_INIT_START_TIMER  			5
#define BGS_DEFAULT_HOLDTIME               	180
#define BGS_DEFAULT_KEEPALIVE              	60
#define BGS_DEFAULT_CONNECT_RETRY  	        30
#define BGS_DEFAULT_SCAN_TIMER		        15	    
#define BGS_PATH_SIZE				128

/* link status */
enum BGP_TAT{
	   TAT_IDLE,  			        //idle                                                                   
	   TAT_ESTA,                            //established              
	   TAT_MAX,   
};

/*connect mode*/
enum BGP_MODE{
           MODE_NONE = 0,
	   MODE_CONNECT,                        //connect                                                             
	   MODE_LISTEN,                         //listen                     
	   MODE_PASSIVE,   	                //created by listen
           MODE_MAX,
};

                                                /* BGP finite state machine status.  */
enum BGP_TYPE{
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
#define BGSNIPQUAD(addr) \
	((unsigned char *)&(addr))[0], \
	((unsigned char *)&(addr))[1], \
	((unsigned char *)&(addr))[2], \
	((unsigned char *)&(addr))[3]

#define RECONNECT_INTERVAL			 (0.5)
#define DISTRIBUTE_INTERVAL			 (0.001)
#define DISPATCH_INTERVAL			 (0.001)
#define WATCH_INTERVAL                           (3)
#define PEER_OLD_TIME                            (3)

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
	char *host;			     /* Printable address of the peer. */
	char path[BGS_PATH_SIZE];	     //path buffer used for device

  	int type;                   	     /*peer type*/
	int status;			     /*peer status*/
	int len;                             /*payload len*/
	int mode;                            /*work mode of this peer*/
	int src;                             /*which this peer link to*/
        int role;                            /*role of this*/
        int drole;                           /*role of this*/
        int quick;                           /*start quick*/
        int reconnect;                       /*reconnect flag*/

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

    //task in
    struct task_list *task_in;

    //task out
    struct task_list *task_out;

    struct thpool_ *thpool;                  //thread pool
    
    struct ev_periodic *t_distribute;        /*distribute thread*/

    struct ev_periodic *t_distpatch;         /*distpatch thread*/ 
    
    struct ev_periodic *t_watch;             /*watch the bgs status*/

            
    struct kmem *kmem;                       /*manager all mem*/
    struct kmem_cache *peer_cache;           /*manager all peer cache*/ 
    struct kmem_cache *stream_cache;         /*manager all stream cache*/
    struct kmem_cache *data_cache;           /*manager all data in cache*/
};

struct event_handle{
    void (*func)(struct stream *);
    int type;
}event_handle_t;

struct date_time{
	u16 year;
	u8 month;
	u8 date;
	u8 hour;
	u8 minute;
	u8 second;
	u16 missecond;
};

#define SET_TAT(stat, bit)      (stat |= (1 << bit))
#define CLEA_TAT(stat, bit)     (stat &= ~(1 << bit))
#define CHECK_TAT(stat, bit)    (stat &  (1 << bit))

int core_init(void);
int core_run(void);
void sighandle(int num);
void bgs_packet_init(void);
int bgs_start ( struct peer *peer );
int bgs_stop ( struct peer *peer );
int bgs_ignore ( struct peer *peer );
int bgs_connect_success ( struct peer *peer );
int bgs_connect_fail ( struct peer *peer );
int bgs_reconnect ( struct peer *peer );
int bgs_stop_with_error ( struct peer *peer );
void bgs_timer_set ( struct peer *peer );
struct peer* peer_new(void);
char *peer_uptime (time_t uptime2, char *buf, size_t len, int type);
int bgs_packet_route(struct stream *s);
void bgs_packet_delete(struct stream_fifo* obuf);
extern int bgs_register_evnet( void (*func)(struct stream *), int type);
extern struct stream  *stream_dclone(struct stream *s);
extern struct stream  *stream_clone(struct stream *s);
extern void stream_free (struct stream *s);
extern struct stream * stream_clone_one(struct stream *from);
extern void dump_sockunion(union sockunion *su);
extern void peer_register(struct peer *peer);
extern void peer_unregister(struct peer *peer);
extern void *peer_lookup(struct peer *peer);
extern void stream_dir_exchange(struct stream *s);
extern int bgs_packet_thread_route(struct stream *s);
extern struct stream* bgs_write_packet(struct stream_fifo *obuf);

//config manger
extern struct global_config bgs_config;    //local config handle

//handle manger
extern struct global_servant bgs_servant;  //local servant handle    


#endif /* __BGS_H_ */

