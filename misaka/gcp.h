#ifndef __GCP_H_
#define __GCP_H_

#include "bgs.h"
#include "types.h"
/* GCP version */
#define	GCP_VERSION	1

#define	MISAKA_HEADER_SIZE		            8                   /* reverse header size*/
#define	MISAKA_MAX_PACKET_SIZE	            1500		/* packet max size*/

#define TCP_DSTIP                           "127.0.0.1"
#define TCP_DSTPORT                         11111

typedef struct gcp_message{
	unsigned char type;                 //packet type
	int src;                            //packet src
	int dst;                            //packet dst
	int sessionid;                      //session for the same event
	unsigned short len;                 //packet len
}gcp_message_t;

//message type here
enum EVENT_TYPE{
    EVENT_NONE,
    EVENT_ECHO,
    EVENT_MAX,
};


#define     ROLE_UDP    (1)
#define     ROLE_TCP    (2)
#define     ROLE_SERVER (3)


#define GCP_HEADER_SIZE                 sizeof(gcp_message_t)


#endif /* __GCP_H_ */
