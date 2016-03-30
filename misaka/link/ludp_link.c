#include "misaka.h"
#include "common.h"

//create ludp socket 
struct peer* peer_ludp_create(const char *spath, const char *dpath)
{
        int ret;
	struct peer* peer;
	int socklen;

	peer = peer_new();
	if (NULL == peer)
		return NULL;
 
        //init the src union
        memset(&peer->lsu, 0, sizeof(struct sockaddr_un));
        snprintf(peer->lsu.sun_path, sizeof(peer->lsu.sun_path), "%s", spath);
        peer->lsu.sun_family = AF_UNIX;
        
        //init the src union
        memset(&peer->ldsu, 0, sizeof(struct sockaddr_un));
        snprintf(peer->ldsu.sun_path, sizeof(peer->ldsu.sun_path), "%s", dpath);
        peer->ldsu.sun_family = AF_UNIX;

	peer->type = TYPE_LUDP;
	peer->mode = MODE_CONNECT;
	return peer;
}


//open the ludp
int ludp_connect(struct peer* peer)
{
        int ret, en;
	//open socket udp socket
	peer->fd = socket(PF_UNIX, SOCK_DGRAM, 0);

	if (peer->fd < 0)
	{
		fprintf(stderr, "socket: %s\n", strerror(errno));
		return connect_error;
	}

	//try to bind
	ret = bind(peer->fd, (struct sockaddr *) &peer->lsu, sizeof(struct sockaddr_un));
        en = errno;

	if (ret < 0)
	{
		fprintf(stderr, "bind fail to %s: %s\n", peer->lsu.sun_path, strerror(en));
		close(peer->fd);
		unlink(peer->lsu.sun_path);
		return connect_error;
	}else{
	
	        fprintf(stderr, "bind success to %s :%s\n", peer->lsu.sun_path, strerror(en));
	}


        return connect_success;
}


//start read packet, 0 one packet, 1 read again, 2 full, -1 error
int ludp_read(struct peer* peer)
{
	int nbytes;
	int type;
	int src;
	int dst;
  	int readsize;
  	char buf[MISAKA_HEADER_SIZE] = {0};          //empty buffer
        struct sockaddr_un lsu;
        int len;
        unsigned char *ptr;         //used to set route place
        gcp_message_t *hp;

        if (peer->packet_size == 0)
            peer->packet_size = MISAKA_MAX_PACKET_SIZE;

        lsu = peer->lsu;
        len = sizeof(struct sockaddr_un);

  	/* readpacket from fd. */
        nbytes = stream_recvfrom (peer->ibuf, peer->fd, MISAKA_MAX_PACKET_SIZE, 0, (struct sockaddr*)&lsu, &len);  

  	/* If read byte is smaller than zero then error occured. */
  	if (nbytes < 0) 
  	{
        
        }
        
        /* Clear input buffer. */
        peer->packet_size = 0;
  	return IO_CHECK;                   //0 get one packet
}


/*ludp write process*/
int ludp_write(struct peer *peer){
  	struct stream *s; 
  	int num;
  	unsigned int count = 0;
      	int writenum;
      	char ip[256];
  	char buf[MISAKA_HEADER_SIZE] = {0};          //empty buffer
        
        //get first stream;
  	s = misaka_write_packet (peer->obuf);
  	if (!s)
    		return 0;	
    		
	/* udp Nonblocking write */
  	do
        {
                //cut reverse header
                stream_get(buf, s, MISAKA_HEADER_SIZE);
			
      		/* Number of bytes to be sent.  */
      		writenum = stream_get_endp (s) - stream_get_getp (s);
		
      		/* Call write() system call.*/
      		num = sendto(peer->fd, STREAM_PNT (s), writenum, \
                0, (struct sockaddr*)&peer->ldsu, sizeof(struct sockaddr_un));

      		if (num < 0)
		{
	 		       
		}

      		if (num != writenum)
		{
	  		     
		}
   
      		/*OK we send packet so delete packet. */
      		misaka_packet_delete (peer->obuf);
    	 }while (++count < MISAKA_WRITE_PACKET_MAX && (s = misaka_write_packet (peer->obuf)) != NULL);
         return 0;  
}

/* ludp server init */
struct peer * ludp_init(const char *srcip, const char *dstip)
{
	struct peer* peer = NULL;
	//create connect peer
	peer = peer_ludp_create(srcip, dstip);

        //add peer into work list
        if(!peer){
            return NULL;
        }


        //register api
	peer->read  = ludp_read;
	peer->write = ludp_write;
	peer->start = ludp_connect;

	//misaka_start(peer);
	return peer;
}

