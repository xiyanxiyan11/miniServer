#include "bgs.h"
#include "common.h"
#include "network.h"
#include "string.h"

//create udp connect socket 
struct peer* peer_udp_create(const char *dstip, unsigned short dstport, unsigned short srcport)
{
        int ret;
	struct peer* peer;
	int socklen;
	struct sockaddr_in sin;
	char buf[256];

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

        //init the dst unsion
        memset(&sin, 0, sizeof(struct sockaddr_in));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = inet_addr(dstip); 
	inet_ntop(AF_INET, &sin.sin_addr, buf, 256);
	//fprintf(stderr, "convert from dstip %s address to union address success\n", buf);
        sin.sin_port = htons(dstport); 
        socklen = sizeof(struct sockaddr_in);
#ifdef HAVE_SIN_LEN
        sin.sin_len = socklen;
#endif
        peer->dsu.sin = sin;

	peer->type = TYPE_UDP;
	peer->mode = MODE_CONNECT;
	return peer;
}

//open the udp
int udp_connect(struct peer* peer)
{
        int ret, en;
	//open socket udp socket
	//open socket udp socket
	peer->fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (peer->fd < 0)
	{
		fprintf(stderr, "socket: %s\n", strerror(errno));
		return connect_error;
	}

	//set address and port re use
	sockopt_reuseaddr(peer->fd);
	sockopt_reuseport(peer->fd);

	en = errno;
	
	//try to bind
	ret = bind(peer->fd, (struct sockaddr *) &peer->su.sin, sizeof(struct sockaddr_in));

	if (ret < 0)
	{
		//fprintf(stderr, "bind: %s\n", strerror(en));
		close(peer->fd);
		return connect_error;
	}else{
	
	        //fprintf(stderr, "bind:%s\n", strerror(en));
	}
        return connect_success;
}

//start read packet, 0 one packet, 1 read again, 2 full, -1 error
int udp_read(struct peer* peer)
{
        struct peer *vpeer, *tpeer;
	int nbytes;
	int type;
	int src;
	int dst;
  	int readsize;
  	union sockunion su;
        struct sockaddr_in sin;
        int len;
        int ret;
        struct stream *s;   //replay packet  
        int tcpid;
        struct stream *rs;
        struct stream ss;

        /* Read packet header to determine type of the packet */
        //if (peer->packet_size == 0)
        peer->packet_size = MISAKA_MAX_PACKET_SIZE;

        sin = peer->dsu.sin;
        len = sizeof(struct sockaddr);

        nbytes = stream_recvfrom (peer->ibuf, peer->fd, peer->packet_size, 0, (struct sockaddr*)&sin, &len); 
        

        peer->ibuf->su.sin = sin;

  	/* If read byte is smaller than zero then error occured. */
        if (nbytes < 0) 
  	{
        
        }
  	return IO_CHECK;                   //0 get one packet
}

/*udp write process*/
int udp_write(struct peer *peer){
        struct stream *s;
  	int num;
  	int count =0;
      	int writenum;
      	char ip[256];
  	char buf[MISAKA_HEADER_SIZE] = {0};          //empty buffer
        

        //get first stream;
  	s = bgs_write_packet (peer->obuf);
  	if (!s)
    		return 0;	
	/* udp Nonblocking write */
  	do
        {

      		/* Number of bytes to be sent.  */
      		writenum = stream_get_endp (s) - stream_get_getp (s);
		
      		/* Call write() system call.*/
      		num = sendto(peer->fd, STREAM_PNT (s), writenum, \
                0, (struct sockaddr*)&s->dsu.sin, sizeof(struct sockaddr));

                //printf("udp send num %d\n", num);

	        inet_ntop(AF_INET, &peer->dsu.sin.sin_addr, ip, 256);

      		if (num < 0)
		{
	 		       
		}

      		if (num != writenum)
		{
	  		     
		}
   
      		/*OK we send packet so delete packet. */
      		bgs_packet_delete (peer->obuf);

    	 }while (++count < MISAKA_WRITE_PACKET_MAX && (s = bgs_write_packet (peer->obuf)) != NULL);
         return count;  
}

/* udp server init */
struct peer * udp_init(const char *str, unsigned short dstport, unsigned short srcport)
{
	struct peer* peer = NULL;
	
	//create connect peer
	peer = peer_udp_create(str, dstport, srcport);
        
        //add peer into work list
        if(!peer){
            return NULL;
        }

        //register api
	peer->read  = udp_read;
	peer->write = udp_write;
	peer->start = udp_connect;
	
	return peer;
}
