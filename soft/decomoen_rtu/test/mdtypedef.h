/**************************************************************************************************
  Revised:        2014-12-04
  Author:         Zhu Jie . Jay . Sleepace
**************************************************************************************************/

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   重定义
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

#ifndef __MD_TYPEDEF_H__
#define __MD_TYPEDEF_H__

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define MD_ABS(_x)      ((_x)>=0?(_x):(-_x))
#define MD_OFS(_t, _m)  (unsigned int)(&(((_t *)0)->_m))
#define MD_IS_UPPER(_c) (((_c)>='A')&&((_c)<='Z'))
#define MD_IS_LOWER(_c) (((_c)>='a')&&((_c)<='z'))
#define MD_IS_DIGIT(_c) (((_c)>='0')&&((_c)<='9'))


#define mdNULL      0
#define mdTRUE      1
#define mdFALSE     0

typedef int8_t          mdINT8;
typedef int16_t         mdINT16;
typedef int32_t         mdINT32;
typedef int64_t         mdINT64;

typedef uint8_t         mdUINT8;
typedef uint16_t        mdUINT16;
typedef uint32_t        mdUINT32;
typedef uint64_t        mdUINT64;

// Special numbers

typedef mdUINT8             mdBOOL;
typedef mdUINT16	    mdWORD;
typedef mdUINT32	    mdDWORD;

// Unsigned numbers
typedef unsigned char     mdBYTE;
typedef unsigned char     byte;
typedef unsigned char     *pbyte;
typedef unsigned char     u8;
typedef unsigned char     mdUCHAR;

typedef unsigned short  mdUSHORT;
typedef unsigned short  u16;
typedef unsigned short  ushort;
// int, long 平台相关
typedef unsigned int    mdUINT;
typedef unsigned int    u32;
typedef unsigned long   mdULONG;

// Signed numbers
typedef signed char     mdCHAR;
typedef signed short    mdSHORT;
typedef signed short    i16;
// int, long 平台相关
typedef signed int      mdINT;
typedef signed int      i32;
typedef signed long     mdLONG;

// decimal
typedef float           mdFLOAT;
typedef double          mdDOUBLE;

typedef enum {
    MD_ENOERR,      // 正常
    MD_EPARAM,      // 参数异常
    MD_EOVERFLOW,   // 越界
    MD_ENOINIT,     // 未初始化

    MD_EBUSY,       // 忙
    MD_ETIMEOUT,    // 超时

    MD_EUNKNOWN     // 未知错误
} eMDErrorCode;


typedef enum {
    MD_VER_TYPE_INVALID     = 0x00,     //无效
    MD_VER_TYPE_DEBUG       = 0x01,     //调试版
    MD_VER_TYPE_BETA        = 0x02,     //测试版
    MD_VER_TYPE_RELEASE     = 0x03,     //发行版本
} eMDVerType;

typedef enum {
    MD_IDE_TYPE_IAR         = 0x00,     //IAR
    MD_IDE_TYPE_KEIL        = 0x01,     //KEIL
    MD_IDE_TYPE_GCC         = 0x02,     //GCC
} eMDIDEType;

typedef enum {
    
    DM_MODEL_NONE			        = 0x00,   //未知

    DM_S101L01G   =    0x0101,  //ADC采集模块(GPRS版本)
    DM_S102L01G	  =    0x0102,  //DO采集模块(GPRS版本)
    DM_S103L01G   =	   0x0103,  //DI采集模块(GPRS版本)
    
    DM_S200L01N	  =    0x0200,  //井盖传感器(NB-IOT版本)
    
    DM_S300L01	  =    0x0300,  //DTU
    
    DM_A400L14G   =	   0x0400,  //通信适配器GPRS版本
    DM_A401L14N   =	   0x0401,  //通信适配器NB-IOT版本
    
    DM_C500L14GZ  =	   0x0500,  //RTU1第一版AD7707
    DM_C501L14GZ  =	   0x0501,  //RTU1第二版AD7689
    DM_C502L14GZ  =	   0x0502,  //RTU1第三版M26
    
    DM_C503H14GZ  =	   0x0503,            //新唐972 RTU第2代   


	MD_MODEL_UNKNOWN        = 0xFFFF
	
} eDMModel;


//产品型号
#define DM_S101L01G_PRODUCT_NAME     "DM-S101L01G"   		//ADC采集模块(GPRS版本)
#define DM_S102L01G_PRODUCT_NAME	 "DM-S102L01G"			//DO采集模块(GPRS版本)
#define DM_S103L01G_PRODUCT_NAME     "DM-S103L01G" 		    //DI采集模块(GPRS版本)
#define DM_S200L01N_PRODUCT_NAME	 "DM-S200L01N"			//井盖传感器(NB-IOT版本)
#define DM_S300L01_PRODUCT_NAME	     "DM-S300L01"			//DTU
#define DM_A400L14G_PRODUCT_NAME     "DM-A400L14G" 		    //通信适配器GPRS版本
#define DM_A401L14N_PRODUCT_NAME     "DM-A401L14N" 		    //通信适配器NB-IOT版本
#define DM_C500L14GZ_PRODUCT_NAME    "DM-C500L14GZ"			//RTU1第一版AD7707
#define DM_C501L14GZ_PRODUCT_NAME    "DM-C501L14GZ"			//RTU1第二版AD7689
#define DM_C502L14GZ_PRODUCT_NAME    "DM-C502L14GZ"			//RTU1第三版M26
#define DM_C503H14GZ_PRODUCT_NAME    "DM-C503H14GZ"			//RTU2 Nuc972方案



#define BIN_MAGIC_WORD      (0xF57BA045930AE418ULL)
typedef struct xMD_BIN_INFO {
    mdINT64   ullMagic;     // 8字节魔数(指纹信息,防止读取错误信息)
    mdBYTE    btIDEType;    // 开发环境
    mdUSHORT  usModel;      // 设备类型
    mdBYTE    btVerType;    // 固件类型
    mdUSHORT  usVerCode;    // 固件版本
} MD_BinInfo_t;

typedef enum 
{
    SOFT_VER_TYPE_MODULE,        //模块软件
    SOFT_VER_TYPE_PRODUCT,       //产品软件
    
}MD_SOFT_VER_TYPE;


//ver_str:  输出字符串
//_p_name:  产品型号名称
//_sVer:    软件版本
//_hVer:    硬件版本
//_VerType: 版本类型 eMDVerType
//_date:  180503    //2018年5月3号
//_soft_ver:  软件版本类型 M 模块软件版本  P产品软件版本


#define DM_VER_FORMAT(ver_str, _pName,_sVer,hVer,_VerType,_date,_soft_type) do {\
    char *ver_type =( _VerType==MD_VER_TYPE_DEBUG) ?"debug":(_VerType==MD_VER_TYPE_BETA?"beta":"release");\
    char *soft_ver_type = (_soft_type == SOFT_VER_TYPE_MODULE)?"M":"P";\
    sprintf(ver_str,"%s_V%d.%02d_%s_%s_%s%d.%02d",_pName,_sVer/100,_sVer%100,_date,ver_type,soft_ver_type,hVer/100,hVer%100);\
}while(0);

#endif

