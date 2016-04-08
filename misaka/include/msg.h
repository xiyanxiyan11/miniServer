#ifndef __MSG_H_
#define __MSG_H_

#include "misaka.h"

#define	MCP_VERSION	1

#define	MISAKA_HEADER_SIZE		    8                   /* reverse header size*/
#define	MISAKA_MAX_PACKET_SIZE	            1024		/* packet max size*/

#define TCP_DSTIP                           "127.0.0.1"
#define TCP_DSTPORT                          11111

#define ROLE_UDP    (1)
#define ROLE_TCP    (2)
#define ROLE_SERVER (3)

typedef struct mcpmsg{
	unsigned char type;                 //packet type
	int src;                            //packet src
	int dst;                            //packet dst
	int session;                        //session id for the same event
	int seq;                            //seq id for one session
	unsigned short len;                 //packet len
}mcpmsg_t;

//message type here
enum EVENT_TYPE{
    EVENT_NONE,
    EVENT_ECHO,
    EVENT_MAX,
};

#define MCP_HEADER_SIZE                 sizeof(mcpmsg_t)

#endif /* __MCP_H_ */
