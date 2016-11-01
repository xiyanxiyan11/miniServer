#include "misaka.h"
#include "common.h"
#include "network.h"
#include "string.h"
#include "link.h"

static struct peer * tcp_passive_init(union sockunion *su, int fd);

//connect progress
int tcp_connect(struct peer* peer){
	unsigned int ifindex = 0;
	//open socket udp socket
	if(peer->fd < 0)
	    peer->fd = socket(AF_INET, SOCK_STREAM, 0);

	if (peer->fd < 0)
	{
		mlog_debug("socket: %s", strerror(errno));
		return connect_error;
	}
	return sockunion_connect(peer->fd, &peer->dsu, (peer->dsu.sin.sin_port), ifindex);
}

//accept progress
int tcp_listen(struct peer *peer){
	int ret = 0;
	if(peer->fd < 0)
	    peer->fd = socket(AF_INET, SOCK_STREAM, 0);
	
	mlog_debug("tcp listen trigger");
	
	if (peer->fd < 0)
	{
		mlog_debug("socket: %s", strerror(errno));
		return connect_error;
	}
        
        ret = sockunion_bind (peer->fd, &peer->su, ntohs(peer->su.sin.sin_port), NULL);
        if(ret < 0 ){
            return connect_error;
        }
        ret = listen(peer->fd, peer->listens);
        if(ret < 0){
            return connect_error;
        }

	//set address and port re use
	sockopt_reuseaddr(peer->fd);
	sockopt_reuseport(peer->fd);
        return connect_success;
}

//passive progress
int tcp_passive(struct peer *peer){
    return connect_success;
}

//start read packet, 0 one packet, 1 read again, 2 full, -1 error
int tcp_read(struct peer* peer)
{
	int nbytes;
        nbytes = stream_read_try(peer->ibuf, peer->fd, peer->packet_size);
  	//mlog_debug("%d bytes read from peer %d, drole %d", nbytes, peer->fd, peer->drole);

        //EINTER
  	if (-2 == nbytes){
            return IO_CHECK; //need read again
        }   
  	
  	//ERROR or CLOSE
  	if(-1 == nbytes){
            if(peer->mode == MODE_PASSIVE)
                return IO_PASSIVE_CLOSE;
            else
                return IO_CLOSE;
        }

        peer->packet_size -= nbytes;

        if(peer->packet_size){
  	    return IO_PARTIAL;
        }else{
  	    return IO_CHECK;
        }
}

//start read 
int tcp_accept(struct peer *peer){
        union sockunion su;
        struct peer *cpeer;
        int ret;
        int fd =  sockunion_accept (peer->fd, &su);
        if(fd < 0)
            return IO_ERROR;
    
        mlog_debug("accept tcp fd %d", fd);
        cpeer = tcp_passive_init(&su, fd);
        if(NULL == cpeer){
            mlog_debug("alloc  peer fail from cache");
            close(fd);
            return IO_ACCEPT;
        }
        //trance pack and unpack to the passive peer
        cpeer->pack = peer->pack;
        cpeer->unpack = peer->unpack;
        cpeer->on_connect = peer->on_connect;
        cpeer->on_disconnect = peer->on_disconnect;
        cpeer->parser = peer->parser;

        //call unpack to distribute id for this peer
        if(ret == IO_PASSIVE_CLOSE){
            return ret;
        }
        cpeer->quick = 1;               //quick connect
        misaka_start(cpeer);
        return IO_ACCEPT;
}

//tcp write support
int tcp_write(struct peer *peer){
  	struct stream *s; 
  	int num;
  	int count = 0;
  	int pcount = 0;
      	int writenum;
     
        //mlog_debug("tcp write trigger with packet %d", peer->obuf->count);
        //get first stream;
  	s = misaka_write_packet (peer->obuf);
  	if (!s)
    		return 0;		
  	do
        {
      		/* Number of bytes to be sent.  */
                writenum = stream_get_endp (s) - stream_get_getp (s);
      		num = write (peer->fd, STREAM_PNT (s), writenum);
      		//mlog_debug("peer fd %d write %d->%d", peer->fd, writenum, num);
                    
      		if (num < 0)
		{
		    return 0;
		}

      		if (num != writenum)
		{
		
		}else{
      		    misaka_packet_delete (peer->obuf);
      		    pcount++;
      		}
    	 }while (count < MISAKA_WRITE_PACKET_MAX && (s = misaka_write_packet (peer->obuf)) != NULL);
         return pcount;  
}

//create tcp connect socket 
struct peer* peer_tcp_create(const char *dstip, unsigned short dstport, unsigned short srcport)
{
        int ret;
	struct peer* peer;
	int socklen;
	struct sockaddr_in sin;

	peer = peer_new();
	if (NULL == peer)
		return NULL;
        
        //init the src union
        memset(&sin, 0, sizeof(struct sockaddr_in));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port = htons(srcport); 
        socklen = sizeof(struct sockaddr_in);
#ifdef HAVE_SIN_LEN
        sin.sin_len = socklen;
#endif
        peer->su.sin = sin;

        memset(&sin, 0, sizeof(struct sockaddr_in));
        sin.sin_family = AF_INET;
        if(dstip){
            sin.sin_addr.s_addr = inet_addr(dstip); 
            sin.sin_port = htons(dstport); 
        }
        socklen = sizeof(struct sockaddr_in);
#ifdef HAVE_SIN_LEN
        sin.sin_len = socklen;
#endif
        peer->dsu.sin = sin;
	peer->type = TYPE_TCP;
	return peer;
}

/* tcp server init */
struct peer * tcp_connect_init(const char *str, unsigned short dstport, unsigned short srcport)
{
	struct peer* peer = NULL;
	
	//create connect peer
	peer = peer_tcp_create(str, dstport, srcport);
        
        //add peer into work list
        if(!peer){
            return NULL;
        }
        //register api
	peer->read  = tcp_read;
	peer->write = tcp_write;
	peer->start = tcp_connect;
	peer->mode = MODE_CONNECT;
	return peer;
} 

/* tcp server init */
struct peer * tcp_listen_init(int srcport)
{
	struct peer* peer = NULL;
	
	//create connect peer
	peer = peer_tcp_create(NULL, 0, srcport);
        
        //add peer into work list
        if(!peer){
            return NULL;
        }

        //register api
	peer->read  = tcp_accept;
	peer->write = tcp_write;
	peer->start = tcp_listen;
	peer->mode = MODE_LISTEN;
	return peer;
}

/* tcp passive init */
struct peer * tcp_passive_init(union sockunion *su, int fd)
{
	struct peer* peer = NULL;
	
	//create connect peer
	peer = peer_new();
	if (NULL == peer)
		return NULL;

	peer->fd = fd;
	peer->dsu = *su;

        //register api
	peer->read  = tcp_read;
	peer->write = tcp_write;
	peer->start = tcp_passive;
	peer->mode = MODE_PASSIVE;
	peer->old = 1;
	
	return peer;
}
