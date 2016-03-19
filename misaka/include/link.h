#ifndef LINK_H
#define LINK_H

struct peer * udp_init(const char *str, unsigned short dstport, unsigned short srcport);
struct peer * ludp_init(const char *srcip, const char *dstip);
struct peer * sdio_init(const char *str);
struct peer * tcp_listen_init(int srcport);
struct peer * tcp_connect_init(const char *str, unsigned short dstport, unsigned short srcport);
#endif
