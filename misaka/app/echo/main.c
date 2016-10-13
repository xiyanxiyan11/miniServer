#include "misaka.h"
#include "link.h"
#include "task.h"
#include "log.h"

//register link and event here
int core_register()
{
    struct peer *peer;

    //register debug api
    int ret = INIT_DEBUG("/tmp/misaka.conf");
    if(ret < 0){
        printf("error in debug init with %d\n", ret);
    }
    SET_DEBUG(DEBUG_DEBUG);

    //register event
    //register_c_event("/tmp/task/.libs/libecho.so", EVENT_ECHO);
    register_lua_event("/tmp/task-lua/echo.lua", EVENT_ECHO);
    //register_python_event("echo", EVENT_ECHO);

    misaka_config.role = ROLE_SERVER;
//support support test
#if 0
    peer  =  udp_init("192.168.97.98", 8000, 11112);
    if(!peer){
        fprintf(stderr, "init peer fail\n");
	    return -1;
    }
    peer->pack =    echo_pack;
    peer->unpack =  echo_unpack;
    peer->role = ROLE_SERVER;
    peer->drole = ROLE_UDP;
    peer_register(peer);
    misaka_start(peer);
#endif


//tcp support test
#if 1
    peer = tcp_listen_init(TCP_DSTPORT);
    peer->listens = 3;
    peer->packet_size = 512;
    peer->on_connect = 1;
    peer->on_disconnect = 1;
    peer->role = ROLE_SERVER;
    peer->drole = ROLE_TCP;
    peer_register(peer);
    misaka_start(peer);
#endif
    return 0;
}


int main(int argc, char **argv)
{
	int background =0;
	int  opt,i;
	while ((opt = getopt(argc, argv, "dhf:l:S:B:C:N:")) != -1) {
               switch (opt) {
			case 'd':
				background = 1;
				break;
			default:
				return 1;
		}
	}

	if (background)
	    daemon(0, 1);
        
        core_init();

        core_register();

        core_run();
        
        return 0;
}

