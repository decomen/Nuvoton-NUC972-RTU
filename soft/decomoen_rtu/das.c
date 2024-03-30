/*
 * File      : das.c
 */

#include <board.h>
#include "mdtypedef.h"
#include "mxml.h"
#include "net_helper.h"

mdBOOL g_bIsTestMode = mdFALSE;

struct das_system_info g_sys_info;
struct das_system_ver g_sys_ver;

rt_thread_t g_AIReadThread;
rt_thread_t g_DIReadThread;
rt_thread_t g_DOWriteThread;

extern mdBOOL g_bIsTestMode;

static void __doSystemReset(void *arg)
{
    sleep(5);
    NVIC_SystemReset();
}

void vDoSystemReset(void)
{
    rt_thread_t th = rt_thread_create("sys_reset", __doSystemReset, RT_NULL, 0x300, 20, 20);
    if (th) {
        rt_thread_startup(th);
    } else {
        NVIC_SystemReset();
    }
}

static void __doSystemReboot(void *arg)
{
    sleep(5);
    NVIC_SystemReboot();
}

void vDoSystemReboot(void)
{
    rt_thread_t th = rt_thread_create("sys_reboot", __doSystemReboot, RT_NULL, 0x300, 20, 20);
    if (th) {
        rt_thread_startup(th);
    } else {
        NVIC_SystemReboot();
    }
}

static void __thread_idle(void *arg)
{
    rt_tick_t regtick = 0;
    extern rt_tick_t g_ulLastBusyTick;
    int key_tick = 0;

    while (1) {
        rt_tick_t tickNow = rt_tick_get();
        cpu_flash_usage_refresh();

        /*if (tickNow - g_ulLastBusyTick >= rt_tick_from_millisecond(1000)) {
            g_ulLastBusyTick = tickNow;
            rt_pin_write(BOARD_GPIO_NET_LED, PIN_HIGH);
        }*/

        if(!g_bIsTestMode){
            if(KeyIoStatus() == 0){
		        printf("key down!\r\n");
                //key_down = 1;
                if((rt_tick_get() - key_tick) >=  rt_tick_from_millisecond(10000)){
                   printf("\n==========key down 10 s, factory reset!!!!!!\n\n");
                   for(int i = 0 ; i < 6; i++){
                        vTestLedToggle();
                        usleep(100000);
                   }
                   board_cfg_del_all();
                   vDoSystemReset();  
                   key_tick = rt_tick_get();
                }
            }else {
                key_tick = rt_tick_get();
            }
        }

        if (tickNow - regtick >= rt_tick_from_millisecond(30000)) {
            if (!g_isreg) {
                reg_testdo(rt_millisecond_from_tick(tickNow - regtick) / 1000);
                if (reg_testover()) {
                    rt_kprintf("test time over\n");
                    if (
#ifdef USR_TEST_CODE
                        !g_bIsTestMode &&
#endif
                        !g_istestover_poweron) {
                        rt_kprintf("reset system\n");
                        NVIC_SystemReset();
                    }
                }
            }
            regtick = tickNow;
        }
        sleep(2);
    }
}

void rt_thread_idle_init(void)
{
    rt_thread_t th = rt_thread_create("idle", __thread_idle, RT_NULL, 0x300, 20, 20);
    if (th) {
        rt_thread_startup(th);
    }
}

int compar(const void *a, const void *b)
{
    return *(var_float_t *)a > *(var_float_t *)b;
}

AIResult_t g_AIEngUnitResult;
AIResult_t g_AIMeasResult;
var_double_t g_dAIStorage_xx[ADC_CHANNEL_NUM] = { 0.0f };
int g_nAIStorageCnt[ADC_CHANNEL_NUM] = { 0 };
rt_tick_t g_dAI_last_storage_tick[ADC_CHANNEL_NUM] = { 0 };


var_double_t g_dAIHourStorage_xx[ADC_CHANNEL_NUM] = { 0.0f };
int g_nAIHourStorageCnt[ADC_CHANNEL_NUM] = { 0 };
rt_tick_t g_dAIHour_last_storage_tick[ADC_CHANNEL_NUM] = { 0 };

void vAIReadTask(void *parameter)
{
    const rt_uint8_t chan_tbl[] = { 6, 7, 0, 1, 2, 3, 4, 5 };
    //const rt_uint8_t chan_tbl[] = { 6, 7, 3, 2, 1, 0, 4, 5 };
    //const rt_uint8_t chan_tbl[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    s_AdcValue_t xAdcVal;
    //static var_float_t fAI_xx[ADC_CHANNEL_NUM][10] = { 0.0f };
    //static var_float_t fAIEng_xx[ADC_CHANNEL_NUM][10] = { 0.0f };
    static analog_cfg_t s_analog_cfgs[ADC_CHANNEL_NUM];

    while (1) {

        for(int j = 0; j < 100; j++){  //增加采集效率，每采集100次后再进行休眠
            das_delay(10);
            memcpy(&s_analog_cfgs[0], &g_analog_cfgs[0], sizeof(g_analog_cfgs));
            for (ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++) {
                int index = chan_tbl[chan];
                analog_cfg_t *pCfg = &s_analog_cfgs[index];
                if (1/*pCfg->enable*/) {
                    if (pCfg->unit!=Unit_Meter) pCfg->ext_corr.factor = 0;
                    if(ADC_MODE_NORMAL == gAdcCfgPram.eMode) {
                        vGetAdcValue(chan, Range_0_20MA, pCfg->range, pCfg->ext_range_min, pCfg->ext_range_max, pCfg->ext_corr, &xAdcVal);
                    } else if(ADC_MODE_TEST == gAdcCfgPram.eMode){
                        //vGetAdcValueTest(chan, Range_0_20MA, pCfg->range, pCfg->ext_range_min, pCfg->ext_range_max, pCfg->ext_corr, &xAdcVal);
                    } else if(ADC_MODE_CALC == gAdcCfgPram.eMode){
                        //vGetAdcValueCal(chan, Range_0_20MA,  pCfg->range, pCfg->ext_range_min, pCfg->ext_range_max, pCfg->ext_corr, &xAdcVal);
                    }
                }

                if(gAdcCfgPram.eMode == ADC_MODE_NORMAL){
                    if (!pCfg->enable) {
                        memset(&xAdcVal, 0, sizeof(s_AdcValue_t));
                    }
                }

                //rt_kprintf("chan = %d,adc = %d , I = %f\r\n", chan, xAdcVal.usEngUnit,xAdcVal.fMeasure);

                //	float vol = (float)xAdcVal.usEngUnit * 5.0 / 16777215.0f ;
                //	rt_kprintf("chan = %d, adc = %d, vol = %d.%d , meter = %d.%d\r\n", chan , xAdcVal.usEngUnit, (int)((int)(vol * 10000) / 10000),
                //	(int)((int)(vol*10000)%10000),(int)((int)(xAdcVal.fPercentUnit * 10000) / 10000), (int)((int)(xAdcVal.fPercentUnit*10000)%10000));

                // fAIEng_xx[chan][count] = (float)xAdcVal.usEngUnit;
                g_AIEngUnitResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
                g_AIMeasResult.fAI_xx[index] = xAdcVal.fMeasure;

                switch (pCfg->unit) {
                case Unit_Eng:
                    //fAI_xx[chan][count] = (float)xAdcVal.usEngUnit;
                    g_xAIResultReg.xAIResult.fAI_xx[index] = (float)xAdcVal.usEngUnit;
                    break;
                case Unit_Binary:
                    // fAI_xx[chan][count] = (float)xAdcVal.usBinaryUnit;
                    g_xAIResultReg.xAIResult.fAI_xx[index] = (float)xAdcVal.usBinaryUnit;
                    break;
                case Unit_Percent:
                    //fAI_xx[chan][count] = xAdcVal.fPercentUnit;
                    g_xAIResultReg.xAIResult.fAI_xx[index] = xAdcVal.fPercentUnit;
                    break;
                case Unit_Meter:
                    //fAI_xx[chan][count] = xAdcVal.fMeterUnit;
                    g_xAIResultReg.xAIResult.fAI_xx[index] = xAdcVal.fMeterUnit;
                    break;
                }

                g_dAIStorage_xx[index] += g_xAIResultReg.xAIResult.fAI_xx[index];
                g_nAIStorageCnt[index]++;
                g_dAIHourStorage_xx[index] += g_xAIResultReg.xAIResult.fAI_xx[index];
                g_nAIHourStorageCnt[index]++;

                if (g_storage_cfg.bEnable && pCfg->enable) {
                    // min
                    if (rt_tick_get() - g_dAI_last_storage_tick[index] >= rt_tick_from_millisecond(g_storage_cfg.ulStep * 60 * 1000)) {
                        char ident[10] = "";
                        if (g_nAIStorageCnt[index] > 0) {
                            var_double_t value = g_dAIStorage_xx[index] / g_nAIStorageCnt[index];
                            g_dAIStorage_xx[index] = 0;
                            g_nAIStorageCnt[index] = 0;
                            g_dAI_last_storage_tick[index] = rt_tick_get();
                            rt_sprintf(ident, "#AI_%d", index);
                            rt_thddog_suspend("bStorageAddData min");
                            bStorageAddData(ST_T_MINUTE_DATA, ident, value, "");
                            rt_thddog_resume();
                        }
                    }

                    // hour
                    if (rt_tick_get() - g_dAIHour_last_storage_tick[index] >= rt_tick_from_millisecond(60 * 60 * 1000)) {
                        char ident[10] = "";
                        if (g_nAIHourStorageCnt[index] > 0) {
                            var_double_t value = g_dAIHourStorage_xx[index] / g_nAIHourStorageCnt[index];
                            g_dAIHourStorage_xx[index] = 0;
                            g_nAIHourStorageCnt[index] = 0;
                            g_dAIHour_last_storage_tick[index] = rt_tick_get();
                            rt_sprintf(ident, "#AI_%d", index);
                            rt_thddog_suspend("bStorageAddData hour");
                            bStorageAddData(ST_T_HOURLY_DATA, ident, value, "");
                            rt_thddog_resume();
                        }
                    }
                }
            }
        }

        rt_thread_delay(gAdcCfgPram.ChannelSleepTime);
        rt_thddog_feed("");

        /*
            for( ADC_CHANNEL_E chan = ADC_CHANNEL_0; chan <= ADC_CHANNEL_7; chan++ ) {
                var_float_t sum = 0, sum_eng = 0;
                qsort( fAI_xx[chan], sizeof(var_float_t), 10, compar );
                qsort( fAI_xx[chan], sizeof(var_float_t), 10, compar );
                for( int i = 3; i <= 6; i++ ) {
                    sum += fAI_xx[chan][i];
                    sum_eng += fAIEng_xx[chan][i];
                }
                sum /= 4; sum_eng /= 4;
                g_AIEngUnitResult.fAI_xx[chan_tbl[chan]] = sum_eng;
                g_xAIResultReg.xAIResult.fAI_xx[chan_tbl[chan]] = sum;
            }*/
    }
    g_AIReadThread = RT_NULL;
    rt_thddog_exit();
}

void vAIReadStop(void)
{
    if (g_AIReadThread) {
        rt_thddog_unregister(g_AIReadThread);
        if (RT_EOK == rt_thread_delete(g_AIReadThread)) {
            g_AIReadThread = RT_NULL;
        }
    }
}

rt_err_t xAIReadReStart(void)
{
    vAIReadStop();

    if (RT_NULL == g_AIReadThread) {
        g_AIReadThread = rt_thread_create("AI", vAIReadTask, RT_NULL, 0x300, 20, 20);

        if (g_AIReadThread) {
            rt_thddog_register(g_AIReadThread, 30);
            rt_thread_startup(g_AIReadThread);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vDIReadTask(void *parameter)
{
    eInOut_stat_t xStat;
    DIResult_t xDIResultBak = g_xDIResultReg.xDIResult;

    while (1) {
        for (eTTL_Input_Chanel_t chan = TTL_INPUT_1; chan < TTL_INPUT_NUM; chan++) {
            vTTLInputputGet(chan, &xStat);
            g_xDIResultReg.xDIResult.usDI_xx[chan] = (xStat == PIN_RESET ? 0 : 1);
            if (g_di_cfgs[chan].enable && xDIResultBak.usDI_xx[chan] != g_xDIResultReg.xDIResult.usDI_xx[chan]) {
                if (g_storage_cfg.bEnable) {
                    char ident[10] = "";
                    rt_sprintf(ident, "#DI_%d", chan);
                    rt_thddog_suspend("bStorageAddData TLL");
                    bStorageAddData(ST_T_DIDO_DATA, ident, xDIResultBak.usDI_xx[chan] ? 1 : 0, RT_NULL);
                    rt_thddog_resume();
                }
                /*rt_thddog_suspend("lua_rtu_doexp TLL");
                lua_rtu_doexp(g_di_cfgs[chan].exp);
                rt_thddog_resume();*/
                if (g_di_cfgs[chan].exp[0]) {
                    evaluate(g_di_cfgs[chan].exp, NULL, NULL);
                }
            }
        }

        xDIResultBak = g_xDIResultReg.xDIResult;

        rt_thddog_feed("");
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
    }
    rt_thddog_exit();
}

void vDIReadStop(void)
{
    if (g_DIReadThread) {
        rt_thddog_unregister(g_DIReadThread);
        if (RT_EOK == rt_thread_delete(g_DIReadThread)) {
            g_DIReadThread = RT_NULL;
        }
    }
}

rt_err_t xDIReadReStart(void)
{
    vDIReadStop();

    if (RT_NULL == g_DIReadThread) {
        g_DIReadThread = rt_thread_create("DI", vDIReadTask, RT_NULL, 0x200, 20, 20);

        if (g_DIReadThread) {
            rt_thddog_register(g_DIReadThread, 30);
            rt_thread_startup(g_DIReadThread);
            return RT_EOK;
        }
    }
    return RT_ERROR;
}

void vDOWriteTask(void *parameter)
{
    DOResult_t xDOResultBak = g_xDOResultReg.xDOResult;

    while (1) {

        for (eRelays_OutPut_Chanel_t chan = RELAYS_OUTPUT_1; chan <= RELAYS_OUTPUT_4; chan++) {
            if (g_xDOResultReg.xDOResult.usDO_xx[chan] != 0) {
                vRelaysOutputConfig(chan, PIN_SET);
            } else {
                vRelaysOutputConfig(chan, PIN_RESET);
            }

            if (xDOResultBak.usDO_xx[chan] != g_xDOResultReg.xDOResult.usDO_xx[chan]) {
                if (g_storage_cfg.bEnable) {
                    char ident[10] = "";
                    rt_sprintf(ident, "#DO_%d", chan);
                    rt_thddog_suspend("bStorageAddData Relays");
                    bStorageAddData(ST_T_DIDO_DATA, ident, xDOResultBak.usDO_xx[chan] ? 1 : 0, RT_NULL);
                    rt_thddog_resume();
                }
                if (g_do_cfgs[chan].exp[0]) {
                    evaluate(g_do_cfgs[chan].exp, NULL, NULL);
                }
                /*rt_thddog_suspend("lua_rtu_doexp Relays");
                lua_rtu_doexp(g_do_cfgs[chan].exp);
                rt_thddog_resume();*/
            }
        }
        for (eTTL_Output_Chanel_t chan = TTL_OUTPUT_1; chan <= TTL_OUTPUT_2; chan++) {
            if (g_xDOResultReg.xDOResult.usDO_xx[4 + chan] != 0) {
                vTTLOutputConfig(chan, PIN_SET);
            } else {
                vTTLOutputConfig(chan, PIN_RESET);
            }

            if (xDOResultBak.usDO_xx[4 + chan] != g_xDOResultReg.xDOResult.usDO_xx[4 + chan]) {
                if (g_storage_cfg.bEnable) {
                    char ident[10] = "";
                    rt_sprintf(ident, "#DO_%d", 4 + chan);
                    rt_thddog_suspend("bStorageAddData TTL");
                    bStorageAddData(ST_T_DIDO_DATA, ident, xDOResultBak.usDO_xx[4 + chan] ? 1 : 0, RT_NULL);
                    rt_thddog_resume();
                }
                if (g_do_cfgs[4 + chan].exp[0]) {
                    evaluate(g_do_cfgs[4 + chan].exp, NULL, NULL);
                }
                /*rt_thddog_suspend("lua_rtu_doexp TTL");
                lua_rtu_doexp(g_di_cfgs[4 + chan].exp);
                rt_thddog_resume();*/
            }
        }

        xDOResultBak = g_xDOResultReg.xDOResult;

        rt_thddog_feed("");
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
    }
    rt_thddog_exit();
}

void vDOWriteStop(void)
{
    if (g_DOWriteThread) {
        rt_thddog_unregister(g_DOWriteThread);
        if (RT_EOK == rt_thread_delete(g_DOWriteThread)) {
            g_DOWriteThread = RT_NULL;
        }
    }
}

rt_err_t xDOWriteReStart(void)
{
    vDOWriteStop();

    if (RT_NULL == g_DOWriteThread) {
        g_DOWriteThread = rt_thread_create("DO", vDOWriteTask, RT_NULL, 0x200, 20, 20);

        if (g_DOWriteThread) {
            rt_thddog_register(g_DOWriteThread, 30);
            rt_thread_startup(g_DOWriteThread);
            return RT_EOK;
        }
    }
    return RT_ERROR;
}

void rt_init_zigbee_thread_entry(void *parameter);
void rt_gprs_thread_entry(void *parameter);

void rt_init_thread_entry(void *parameter)
{
    my_system("rm /tmp/test.log -rf");
#ifdef USR_TEST_CODE

   // rt_pin_mode(BOARD_GPIO_ZIGBEE_SLEEP, PIN_MODE_INPUT_PULLUP);

	if (vIsTestModeIo() == 1) {
		g_bIsTestMode = mdTRUE;
		g_istestover_poweron = mdFALSE;
    	xTestTaskStart();
        my_system("echo 'enter test mode' > /tmp/test.log");
	}
	
#endif

    das_set_time(das_get_time(), 28800);

    cpu_flash_usage_refresh();

    board_cfg_init();
    // 必须先 storage_cfg_init
    storage_cfg_init();
    vStorageInit();
    storage_log_init();

    vDevCfgInit();      // 放在最前面
    vAdcCalCfgInit();
    host_cfg_init();     // 放在第二
    reg_init();
    auth_cfg_init();
    net_cfg_init();
    tcpip_cfg_init();
    uart_cfg_init();
    analog_cfgs_init();
    gprs_cfg_init();
    zigbee_cfg_init();
    di_cfgs_init();
    do_cfgs_init();
    xfer_uart_cfg_init();
    vVarManage_ExtDataInit();
    rt_thddog_feed("");

    serial_helper_init();
    
    if (!g_istestover_poweron && !g_bIsTestMode ) {
        xAIReadReStart();
        xDIReadReStart();
        xDOWriteReStart();
    }

    if (!g_istestover_poweron && !g_bIsTestMode) {
        for (int port = 0; port < BOARD_UART_MAX; port++) {
            if ((UART_TYPE_232 == g_uart_cfgs[port].uart_type ||
                 UART_TYPE_485 == g_uart_cfgs[port].uart_type /*||
               UART_TYPE_ZIGBEE == g_uart_cfgs[port].uart_type*/) &&
                (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type ||
                 PROTO_MODBUS_ASCII == g_uart_cfgs[port].proto_type)
               ) {

                if (!g_xfer_net_dst_uart_occ[port]) {
                    if (PROTO_SLAVE == g_uart_cfgs[port].proto_ms) {
                        xMBRTU_ASCIISlavePollReStart(port, (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type) ? (MB_RTU) : (MB_ASCII));
                    } else if (PROTO_MASTER == g_uart_cfgs[port].proto_ms) {
                        xMBRTU_ASCIIMasterPollReStart(port, (PROTO_MODBUS_RTU == g_uart_cfgs[port].proto_type) ? (MB_RTU) : (MB_ASCII));
                    }
                }
            }
        }
        sdccp_serial_openall();
    }

    
    if (!g_istestover_poweron) {
        varmanage_start();

        {
            rt_thread_t init_zgb_thread = rt_thread_create("initzgb", rt_init_zigbee_thread_entry, RT_NULL, 512, 20, 20);

            if (init_zgb_thread != RT_NULL) {
                rt_thddog_register(init_zgb_thread, 30);
                rt_thread_startup(init_zgb_thread);
            }
        }
        {
            rt_thread_t init_gprs_thread = rt_thread_create("gprs", rt_gprs_thread_entry, RT_NULL, 512, 20, 20);

            if (init_gprs_thread != RT_NULL) {
                rt_thddog_register(init_gprs_thread, 120);
                rt_thread_startup(init_gprs_thread);
            }
        }
    }

    /*extern void lua_rtu_init(void);
    lua_rtu_init();*/

    // 先初始化串口

    if(!g_bIsTestMode) {
        xfer_helper_serial_init();
    }

    //while (!das_do_is_enet_up()) {
      //  rt_thread_delay(200);
    //}

    if(!das_do_is_enet_up()){
        sleep(5);
    }
    
    rt_kprintf("webnet_init\n");
    webnet_init();

    if (!g_istestover_poweron && !g_bIsTestMode ) {
        xfer_helper_enet_init();

        for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
            if (g_tcpip_cfgs[n].enable && 
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                (TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type ||
                 TCP_IP_UDP == g_tcpip_cfgs[n].tcpip_type)) {

                if(PROTO_MODBUS_TCP == g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    if (PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBTCPSlavePollReStart(n);
                    } else if (PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBTCPMasterPollReStart(n);
                    }
                } else if(PROTO_MODBUS_RTU_OVER_TCP == g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    if (PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBRTU_OverTCPSlavePollReStart(n);
                    } else if (PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                        xMBRTU_OverTCPMasterPollReStart(n);
                    }
                }
            }
        }

        for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
            if (g_tcpip_cfgs[n].enable &&
                TCP_IP_M_NORMAL == g_tcpip_cfgs[n].mode &&
                TCP_IP_TCP == g_tcpip_cfgs[n].tcpip_type &&
                TCPIP_CLIENT == g_tcpip_cfgs[n].tcpip_cs &&
                (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type ||
                    PROTO_DM101 == g_tcpip_cfgs[n].cfg.normal.proto_type
                )) {
                cc_net_open(n);
            }
        }
    }

    printf("开始激活\n");
    while (!reg_hclient_query()) {
        rt_thddog_feed("redo reg_hclient_query");
        rt_thread_delay(rt_tick_from_millisecond(2000));
    }
    //elog_v("test", "this is a test log!");

    /*for(int i = 0; i < 100; i++) {
        elog_e("test", "testaaaaaaaaaaaaaa----%d", i);
        rt_thddog_feed("elog_e test");
        rt_thread_delay(10);
    }*/

    rt_thddog_unreg_inthd();
}

rt_bool_t g_zigbee_init = RT_FALSE;
void rt_init_zigbee_thread_entry(void *parameter)
{
    /*zigbee 初始化*/
    if (!bZigbeeInit(RT_FALSE)) {
        rt_kprintf("zigbee init err!\n");
        while (1) rt_thread_delay(1 * RT_TICK_PER_SECOND);
    }

    {
        int retry = 3;
        ZIGBEE_DEV_INFO_T xInfo;
        while (ZIGBEE_ERR_OK != eZigbeeGetDevInfo(&xInfo, &g_zigbee_cfg.ucState, &g_zigbee_cfg.usType, &g_zigbee_cfg.usVer) ) {
            //bZigbeeInit( HW_UART3, RT_TRUE );
            vZigbeeHWReset();
            rt_thread_delay(1 * RT_TICK_PER_SECOND);
            rt_thddog_feed("zgb re_init");
            if(--retry <= 0) {
                g_zigbee_init = RT_FALSE;
                elog_w("init_zgb", "zigbee init err 3 times");
                goto _END;
            }
        }

        if (xInfo.WorkMode == ZIGBEE_WORK_END_DEVICE &&
            g_zigbee_cfg.xInfo.WorkMode == ZIGBEE_WORK_COORDINATOR) {
            xInfo.WorkMode = ZIGBEE_WORK_COORDINATOR;
        }
        g_zigbee_cfg.xInfo = xInfo;

        if (!g_xfer_net_dst_uart_occ[BOARD_ZIGBEE_UART]) {
            if (ZIGBEE_WORK_END_DEVICE == g_zigbee_cfg.xInfo.WorkMode ||
                ZIGBEE_WORK_ROUTER == g_zigbee_cfg.xInfo.WorkMode) {
                xMBRTU_ASCIISlavePollReStart(BOARD_ZIGBEE_UART, MB_RTU);
            } else if (ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode) {
                xMBRTU_ASCIIMasterPollReStart(BOARD_ZIGBEE_UART, MB_RTU);
            }
        }
    }

    g_zigbee_init = RT_TRUE;
    rt_kprintf("zigbee init ok! ver = %d\n", g_zigbee_cfg.usVer);

_END:
    rt_thddog_unreg_inthd();
}

void rt_gprs_thread_entry(void *parameter)
{
    gprs_do_init();

    while (g_gprs_work_cfg.eWMode != GPRS_WM_SHUTDOWN) {
        if (!das_do_is_gprs_up()) {
            if (is_tcpip_used_gprs()) {
                gprs_do_reinit(0);
            }
        }
        sleep(5);
        rt_thddog_feed("");
    }
    
    rt_thddog_unreg_inthd();
}

void rt_elog_init(void);

int rt_application_init()
{
    my_system("debug off");
    das_sys_time_init();
    rt_elog_init();
    net_init_all();
    rt_thread_idle_init();
    rt_init_thread_entry(NULL);
    return 0;
}

/*
void HardFault_Handler(void)
{
    list_mem();
    list_thread();
    while (1);
}*/

static void elog_user_assert_hook(const char* ex, const char* func, size_t line);

void rt_elog_init(void)
{
    /* initialize EasyFlash and EasyLogger */
    if ((elog_init() == ELOG_NO_ERR)) {
        /* set enabled format */
        elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL & ~ELOG_FMT_P_INFO);
        elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
        elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
        elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
        /* set EasyLogger assert hook */
        elog_assert_set_hook(elog_user_assert_hook);
        /* start EasyLogger */
        elog_start();
    } else {
        /* initialize fail and switch to fault status */
    }
}

static void elog_user_assert_hook(const char* ex, const char* func, size_t line)
{
#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    /* disable async output */
    elog_async_enabled(false);
#endif

    /* disable logger output lock */
    elog_output_lock_enabled(false);
    /* output logger assert information */
    elog_a("elog", "(%s) has assert failed at %s:%d.", ex, func, line);
    while(1);
}

/*@}*/

