#ifndef __VAR_MANAGE_H__
#define __VAR_MANAGE_H__

#include <stdint.h>

#define VAR_MANAGE_MALLOC(_n)           rt_malloc((_n))
#define VAR_MANAGE_CALLOC(_cnt,_n)      rt_calloc((_cnt),(_n))
#define VAR_MANAGE_FREE(_p)             if((_p)) { rt_free((_p)); _p = 0; }
#define VAR_MANAGE_OFS(_t, _m)          (unsigned int)(&(((_t *)0)->_m))

#define VAR_NULL                        0
#define VAR_TRUE                        1
#define VAR_FALSE                       0
#define VAR_BIT_0                       0
#define VAR_BIT_1                       1

typedef char *var_str_t;      /**<  string type */
typedef signed   char                   var_char_t;     /**<  8bit integer type */
typedef signed   char                   var_int8_t;     /**<  8bit integer type */
typedef int16_t                         var_int16_t;    /**< 16bit integer type */
typedef int32_t                         var_int32_t;    /**< 32bit integer type */
typedef int64_t                         var_int64_t;    /**< 64bit integer type */
typedef uint8_t                         var_uint8_t;    /**<  8bit unsigned integer type */
typedef unsigned char                   var_uchar_t;    /**<  8bit unsigned integer type */
typedef uint8_t                         var_byte_t;     /**<  8bit unsigned integer type */
typedef uint8_t                         var_bit_t;      /**<  8bit unsigned integer type */
typedef uint16_t                        var_uint16_t;   /**< 16bit unsigned integer type */
typedef uint32_t                        var_uint32_t;   /**< 32bit unsigned integer type */
typedef uint64_t                        var_uint64_t;   /**< 64bit unsigned integer type */
typedef var_uint8_t                     var_bool_t;     /**< boolean type */

typedef int                             var_int_t;
typedef unsigned int                    var_uint_t;

typedef float                           var_float_t;
typedef double                          var_double_t;

/* 32bit CPU */
typedef long                            var_base_t;     /**< Nbit CPU related date type */
typedef unsigned long                   var_ubase_t;    /**< Nbit unsigned CPU related data type */

typedef enum {
    E_VAR_BIT,
    E_VAR_INT8,
    E_VAR_UINT8,
    E_VAR_INT16,
    E_VAR_UINT16,
    E_VAR_INT32,
    E_VAR_UINT32,
    E_VAR_FLOAT,
    E_VAR_DOUBLE,
    E_VAR_ARRAY,        // can be a utf8 string or binary buffer

    E_VAR_ERROR,
} eVarType;

typedef enum {
    E_VAR_RO,
    E_VAR_WO,
    E_VAR_RW,
} eVarRWType;

typedef enum {
    E_ERR_OP_NONE       = 0x00,
    E_ERR_OP_CLEAR,
} eErrOpType;

#define VAR_TYPE_SIZE_ARRAY         { 1, 1, 1, 2, 2, 4, 4, 4, 8, 1 }

typedef union {
    var_bit_t       val_bit;
    var_int8_t      val_i8;
    var_uint8_t     val_u8;
    var_int16_t     val_i16;
    var_uint16_t    val_u16;
    var_int32_t     val_i32;
    var_uint32_t    val_u32;
    var_float_t     val_f;
    var_double_t    val_db;
    var_uint16_t    val_reg[1];         // 最多16个寄存器
    var_uint8_t     val_ary[1];         // 最多32个
} VarValue_v;

typedef struct {
    var_int_t       len;
    char *str;
} VarString_t;

typedef struct {
    var_uint64_t    time;           // time
    var_uint32_t    count;          // count
    var_double_t    val_avg;        // 均值
    var_float_t     val_cur;        // 当前值
    var_float_t     val_min;        // 最小值
    var_float_t     val_max;        // 最大值
} VarAvg_t;

typedef struct {
    char            szName[33];      // has '\0'
} VarName_t;

typedef struct {
    char            szAlias[33];     // has '\0'
} VarAlias_t;

#define VAR_EXT_DATA_SIZE           (256)

// add by jay 2016/11/26
// 单字节对齐, 可以节省一定的字节数, 对效率的影响可以忽略
//#pragma pack(1)

typedef struct {
    var_uint8_t addr[6];
} dlt645_addr_t;

typedef struct {
    var_uint8_t     btOpCode;       // 功能码
    var_int_t       nSyncFAddr;     // 同帧地址
    var_uint8_t     btSlaveAddr;    // 从机地址
    var_uint16_t    usExtAddr;      // 外部关联地址
    var_uint16_t    usAddrOfs;      // 外部关联地址偏移
} ExtData_IO_modbus_t;

typedef struct {
    dlt645_addr_t   addr;           // 设备地址
    var_uint32_t    op;             // 功能码
} ExtData_IO_dlt645_t;

typedef struct {
    var_uint8_t     op;             // 功能码
} ExtData_IO_dust_t;

typedef union {
    ExtData_IO_modbus_t     modbus; // mosbus 结构
    ExtData_IO_dlt645_t     dlt645; // dlt645 结构, 串口专用协议
    ExtData_IO_dust_t       dust;   // dust 结构, 串口专用协议
} ExtData_IO_Param_t;

typedef struct {
    var_uint8_t     btRWType;       // 读写类型
    var_uint8_t     btVarType;      // 数据类型
    var_uint8_t     btVarSize;      // 数据大小, val_ary时, 表示数组大小
    var_uint8_t     btVarRuleType;  // 编码形式
    VarValue_v      xValue;         // 值
    var_float_t     fMax, fMin;     // 最大/小值
    var_float_t     fInit;          // 初始值
    var_float_t     fRatio;         // 变比
    var_bool_t      bUseMax,bUseMin;
    var_bool_t      bUseInit,bUseRatio;

    var_uint16_t    usDevType;      // 关联硬件
    var_uint16_t    usDevNum;       // 关联硬件编号
    var_uint16_t    usProtoType;    // 关联协议编号
    char            *szProtoName;   // 第三方协议名称(如：LUA协议名称)

    var_uint16_t    usAddr;         // 内部关联地址

    var_uint8_t     btErrOp;
    var_uint8_t     btErrCnt;
    var_uint8_t     btErrNum;       // **内部使用

    ExtData_IO_Param_t      param;

    char            *szExp;         // 表达式
} ExtData_IO_t;

typedef struct {
    var_bool_t      bEnable;        // 是否报警
} ExtData_Alarm_t;

typedef struct {
    var_bool_t      bEnable;        // 是否存盘
    var_uint8_t     btType;         // 存盘方式
    var_uint32_t    ulStep;         // 存盘间隔

    VarAvg_t        xAvgReal;       // 实时均值器
    VarAvg_t        xAvgMin;        // 分钟均值器
    VarAvg_t        xAvgHour;       // 小时均值器

    var_bool_t      bCheck;
    var_float_t     fMax, fMin;
} ExtData_Storage_t;

typedef struct _ExtDataUp {
    var_bool_t      bEnable;        // 有效标志

    VarAvg_t        xAvgUp;         // 上传均值器
    VarAvg_t        xAvgMin;        // 分钟均值器
    VarAvg_t        xAvg5Min;       // 5分钟均值器
    VarAvg_t        xAvgHour;       // 小时均值器

    char            *szNid;         // nid
    char            *szFid;         // fid
    char            *szUnit;        // unit
    
    var_uint8_t     pi;             // 小数点位数

    var_uint16_t    usDevType;      // 关联硬件
    var_uint16_t    usDevNum;       // 关联硬件编号
    var_uint16_t    usProtoType;    // 关联协议编号
    char            *szProtoName;   // 第三方协议名称(如：LUA协议名称)

    char            *szDesc;        // 描述
} ExtData_Up_t;

#define EXT_DATA_GET_NID(_nid)      ((_nid && _nid[0])?_nid:g_host_cfg.szId)

typedef struct _ExtData {
    var_bool_t      bEnable;        // 有效标志, 无效时, 后面的数据均无意义

    VarName_t       xName;          // 名称
    VarAlias_t      xAlias;         // 别名

    ExtData_IO_t        xIo;        // IO连接属性
    ExtData_Alarm_t     xAlarm;     // 报警属性
    ExtData_Storage_t   xStorage;   // 存盘属性
    ExtData_Up_t        xUp;

    struct _ExtData *next, *prev;   // 链表管理
} ExtData_t;

//#pragma pack()

typedef struct {
    int             n;              // 数目 <= VAR_EXT_DATA_SIZE
    ExtData_t       *pList;         // 链表
} ExtDataList_t;

var_uint16_t var_htons(var_uint16_t n);
var_uint32_t var_htonl(var_uint32_t n);
void vVarManage_ExtDataInit(void);
var_bool_t bVarManage_CheckExtValue(ExtData_t *data, VarValue_v *var);
var_bool_t bVarManage_GetExtValue(ExtData_t *data, var_double_t *value);
var_bool_t bVarManage_GetExtValueWithName(const char *name, var_double_t *value);
var_bool_t bVarManage_SetExtDataValue(ExtData_t *data, VarValue_v *pValue);
var_bool_t bVarManage_SetExtDataValueWithName(const char *name, VarValue_v *pValue);
ExtData_t* pVarManage_GetExtDataWithAddr(var_uint16_t usAddr, ExtData_t *data);
ExtData_t* pVarManage_GetExtDataWithSlaveAddr(var_uint16_t usExtAddr, var_uint8_t btSlaveAddr, ExtData_t *data);
var_int32_t lVarManage_GetExtDataUpInterval(var_uint16_t usDevType, var_uint16_t usDevNum);
void vVarManage_SetExtDataUpInterval(var_uint16_t usDevType, var_uint16_t usDevNum, var_uint32_t interval);
ExtData_t* pVarManage_GetExtDataWithUpProto(
    ExtData_t       *first,
    var_uint16_t    usDevType,
    var_uint16_t    usDevNum,
    var_uint16_t    usProtoType
    );
void vVarManage_ExtDataRefreshTask(void *parameter);
void vVarManage_ExtDataUpTask(void *parameter);

void vVarManage_TakeMutex( void );
void vVarManage_ReleaseMutex( void );

void bVarManage_UpdateAvgValue(VarAvg_t *varavg, var_double_t value);

var_bool_t bVarManage_CheckContAddr(var_uint16_t usAddr, var_uint16_t usNRegs);
var_bool_t bVarManage_RefreshExtDataWithModbusSlave(var_uint16_t usAddress, var_uint16_t usNRegs);
var_bool_t bVarManage_RefreshExtDataWithModbusMaster(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum, 
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs, 
    var_uint8_t *pucRegBuffer
);
var_bool_t bVarManage_ReadExtDataWithModbusMaster(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum, 
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs, 
    var_uint8_t *pucRegBuffer 
);

extern var_uint16_t        *g_xExtDataRegs;
extern var_bool_t          g_xExtDataRegsFlag[];

#define var_check_diff(_t_now,_t_old,_t_diff) ((_t_old) > (_t_now) || (_t_now) - (_t_old) >= (_t_diff))

void varmanage_start(void);
void varmanage_update(void);

#endif

