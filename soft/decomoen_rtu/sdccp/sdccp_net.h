
#ifndef __SDCCP_NET_H__
#define __SDCCP_NET_H__

rt_bool_t cc_net_open(rt_uint8_t index);
void cc_net_close(rt_uint8_t index);
void cc_net_disconnect(rt_uint8_t index);
rt_bool_t cc_net_send(rt_uint8_t index, const rt_uint8_t *pData, rt_uint16_t usLen);

extern rt_uint8_t   *s_pCCBuffer[];
extern rt_base_t    s_CCBufferPos[];
extern rt_mq_t      s_CCDataQueue[];
extern rt_bool_t    s_cc_reinit_flag[];

#endif

