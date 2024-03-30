/*
 * File      : board.c
 */

#include "board.h"

s_Rs232_Rs485_Stat gUartType;

void rt_hw_board_init(void) {
    threaddog_init();
    vTestRs232Rs485(&gUartType);
    if (gUartType.eUart0Type == UART_TYPE_232) {
        rt_kprintf(" uart 0 232\r\n"); 
    } else {
        rt_kprintf(" uart 0 485\r\n");
    }

    if (gUartType.eUart1Type == UART_TYPE_232) {
        rt_kprintf(" uart 1 232\r\n");
    } else {
        rt_kprintf(" uart 1 485\r\n");
    }

    if (gUartType.eUart2Type == UART_TYPE_232) {
        rt_kprintf(" uart 2 232\r\n");
    } else {
        rt_kprintf(" uart 2 485\r\n");
    }

    if (gUartType.eUart3Type == UART_TYPE_232) {
        rt_kprintf(" uart 3 232\r\n");
    } else {
        rt_kprintf(" uart 3 485\r\n");
    }
    vAd7689Init();
    vInOutInit();
}

#define DEV_NAME		"/dev/dm_io"
#define TTY_UART_CTL    _IO('T',100)
#define TTYS1_TEST  0
#define TTYS2_TEST  1

typedef struct {
	unsigned char gpio;
	unsigned char dir; //0 input 1 output
	unsigned char val;
}dm_io_t;

void vTestRs232Rs485(s_Rs232_Rs485_Stat *pState) 
{
    pState->eUart2Type = UART_TYPE_485;
    pState->eUart3Type = UART_TYPE_485;

	int fd = -1;	
	fd = open(DEV_NAME, O_RDWR);
	if (fd == -1) {		
		printf("Cannot open %s!\n",DEV_NAME);		
		return;	
	}
    
	dm_io_t iodata;
    
	iodata.gpio = TTYS1_TEST;
	iodata.dir = 0;
	iodata.val = 1;
    if (ioctl(fd, TTY_UART_CTL, &iodata) < 0) {	
        printf("TTY_UART_CTL set fail\n");
    }
    if (iodata.val == 1) {
		pState->eUart1Type = UART_TYPE_232;
    } else {
		pState->eUart1Type = UART_TYPE_485;
    }

    
    iodata.gpio = TTYS2_TEST;
	iodata.dir = 0;
	iodata.val = 1;
    if (ioctl(fd, TTY_UART_CTL,&iodata) < 0) {   
		printf("TTY_UART_CTL set fail\n");
    }
    if (iodata.val == 1) {
		pState->eUart0Type = UART_TYPE_232;
    } else {
		pState->eUart0Type = UART_TYPE_485;
    }
	close(fd);
}

eCELLNetTYPE_t g_xCellNetType = E_GPRS_M26;

eCELLNetTYPE_t vCheckCellNetType(void)
{
    if(access("/dev/ttyUSB0", R_OK) == 0){
       // rt_kprintf("\n============= 4G MODULE EC20 IS CHECK!\n");
        g_xCellNetType = E_4G_EC20;
        return E_4G_EC20;
    }else {
        //rt_kprintf("\n============= 2G MODULE M26 IS CHECK!\n");
        g_xCellNetType = E_GPRS_M26;
        return E_GPRS_M26;
    }
}

