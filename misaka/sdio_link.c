#include "bgs.h"
#include "common.h"

int sdio_connect(struct peer *peer){
	peer->fd = open(peer->path, O_RDWR);
        if(peer->fd < 0)
		return connect_error;
	return connect_success;
}

//create sdio connect socket
struct peer* peer_sdio_create(const char *path)
{
	struct peer* peer;
	peer = peer_new();
	if (NULL == peer)
		return NULL;

	snprintf(peer->path, (size_t)MISAKA_PATH_SIZE, "%s", path);
	peer->type = TYPE_SDIO;
	peer->mode = MODE_CONNECT;
	peer->status = TAT_IDLE;
	return peer;
}

//start read packet, 0 one packet, 1 read again, 2 full, -1 error
int sdio_read(struct peer* peer)
{
	int nbytes;
	int type;

        /* Read packet header to determine type of the packet */
        if (peer->packet_size == 0)
            peer->packet_size = MISAKA_MAX_PACKET_SIZE;

  	nbytes = stream_read(peer->ibuf, peer->fd, peer->packet_size);


  	/* If read byte is smaller than zero then error occured. */
  	if (nbytes < 0) 
  	{
        
        }


        if(nbytes == 0){
        
        
        }
    
  	return IO_CHECK;
}

/*sdio write process*/
int sdio_write(struct peer *peer){
  	struct stream *s; 
  	int num;
  	unsigned int count = 0;
      	int writenum;
        
        //get first stream;
  	s = misaka_write_packet (peer->obuf);
  	if (!s)
    		return 0;	
    		
	/* udp Nonblocking write */
  	do
        {
			
      		/* Number of bytes to be sent.  */
     
                writenum = stream_get_endp (s) - stream_get_getp (s);

                /*write data into file*/
      		num = write (peer->fd, STREAM_PNT (s), writenum);


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

struct peer * sdio_init(const char *str){
	int ret, en;
	struct peer* peer = NULL;
	
	//create connect peer
	peer = peer_sdio_create(str);
        
        //add peer into work list
        if(!peer){
            return NULL;
        }


        //register api
	peer->read  = sdio_read;
	peer->write = sdio_write;
	peer->start = sdio_connect;

	//misaka_start(peer);
        return peer;
}
