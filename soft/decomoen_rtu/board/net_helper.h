#ifndef __NET_HELPER_H__
#define __NET_HELPER_H__

#include "board.h"

#define NET_HELPER_MAX      (BOARD_TCPIP_MAX)

#define NET_IS_ENET(_n)     ((_n)<BOARD_ENET_TCPIP_NUM)

#if NET_HAS_GPRS
#define NET_IS_GPRS(_n)     ((_n)>=BOARD_ENET_TCPIP_NUM && (_n)<BOARD_TCPIP_MAX)
#else
#define NET_IS_GPRS(_n)     0
#endif

#define NET_IS_NORMAL(_n)   (TCP_IP_M_NORMAL == g_tcpip_cfgs[_n].mode)
#define NET_IS_XFER(_n)     (TCP_IP_M_XFER == g_tcpip_cfgs[_n].mode)

#define NET_GET_XFER(_n)    (_n)

typedef struct {
    rt_uint8_t      *buffer;
    int             bufsz;
    int             pos;
    int             sock_fd;
    int             tcp_fd;
    rt_bool_t       shutdown;
    rt_bool_t       disconnect;
    rt_mutex_t      mutex;
    rt_thread_t     thread;
    rt_uint32_t     idetime;
    struct sockaddr_in to_addr;
} net_helper_t;

void net_init_all(void);
rt_bool_t net_open(int n, int bufsz, int stack_size, const char *prefix);
void net_disconnect(int n);
rt_bool_t net_isconnect(int n);
rt_bool_t net_waitconnect(int n);
void net_close(int n);
int net_send(int n, const rt_uint8_t *buffer, rt_uint16_t bufsz);
net_helper_t *net_get_helper(int n);
void ws_net_try_send(rt_uint16_t port, void *buffer, rt_size_t size);
void ws_gprs_try_send(rt_uint16_t port, void *buffer, rt_size_t size);

#endif

