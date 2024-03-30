
#include "board.h"
#include "net_helper.h"
#include "sdccp_net.h"
#include "cc_bjdc.h"
#include "hjt212.h"
#include "dm101.h"

rt_uint8_t          *s_pCCBuffer[BOARD_TCPIP_MAX];
rt_base_t           s_CCBufferPos[BOARD_TCPIP_MAX];
rt_mq_t             s_CCDataQueue[BOARD_TCPIP_MAX];
rt_bool_t           s_cc_reinit_flag[BOARD_TCPIP_MAX];

rt_bool_t cc_net_open(rt_uint8_t index)
{
    tcpip_cfg_t *pCfg = &g_tcpip_cfgs[index];

    cc_net_close(index);

    switch(pCfg->cfg.normal.proto_type) {
    case PROTO_CC_BJDC: 
    {
        cc_bjdc_open(index);
        net_open(index, 10240, CC_BJDC_PARSE_STACK, "netbjc");
        break;
    }
    case PROTO_HJT212:
    {
        hjt212_open(index);
        net_open(index, 10240, HJT212_PARSE_STACK, "nethjt");
        break;
    }
    case PROTO_DM101:
    {
        dm101_open(index);
        net_open(index, 10240, DM101_PARSE_STACK, "netdm");
        break;
    }
    }


    return RT_TRUE;
}

void cc_net_close(rt_uint8_t index)
{
    tcpip_cfg_t *pCfg = &g_tcpip_cfgs[index];

    net_close(index);

    switch(pCfg->cfg.normal.proto_type) {
    case PROTO_CC_BJDC:
        cc_bjdc_close(index);
        break;
    case PROTO_HJT212:
        hjt212_close(index);
        break;
    case PROTO_DM101:
        dm101_close(index);
        break;
    }
    //rt_mutex_release( &s_cc_mutex[index] );
}

void cc_net_disconnect(rt_uint8_t index)
{
    net_disconnect(index);
}

rt_bool_t cc_net_send(rt_uint8_t index, const rt_uint8_t *pData, rt_uint16_t usLen)
{
    return (net_send(index, (const rt_uint8_t *)pData, usLen) > 0);
}

