#ifndef __MSG_H_
#define __MSG_H_

#include "misaka.h"

#define	MCP_VERSION	1

#define	MISAKA_HEADER_SIZE		    8                   /* reverse header size*/
#define	MISAKA_MAX_PACKET_SIZE	            256		        /* packet max size*/

#define TCP_DSTIP                           "127.0.0.1"
#define TCP_DSTPORT                          8002

#define ROLE_UDP    (1)
#define ROLE_TCP    (2)
#define ROLE_SERVER (3)


//message type here
enum EVENT_TYPE{
    EVENT_NONE, 
    //usr type
    EVENT_ECHO,
    //system type
    EVENT_SYS,  
    EVENT_NET,                              
    EVENT_MAX,
};

#define MCP_SIZE                 sizeof(mcpmsg_t)

#endif /* __MCP_H_ */
