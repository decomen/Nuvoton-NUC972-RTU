#include <board.h>
#include <stdio.h>
#include "modbus.h"
#include "dlt645.h"
#include "dlt645_1997.h"
#include "net_helper.h"
#include "sdccp_dust.h"
#include "modbus_helper.h"

typedef struct varmanage_run {
    var_uint16_t    dev_type;
    var_uint16_t    dev_num;
    rt_thread_t     thread;
    int             use;
} varmanage_run_t;

static varmanage_run_t s_varmanage_run[BOARD_COLL_MAX];

// 1024-4096 采集变量地址映射表
var_uint16_t        *g_xExtDataRegs = &g_modbus_regs[USER_REG_EXT_DATA_START];
var_bool_t          g_xExtDataRegsFlag[USER_REG_EXT_DATA_SIZE];

static rt_tick_t s_com_last_tick[BOARD_UART_MAX] = {0};

// 同帧地址链表 2级链表

#define _SYNC_EXT_MAX_SIZE          (32)    //同帧链最多每次读取32个寄存器

typedef struct _sync_ext_t {
    union {
        struct {
            var_uint16_t    addr;           // 映射地址
        } self;
        struct {
            var_uint16_t    addr;           // 升序
            var_uint16_t    ofs;            // 偏移
            var_uint8_t     size;           // 寄存器数目
        } modbus;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint32_t    op;             // 功能码
        } dlt645;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint8_t     op;             // 功能码
        } dust;
        // ...
    } param;
    struct _sync_ext_t *next,*prev; // 链表管理
} sync_ext_t;

typedef struct _sync_slave_t {
    var_uint8_t     dev_type;       // 关联硬件
    var_uint8_t     dev_num;        // 关联硬件编号
    var_uint8_t     proto_type;     // 关联协议编号
    union {
        // modbus 按 从机地址+同帧地址 分组
        struct {
            var_uint8_t     op;             // 功能码
            var_uint8_t     slave_addr;     // 从机地址
            var_int8_t      fa_addr;        // 同帧地址
        } modbus;
        // dlt645 按 设备地址 分组
        struct {
            dlt645_addr_t   addr;             // 设备地址
        } dlt645;
        // ...
    } param;
    struct _sync_ext_t  *ext_list;  // 采集链(按地址升序)
    struct _sync_slave_t *next;     // 链表管理
} sync_slave_t;

typedef struct _sync_node_t {
    var_uint8_t     dev_type;       // 关联硬件
    var_uint8_t     dev_num;        // 关联硬件编号
    var_uint8_t     proto_type;     // 关联协议编号
    union {
        struct {
            var_uint16_t    addr;           // 映射地址
        } self;
        struct {
            var_uint8_t     op;             // 功能码
            var_uint8_t     slave_addr;     // 从机地址
            var_uint16_t    ext_addr;       // 升序
            var_uint16_t    ext_ofs;        // 偏移
            var_uint8_t     ext_size;       // 寄存器数目
        } modbus;
        struct {
            dlt645_addr_t   dlt645_addr;    // 设备地址
            var_uint16_t    addr;           // 映射地址
            var_uint32_t    op;             // 功能码
        } dlt645;
        struct {
            var_uint16_t    addr;           // 映射地址
            var_uint8_t     op;             // 功能码
        } dust;
        // ...
    } param;
} sync_node_t;

static sync_slave_t     *s_psync_slave = VAR_NULL;

static ExtDataList_t    s_xExtDataList;

static pthread_mutex_t s_varmanage_mutex;

// storage_cfg
rt_bool_t board_cfg_varext_loadlist( ExtDataList_t *pList );
rt_bool_t board_cfg_varext_del_all(void);
rt_bool_t board_cfg_varext_del(const char *name);
rt_bool_t board_cfg_varext_update( const char *name, ExtData_t *data);
rt_bool_t board_cfg_varext_add(ExtData_t *data);

var_uint16_t var_htons(var_uint16_t n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

var_uint32_t var_htonl(var_uint32_t n)
{
    return ((n & 0xff) << 24) |
           ((n & 0xff00) << 8) |
           ((n & 0xff0000UL) >> 8) |
           ((n & 0xff000000UL) >> 24);
}

static void __RefreshRegsFlag(void);
static void __sort_extdata_link(void);
static var_bool_t __creat_sync_slave_link(void);

static void prvSetExtDataCfgDefault(void)
{
    s_xExtDataList.n = 0;
    s_xExtDataList.pList = VAR_NULL;
}

void vVarManage_ExtDataInit(void)
{
    s_xExtDataList.n = 0;
    s_xExtDataList.pList = VAR_NULL;

    for(int i = 0; i < BOARD_UART_MAX; i++) {
        s_com_last_tick[i] = (rt_tick_t)-1;
    }

    if( !board_cfg_varext_loadlist(&s_xExtDataList) ) {
        prvSetExtDataCfgDefault();
    }

    __sort_extdata_link();
    __RefreshRegsFlag();
    __creat_sync_slave_link();
    
    if (rt_mutex_init(&s_varmanage_mutex, "varmtx", RT_IPC_FLAG_FIFO) != RT_EOK) {
        rt_kprintf("init varmanage mutex failed\n");
    }
}

// add by jay 2016/12/27   考虑第三方协议节点

static var_bool_t __proto_is_rtu_self(int dev_type, int dev_num)
{
    return (PROTO_DEV_RTU_SELF == dev_type && dev_num < 8);
}

static var_bool_t __proto_is_rtu_self_mid(int dev_type)
{
    return (PROTO_DEV_RTU_SELF_MID == dev_type);
}

static var_bool_t __proto_is_modbus(int dev_type, int proto_type)
{
    if (PROTO_DEV_IS_RS(dev_type)) {
        return (proto_type <= PROTO_MODBUS_ASCII);
    } else if (PROTO_DEV_IS_NET(dev_type) || PROTO_DEV_IS_GPRS(dev_type)) {
        return (proto_type <= PROTO_MODBUS_TCP || proto_type == PROTO_MODBUS_RTU_OVER_TCP);
    } else if (PROTO_DEV_IS_ZIGBEE(dev_type)) {
        return (proto_type <= PROTO_MODBUS_ASCII);
    }

    return VAR_FALSE;
}

static var_bool_t __proto_is_dlt645_2007(int dev_type, int proto_type)
{
    return (PROTO_DEV_IS_RS(dev_type) && proto_type == PROTO_DLT645);
}

static var_bool_t __proto_is_dlt645_1997(int dev_type, int proto_type)
{
    return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DLT645_1997);
}

static var_bool_t __proto_is_dlt645(int dev_type, int proto_type)
{
    return __proto_is_dlt645_2007(dev_type, proto_type) || __proto_is_dlt645_1997(dev_type, proto_type);
}


static var_bool_t __proto_is_dust(int dev_type, int proto_type)
{
    return (dev_type <= PROTO_DEV_RS_MAX && proto_type == PROTO_DUST);
}

// add by jay 2016/11/29   暂时未考虑第三方自定义协议节点

static void __free_sync_slave_link(void)
{
    while (s_psync_slave) {
        sync_slave_t *next_slave = s_psync_slave->next;
        sync_ext_t *list_ext = s_psync_slave->ext_list;
        while (list_ext) {
            sync_ext_t *next_ext = list_ext->next;
            VAR_MANAGE_FREE(list_ext);
            list_ext = next_ext;
        }
        VAR_MANAGE_FREE(s_psync_slave);
        s_psync_slave = next_slave;
    }
}

void varmanage_free_all(void)
{
    rt_enter_critical();
    ExtData_t *node = s_xExtDataList.pList;
    while (node) {
        ExtData_t *next = node->next;
        if (node) {
            // io
            VAR_MANAGE_FREE(node->xIo.szProtoName);
            VAR_MANAGE_FREE(node->xIo.szExp);

            // up
            VAR_MANAGE_FREE(node->xUp.szNid);
            VAR_MANAGE_FREE(node->xUp.szFid);
            VAR_MANAGE_FREE(node->xUp.szUnit);
            VAR_MANAGE_FREE(node->xUp.szProtoName);
            VAR_MANAGE_FREE(node->xUp.szDesc);

            //node
            VAR_MANAGE_FREE(node);
        }
        node = next;
    }
    s_xExtDataList.n = 0;
    s_xExtDataList.pList = VAR_NULL;
    rt_exit_critical();
    __free_sync_slave_link();
}

static sync_slave_t *__add_sync_slave_node(ExtData_t *data)
{
    sync_slave_t *node = VAR_MANAGE_CALLOC(1, sizeof(sync_slave_t));
    if (node) {
        sync_slave_t *last = s_psync_slave;
        while (last && last->next) last = last->next;
        if (last) {
            last->next = node;
        } else {
            s_psync_slave = node;
        }
        node->dev_type = data->xIo.usDevType;
        node->dev_num = data->xIo.usDevNum;
        node->proto_type = data->xIo.usProtoType;
        node->ext_list = VAR_MANAGE_CALLOC(1, sizeof(sync_ext_t));
        if (__proto_is_rtu_self(node->dev_type, node->dev_num)) {
            if (node->ext_list) {
                node->ext_list->param.self.addr = data->xIo.usAddr;
            }
        } else if (__proto_is_rtu_self_mid(node->dev_type)) {
            if (node->ext_list) {
                node->ext_list->param.self.addr = data->xIo.usAddr;
            }
        } else if (__proto_is_modbus(node->dev_type, node->proto_type)) {
            node->param.modbus.op = data->xIo.param.modbus.btOpCode;
            node->param.modbus.slave_addr = data->xIo.param.modbus.btSlaveAddr;
            node->param.modbus.fa_addr = data->xIo.param.modbus.nSyncFAddr;
            if (node->ext_list) {
                node->ext_list->param.modbus.addr = data->xIo.param.modbus.usExtAddr;
                node->ext_list->param.modbus.ofs = data->xIo.param.modbus.usAddrOfs;
                node->ext_list->param.modbus.size = (data->xIo.btVarSize+1)/2;
            }
        } else if (__proto_is_dlt645(node->dev_type, node->proto_type)) {
            node->param.dlt645.addr = data->xIo.param.dlt645.addr;
            if (node->ext_list) {
                node->ext_list->param.dlt645.addr = data->xIo.usAddr;
                node->ext_list->param.dlt645.op = data->xIo.param.dlt645.op;
            }
        } else if (__proto_is_dust(node->dev_type, node->proto_type)) {
            if (node->ext_list) {
                node->ext_list->param.dust.addr = data->xIo.usAddr;
                node->ext_list->param.dust.op = data->xIo.param.dust.op;
            }
        }
        return node;
    }

    return VAR_NULL;
}

static sync_slave_t *__find_sync_slave_node(ExtData_t *data)
{
    sync_slave_t *node = s_psync_slave;
    if (__proto_is_rtu_self(data->xIo.usDevType, data->xIo.usDevNum)) {
        while (node) {
            if (node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_rtu_self_mid(data->xIo.usDevType)) {
        while (node) {
            if (node->dev_type == data->xIo.usDevType) {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_modbus(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if (node->param.modbus.op == data->xIo.param.modbus.btOpCode && 
                node->param.modbus.fa_addr == data->xIo.param.modbus.nSyncFAddr && 
                node->param.modbus.slave_addr == data->xIo.param.modbus.btSlaveAddr &&
                node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_dlt645(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if ( 0 == memcmp(&node->param.dlt645.addr, &data->xIo.param.dlt645.addr, sizeof(dlt645_addr_t)) && 
                node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    } else if (__proto_is_dust(data->xIo.usDevType, data->xIo.usProtoType)) {
        while (node) {
            if (node->dev_type == data->xIo.usDevType && 
                node->dev_num == data->xIo.usDevNum && 
                node->proto_type == data->xIo.usProtoType) 
            {
                return node;
            }
            node = node->next;
        }
    }
    return VAR_NULL;
}

// 按addr排序
static sync_ext_t *__add_sync_ext_node(ExtData_t *data)
{
    sync_slave_t *node_slave = __find_sync_slave_node(data);
    if (node_slave) {
        sync_ext_t *node_ext = node_slave->ext_list;
        sync_ext_t *node_new = VAR_MANAGE_CALLOC(1, sizeof(sync_ext_t));
        if (node_new) {
            if (__proto_is_rtu_self( node_slave->dev_type, node_slave->dev_num)) {
                node_new->param.self.addr = data->xIo.usAddr;
            } else if (__proto_is_rtu_self_mid( node_slave->dev_type)) {
                node_new->param.self.addr = data->xIo.usAddr;
            } else if (__proto_is_modbus( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.modbus.addr = data->xIo.param.modbus.usExtAddr;
                node_new->param.modbus.ofs = data->xIo.param.modbus.usAddrOfs;
                node_new->param.modbus.size = (data->xIo.btVarSize+1)/2;
                while (node_ext) {
                    // 插入链表, 直接返回
                    if (node_new->param.modbus.addr < node_ext->param.modbus.addr) {
                        sync_ext_t *prev = node_ext->prev;
                        if (prev) {
                            node_new->prev = prev;
                            prev->next = node_new;
                        } else {    // 头结点后移
                            node_slave->ext_list = node_new;
                        }
                        node_ext->prev = node_new;
                        node_new->next = node_ext;
                        return node_new;
                    } else if (
                        node_new->param.modbus.addr == node_ext->param.modbus.addr && 
                        node_new->param.modbus.ofs == node_ext->param.modbus.ofs && 
                        node_new->param.modbus.size == node_ext->param.modbus.size) {
                        VAR_MANAGE_FREE(node_new);
                        return node_ext;
                    }
                    node_ext = node_ext->next;
                }
            } else if (__proto_is_dlt645( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.dlt645.addr = data->xIo.usAddr;
                node_new->param.dlt645.op = data->xIo.param.dlt645.op;
            } else if (__proto_is_dust( node_slave->dev_type, node_slave->proto_type)) {
                node_new->param.dust.addr = data->xIo.usAddr;
                node_new->param.dust.op = data->xIo.param.dust.op;
            }
            // 连接到最后
            node_ext = node_slave->ext_list;
            while (node_ext && node_ext->next) node_ext = node_ext->next;
            node_ext->next = node_new;
            node_new->prev = node_ext;
            return node_new;
        }
    } else {
        node_slave = __add_sync_slave_node(data);
        return node_slave->ext_list;
    }

    return VAR_NULL;
}

// 连续地址节点合并
// change by jay 2016/12/26  目前仅有modbus需要合并地址
static void __trim_sync_slave_link(void)
{
    sync_slave_t *node_slave = s_psync_slave;
    while (node_slave) {
        sync_ext_t *node_ext = node_slave->ext_list;
        if (__proto_is_modbus( node_slave->dev_type, node_slave->proto_type)) {
            while (node_ext) {
                sync_ext_t *next = node_ext->next;
                if (next) {
                    // 可串接
                    // add by jay 2014/04/24 相等才串接(可以处理统一寄存器地址不会采集的问题)
                    if (node_ext->param.modbus.addr + node_ext->param.modbus.size >= next->param.modbus.addr ) {
                        var_uint16_t addr_a = node_ext->param.modbus.addr + node_ext->param.modbus.size;
                        var_uint16_t addr_b = next->param.modbus.addr + next->param.modbus.size;
                        if (addr_a < addr_b) {    //new size
                            // cmp size
                            if ((addr_b-node_ext->param.modbus.addr)<=_SYNC_EXT_MAX_SIZE) {
                                node_ext->param.modbus.size = addr_b-node_ext->param.modbus.addr;
                            } else {
                                node_ext = node_ext->next;
                                continue;
                            }
                        }
                        // del next
                        node_ext->next = next->next;
                        if (next->next) next->next->prev = node_ext;
                        VAR_MANAGE_FREE(next);
                    } else {
                        node_ext = node_ext->next;
                    }
                } else {
                    break;
                }
            }
        }
        node_slave = node_slave->next;
    }
}

static var_bool_t __isexist_sync_ext(sync_slave_t *slave, sync_ext_t *ext)
{
    sync_slave_t *node_slave = s_psync_slave;
    while (node_slave) {
        if (slave==node_slave) {
            sync_ext_t *node_ext = node_slave->ext_list;
            while (node_ext) {
                if (ext==node_ext) {
                    return VAR_TRUE;
                }
                node_ext = node_ext->next;
            }
            return VAR_FALSE;
        }
        node_slave = node_slave->next;
    }
    return VAR_FALSE;
}

// 查找下一个节点, 临界区, 防止链表结构中途变化导致异常
// 通过node返回, 使之线程访问安全
static var_bool_t __get_next_sync_ext_node(sync_slave_t **slave, sync_ext_t **ext, sync_node_t *node)
{
    var_bool_t ret = VAR_FALSE;
    
    rt_enter_critical();
    {
        sync_slave_t *node_slave = *slave;
        sync_ext_t *node_ext = *ext;
        
        if (node_slave && node_ext && __isexist_sync_ext(node_slave, node_ext)) {
            if (node_ext->next) {
                node_ext = node_ext->next;
            } else {
                node_slave = node_slave->next;
                if (node_slave) node_ext = node_slave->ext_list;
            }
        } else {
            node_ext = VAR_NULL;
            node_slave = s_psync_slave;
            if (node_slave) node_ext = node_slave->ext_list;
        }
        if (node_slave && node_ext) {
            node->dev_type = node_slave->dev_type;
            node->dev_num = node_slave->dev_num;
            node->proto_type = node_slave->proto_type;
            if (__proto_is_rtu_self( node_slave->dev_type, node_slave->dev_num)) {
                node->param.self.addr = node_ext->param.self.addr;
            } else if (__proto_is_rtu_self_mid( node_slave->dev_type)) {
                node->param.self.addr = node_ext->param.self.addr;
            } else if (__proto_is_modbus( node_slave->dev_type, node_slave->proto_type)) {
                node->param.modbus.op = node_slave->param.modbus.op;
                node->param.modbus.slave_addr = node_slave->param.modbus.slave_addr;
                node->param.modbus.ext_addr = node_ext->param.modbus.addr;
                node->param.modbus.ext_ofs = node_ext->param.modbus.ofs;
                node->param.modbus.ext_size = node_ext->param.modbus.size;
            } else if (__proto_is_dlt645( node_slave->dev_type, node_slave->proto_type)) {
                node->param.dlt645.dlt645_addr = node_slave->param.dlt645.addr;
                node->param.dlt645.addr = node_ext->param.dlt645.addr;
                node->param.dlt645.op = node_ext->param.dlt645.op;
            } else if (__proto_is_dust( node_slave->dev_type, node_slave->proto_type)) {
                node->param.dust.addr = node_ext->param.dust.addr;
                node->param.dust.op = node_ext->param.dust.op;
            }
            ret = VAR_TRUE;
        }
        *slave = node_slave;
        *ext = node_ext;
    }
    rt_exit_critical();

    return ret;
}

static var_bool_t __creat_sync_slave_link(void)
{
    var_bool_t ret = VAR_FALSE;
    
    rt_enter_critical();
    {
        for(int i = 0; i < BOARD_UART_MAX; i++) {
            s_com_last_tick[i] = (rt_tick_t)-1;
        }
        
        __free_sync_slave_link();

        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                if (node->bEnable) {
                    __add_sync_ext_node(node);
                }
                node = node->next;
            }
            ret = VAR_TRUE;
        }

        __trim_sync_slave_link();
    }
    rt_exit_critical();

    return ret;
}

static void __sort_extdata_link(void)
{
    rt_enter_critical();
    {
        if (s_xExtDataList.pList) {
            ExtData_t *top = s_xExtDataList.pList;
            ExtData_t *next, *node, *tmp;
            node = top;
            while (node) {
                next = node->next;
                if(next) {
                    for(tmp = top; tmp != next; tmp = tmp->next) {
                        if(tmp->prev) {
                            if(next->xIo.usAddr >= tmp->prev->xIo.usAddr && next->xIo.usAddr <= tmp->xIo.usAddr) {
                                node->next = next->next;
                                if(next->next) next->next->prev = node;
                                next->next = tmp;
                                next->prev = tmp->prev;
                                tmp->prev->next = next;
                                if(tmp->next) tmp->next->prev = next;
                                break;
                            }
                        } else {
                            if(next->xIo.usAddr <= tmp->xIo.usAddr) {
                                node->next = next->next;
                                if(next->next) next->next->prev = node;
                                next->next = tmp;
                                next->prev = tmp->prev;
                                if(tmp->next) tmp->next->prev = next;
                                s_xExtDataList.pList = next;
                                top = s_xExtDataList.pList;
                                break;
                            }
                        }
                    }
                }
                node = node->next;
            }
        }
    }
    rt_exit_critical();
}

static ExtData_t* __GetExtDataWithName(const char *name)
{
    ExtData_t *ret = VAR_NULL;
    if (name && s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (0 == strcmp(name, node->xName.szName)) {
                ret = node;
                break;
            }
            node = node->next;
        }
    }
    return ret;
}

void vVarManage_InsertNode(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *last = s_xExtDataList.pList;
        while (last && last->next) last = last->next;
        if (last) {
            last->next = data;
            data->prev = last;
        } else {
            s_xExtDataList.pList = data;
        }
        s_xExtDataList.n++;
    }
    rt_exit_critical();
}

ExtData_t *pVarManage_CopyData( ExtData_t *in )
{
    ExtData_t *out = VAR_NULL;
    rt_enter_critical();
    {
        if( in ) {
            out = VAR_MANAGE_CALLOC(1, sizeof(ExtData_t));
            if (out) {
                memcpy( out, in, sizeof(ExtData_t) );

                // io
                out->xIo.szProtoName = rt_strdup(in->xIo.szProtoName);
                out->xIo.szExp = rt_strdup(in->xIo.szExp);

                // up
                out->xUp.szNid = rt_strdup(in->xUp.szNid);
                out->xUp.szFid = rt_strdup(in->xUp.szFid);
                out->xUp.szUnit = rt_strdup(in->xUp.szUnit);
                out->xUp.szProtoName = rt_strdup(in->xUp.szProtoName);
                out->xUp.szDesc = rt_strdup(in->xUp.szDesc);
            }
        }
    }
    rt_exit_critical();

    return out;
}

// add by jay 2016/11/25
// *data must heap point
void vVarManage_FreeData( ExtData_t *data )
{
    if(data) {
        VAR_MANAGE_FREE(data->xIo.szProtoName);
        VAR_MANAGE_FREE(data->xIo.szExp);

        // up
        VAR_MANAGE_FREE(data->xUp.szNid);
        VAR_MANAGE_FREE(data->xUp.szFid);
        VAR_MANAGE_FREE(data->xUp.szUnit);
        VAR_MANAGE_FREE(data->xUp.szProtoName);
        VAR_MANAGE_FREE(data->xUp.szDesc);

        VAR_MANAGE_FREE(data);
    }
}

void vVarManage_RemoveNodeWithName(const char *name)
{
    rt_enter_critical();
    {
        ExtData_t *data = __GetExtDataWithName(name);
        if (data) {
            ExtData_t *prev = data->prev;
            ExtData_t *next = data->next;
            if (prev) {
                prev->next = next;
            } else {    // 头结点后移
                s_xExtDataList.pList = next;
            }
            if (next) next->prev = prev;
            s_xExtDataList.n--;

            // io
            VAR_MANAGE_FREE(data->xIo.szProtoName);
            VAR_MANAGE_FREE(data->xIo.szExp);

            // up
            VAR_MANAGE_FREE(data->xUp.szNid);
            VAR_MANAGE_FREE(data->xUp.szFid);
            VAR_MANAGE_FREE(data->xUp.szUnit);
            VAR_MANAGE_FREE(data->xUp.szProtoName);
            VAR_MANAGE_FREE(data->xUp.szDesc);

            //node
            VAR_MANAGE_FREE(data);

            if (0 == s_xExtDataList.n) {
                s_xExtDataList.pList = VAR_NULL;
            }
        }
    }
    rt_exit_critical();
}

// 调用后, 必须进行释放
ExtData_t* __GetExtDataWithIndex(int n)
{
    ExtData_t *ret = VAR_NULL;
    rt_enter_critical();
    {
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node && n--) {
                node = node->next;
            }
            ret = pVarManage_CopyData(node);
            
        }
    }
    rt_exit_critical();
    return ret;
}

static var_bool_t __GetExtValue(var_int8_t type, VarValue_v *var, var_double_t *value)
{
    var_bool_t ret = VAR_TRUE;
    switch (type) {
    case E_VAR_BIT:
        *value = var->val_bit;
        break;
    case E_VAR_INT8:
        *value = var->val_i8;
        break;
    case E_VAR_UINT8:
        *value = var->val_u8;
        break;
    case E_VAR_INT16:
        *value = var->val_i16;
        break;
    case E_VAR_UINT16:
        *value = var->val_u16;
        break;
    case E_VAR_INT32:
        *value = var->val_i32;
        break;
    case E_VAR_UINT32:
        *value = var->val_u32;
        break;
    case E_VAR_FLOAT:
        *value = var->val_f;
        break;
    case E_VAR_DOUBLE:
        *value = var->val_db;
        break;
    default:
        ret = VAR_FALSE;
    }

    return ret;
}

static void __RefreshRegsFlag(void)
{
    rt_enter_critical();
    {
        memset(g_xExtDataRegsFlag, 0, sizeof(g_xExtDataRegsFlag));
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                if (node->bEnable) {
                    for(int i = 0; i < (node->xIo.btVarSize+1)/2; i++) {
                        int addr = node->xIo.usAddr+i;
                        if( EXT_DATA_REG_CHECK(addr) ) {
                            g_xExtDataRegsFlag[addr-USER_REG_EXT_DATA_START] = VAR_TRUE;
                        }
                    }
                }
                node = node->next;
            }
        }
    }
    rt_exit_critical();
}

var_bool_t bVarManage_CheckExtValue(ExtData_t *data, VarValue_v *var)
{
    var_double_t value = 0;
    if (__GetExtValue(data->xIo.btVarType, var, &value)) {
        if (data->xIo.bUseMax && value > data->xIo.fMax) {
            return VAR_FALSE;
        }
        if (data->xIo.bUseMin && value < data->xIo.fMin) {
            return VAR_FALSE;
        }
    }

    return VAR_TRUE;
}

var_bool_t bVarManage_GetExtValue(ExtData_t *data, var_double_t *value)
{
    var_bool_t ret = VAR_TRUE;
    if (data) {
        return __GetExtValue(data->xIo.btVarType, &data->xIo.xValue, value);
    }

    return ret;
}

void bVarManage_UpdateAvgValue(VarAvg_t *varavg, var_double_t value)
{
    if(varavg) {
        varavg->val_avg += value;
        varavg->val_cur = value;
        varavg->count++;
        // 最大最小值, 初值
        if(1 == varavg->count) {
            varavg->val_min = value;
            varavg->val_max = value;
        } else {
            // 更新最大最小值
            if(value < varavg->val_min) {
                varavg->val_min = value;
            }
            if(value > varavg->val_max) {
                varavg->val_max = value;
            }
        }
    }
}

var_bool_t bVarManage_GetExtValueWithName(const char *name, var_double_t *value)
{
    var_bool_t ret = VAR_TRUE;
    rt_enter_critical();
    {
        ret = bVarManage_GetExtValue(__GetExtDataWithName(name), value);
    }
    rt_exit_critical();
    return ret;
}

static void __update_value(ExtData_t *data)
{
    var_double_t value = 0;
    if (data && bVarManage_GetExtValue(data, &value)) {
        data->xStorage.xAvgReal.val_cur = value;
        data->xStorage.xAvgMin.val_cur = value;
        data->xStorage.xAvgHour.val_cur = value;
        //data->xUp.xAvgUp.val_cur = value;
        data->xStorage.xAvgReal.val_avg += value;
        data->xStorage.xAvgMin.val_avg += value;
        data->xStorage.xAvgHour.val_avg += value;
        //data->xUp.xAvgUp.val_avg += value;
        data->xStorage.xAvgReal.count++;
        data->xStorage.xAvgMin.count++;
        data->xStorage.xAvgHour.count++;
        //data->xUp.xAvgUp.count++;
    }
}

static var_bool_t __doexp_value(ExtData_t *data)
{
    var_bool_t ret = VAR_FALSE;
    double value = 0;
    if (data && data->xIo.szExp && bVarManage_GetExtValue(data, &value)) {
        int is_error = 1;
        double exp_val = evaluate(data->xIo.szExp, data->xName.szName, &is_error);
        if (is_error) {
            rt_kprintf("['%s']->error exp:%s\n", data->xName.szName, data->xIo.szExp );
        } else {
            ret = (exp_val!=value);
            if (ret) {
                switch (data->xIo.btVarType) {
                case E_VAR_BIT:
                    data->xIo.xValue.val_bit = ((exp_val!=0)?1:0);
                    break;
                case E_VAR_INT8:
                    data->xIo.xValue.val_i8 = (var_int8_t)exp_val;
                    break;
                case E_VAR_UINT8:
                    data->xIo.xValue.val_u8 = (var_uint8_t)exp_val;
                    break;
                case E_VAR_INT16:
                    data->xIo.xValue.val_i16 = (var_int16_t)exp_val;
                    break;
                case E_VAR_UINT16:
                    data->xIo.xValue.val_u16 = (var_uint16_t)exp_val;
                    break;
                case E_VAR_INT32:
                    data->xIo.xValue.val_i32 = (var_int32_t)exp_val;
                    break;
                case E_VAR_UINT32:
                    data->xIo.xValue.val_u32 = (var_uint32_t)exp_val;
                    break;
                case E_VAR_FLOAT:
                    data->xIo.xValue.val_f = (var_float_t)exp_val;
                    break;
                case E_VAR_DOUBLE:
                    data->xIo.xValue.val_db = (var_double_t)exp_val;
                    break;
                default:
                    ret = VAR_FALSE;
                }
            }
        }
    }

    return ret;
}

var_bool_t bVarManage_CheckContAddr(var_uint16_t usAddr, var_uint16_t usNRegs)
{
    var_bool_t ret = VAR_TRUE;
    rt_enter_critical();
    while(usNRegs) {
        if( !EXT_DATA_REG_CHECK(usAddr) || !g_xExtDataRegsFlag[usAddr-USER_REG_EXT_DATA_START] ) {
            ret = VAR_FALSE;
            break;
        }
        usNRegs--;
        usAddr++;
    }
    rt_exit_critical();
    return ret;
}

static var_int_t __GetExtDataIoInfoWithAddr(
    var_uint16_t    usAddress, 
    var_uint16_t    *usDevType, 
    var_uint16_t    *usDevNum, 
    var_uint8_t     *btSlaveAddr, 
    var_uint16_t    *usExtAddr, 
    var_uint8_t     *btOpCode
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                usAddress >= node->xIo.usAddr &&
                usAddress < (node->xIo.btVarSize+1)/2 + node->xIo.usAddr) {
                *usDevType = node->xIo.usDevType;
                *usDevNum = node->xIo.usDevNum;
                *btSlaveAddr = node->xIo.param.modbus.btSlaveAddr;
                *usExtAddr = node->xIo.param.modbus.usExtAddr+(usAddress-node->xIo.usAddr);
                *btOpCode = node->xIo.param.modbus.btOpCode;
                nregs = (node->xIo.btVarSize+1)/2-(usAddress-node->xIo.usAddr);
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

extern void vLittleEdianExtData(var_uint8_t btType, var_uint8_t btRule, VarValue_v *in, VarValue_v *out);
extern void vBigEdianExtData(var_uint8_t btType, var_uint8_t btRule, VarValue_v *in, VarValue_v *out);
static void __RefreshExtDataWithRegs(var_uint16_t usAddress, var_uint16_t usNRegs)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                usAddress >= node->xIo.usAddr &&
                usAddress < (node->xIo.btVarSize+1)/2 + node->xIo.usAddr) {
                VarValue_v xValue;
                vLittleEdianExtData( node->xIo.btVarType, node->xIo.btVarRuleType, (VarValue_v *)&g_xExtDataRegs[usAddress-USER_REG_EXT_DATA_START], &xValue );
                if (bVarManage_CheckExtValue(node, &xValue) ) {
                    node->xIo.xValue = xValue;
                    if (__doexp_value(node)) {
                        vBigEdianExtData( node->xIo.btVarType, node->xIo.btVarRuleType, &node->xIo.xValue, (VarValue_v *)&g_xExtDataRegs[usAddress-USER_REG_EXT_DATA_START] );
                    }
                    __update_value( node );
                }
                node->xIo.btErrNum = 0;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

var_bool_t bVarManage_RefreshExtDataWithModbusSlave(var_uint16_t usAddress, var_uint16_t usNRegs)
{
    var_bool_t      ret = VAR_FALSE;
    var_uint16_t    usDevType;
    var_uint16_t    usDevNum;
    var_uint8_t     btSlaveAddr;
    var_uint16_t    usExtAddr;
    var_uint8_t     btOpCode;
    while( usNRegs ) {
        var_int_t nregs = __GetExtDataIoInfoWithAddr(usAddress, &usDevType, &usDevNum, &btSlaveAddr, &usExtAddr, &btOpCode);
        if( nregs > 0 ) {
            var_int8_t _p = -1;
            var_uint16_t *regs = &g_xExtDataRegs[usAddress-USER_REG_EXT_DATA_START];
            if (PROTO_DEV_IS_RS(usDevType)) {
                _p = nUartGetInstance(usDevType);
            } else if (PROTO_DEV_IS_ZIGBEE(usDevType)) {
                extern rt_bool_t g_zigbee_init;
                if (g_zigbee_init) {
                    if (zgb_set_dst_node(ZGB_SN_T_MODBUS_RTU, btSlaveAddr)) {
                        _p = BOARD_ZIGBEE_UART;
                    }
                }
            } else if (PROTO_DEV_IS_NET(usDevType)) {
                _p = usDevNum;
            } else if (PROTO_DEV_IS_GPRS(usDevType)) {
                _p = usDevNum + BOARD_ENET_TCPIP_NUM;
            }
            
            if(_p >= 0) {
                switch(btOpCode) {
                case MODBUS_FC_READ_HOLDING_REGISTERS:
                    modbus_write_registers_with(usDevType, _p, btSlaveAddr, usExtAddr, usNRegs, (const uint16_t *)regs);
                    break;
                }
            }
            __RefreshExtDataWithRegs(usAddress,nregs);
            usAddress+=nregs;
            usNRegs = ((usNRegs>nregs)?(usNRegs-nregs):0);
            ret = VAR_TRUE;
        } else {
            // skip one reg
            usAddress++;
            usNRegs--;
        }
    }
    return ret;
}

static var_int_t __GetExtDataIoInfoWithSlaveAddr(
    var_uint16_t    usDevType,      // 关联硬件
    var_uint16_t    usDevNum,       // 关联硬件编号
    var_uint8_t     btSlaveAddr, 
    var_uint16_t    usAddress, 
    var_uint16_t    *usAddr
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usDevType == usDevType &&
                node->xIo.usDevNum == usDevNum &&
                node->xIo.param.modbus.btSlaveAddr == btSlaveAddr &&
                usAddress >= node->xIo.param.modbus.usExtAddr &&
                usAddress < (node->xIo.btVarSize+1)/2 + node->xIo.param.modbus.usExtAddr) {
                *usAddr = node->xIo.usAddr + (usAddress-node->xIo.param.modbus.usExtAddr);
                nregs = (node->xIo.btVarSize+1)/2-(usAddress-node->xIo.param.modbus.usExtAddr);
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

void __refresh_regs(var_uint8_t *regbuf, var_uint16_t usAddr, int nregs)
{
    var_uint16_t *regs = &g_xExtDataRegs[usAddr-USER_REG_EXT_DATA_START];
    int iRegIndex = 0;
    while (nregs > 0) {
        regs[iRegIndex] = *regbuf++;
        regs[iRegIndex] |= *regbuf++ << 8;
        iRegIndex++;
        nregs--;
    }
}

void __clear_regs(var_uint16_t usAddr, int nregs)
{
    //var_uint16_t *regs = &g_xExtDataRegs[usAddr-USER_REG_EXT_DATA_START];
    memset(&g_xExtDataRegs[usAddr-USER_REG_EXT_DATA_START], 0, 2*nregs);
}

static void vVarManage_ModbusMasterTryClearExtDataWithErr(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum,
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs
)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            int nregs = (node->xIo.btVarSize+1)/2;
            if (node->bEnable &&
                node->xIo.usDevType == usDevType &&
                node->xIo.usDevNum == usDevNum &&
                node->xIo.param.modbus.btSlaveAddr == btSlaveAddr &&
                node->xIo.param.modbus.usExtAddr >= usAddress &&
                nregs + node->xIo.param.modbus.usExtAddr <= usAddress + usNRegs) {
                if(E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, nregs);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

void __read_regs(var_uint8_t *regbuf, var_uint16_t usAddr, int nregs)
{
    var_uint16_t *regs = &g_xExtDataRegs[usAddr-USER_REG_EXT_DATA_START];
    int iRegIndex = 0;
    while (nregs > 0) {
        *regbuf++ = (UCHAR)(regs[iRegIndex] & 0xFF);
        *regbuf++ = (UCHAR)(regs[iRegIndex] >> 8 & 0xFF);
        iRegIndex++;
        nregs--;
    }
}

var_bool_t bVarManage_RefreshExtDataWithModbusMaster(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum, 
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs, 
    var_uint8_t *pucRegBuffer
)
{
    var_bool_t ret = VAR_FALSE;
    {
        rt_enter_critical();
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                int nregs = (node->xIo.btVarSize+1)/2;
                if (node->bEnable &&
                    node->xIo.usDevType == usDevType &&
                    node->xIo.usDevNum == usDevNum &&
                    node->xIo.param.modbus.btSlaveAddr == btSlaveAddr &&
                    node->xIo.param.modbus.usExtAddr >= usAddress &&
                    nregs + node->xIo.param.modbus.usExtAddr <= usAddress + usNRegs) {
                    int ofs = node->xIo.param.modbus.usExtAddr - usAddress;
                    __refresh_regs(pucRegBuffer + (ofs << 1), node->xIo.usAddr, nregs);
                    __RefreshExtDataWithRegs(node->xIo.usAddr, nregs);
                }
                node = node->next;
            }
        }
        rt_exit_critical();
        ret = VAR_TRUE;
    }

    return ret;
}

var_bool_t bVarManage_ReadExtDataWithModbusMaster(
    var_uint16_t usDevType, 
    var_uint16_t usDevNum, 
    var_uint8_t btSlaveAddr, 
    var_uint16_t usAddress, 
    var_uint16_t usNRegs, 
    var_uint8_t *pucRegBuffer 
)
{
    var_bool_t ret = VAR_TRUE;
    {
        rt_enter_critical();
        if (s_xExtDataList.pList) {
            ExtData_t *node = s_xExtDataList.pList;
            while (node) {
                int nregs = (node->xIo.btVarSize+1)/2;
                if (node->bEnable &&
                    node->xIo.usDevType == usDevType &&
                    node->xIo.usDevNum == usDevNum &&
                    node->xIo.param.modbus.btSlaveAddr == btSlaveAddr &&
                    node->xIo.param.modbus.usExtAddr >= usAddress &&
                    nregs + node->xIo.param.modbus.usExtAddr <= usAddress + usNRegs) {
                    int ofs = node->xIo.param.modbus.usExtAddr - usAddress;
                    __read_regs(pucRegBuffer + (ofs << 1), node->xIo.usAddr, nregs);
                }
                node = node->next;
            }
        }
        rt_exit_critical();
    }
    
    return ret;
}

static var_int_t __GetExtDataIoInfoWithDlt645Addr(
    dlt645_addr_t   *dlt645_addr, 
    var_uint16_t    addr, 
    var_uint32_t    op, 
    var_int8_t      *var_type
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                0 == memcmp(&node->xIo.param.dlt645.addr, dlt645_addr, sizeof(dlt645_addr_t)) &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dlt645.op == op ) {
                *var_type = node->xIo.btVarType;
                nregs = (node->xIo.btVarSize+1)/2;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

//var_uint8_t btSlaveAddr, var_uint16_t usAddress, var_uint16_t usNRegs, var_uint8_t *pucRegBuffer
var_bool_t bVarManage_RefreshExtDataWithDlt645(dlt645_addr_t *dlt645_addr, var_uint16_t addr, mdUINT32 op, float val)
{
    var_bool_t ret = VAR_FALSE;
    var_int8_t var_type = -1;
    var_int_t nregs = __GetExtDataIoInfoWithDlt645Addr(dlt645_addr, addr, op, &var_type);
    if( nregs > 0 ) {
        var_uint8_t f_buf[8] = {0};
        long intval = (long)val;
        if (var_type <= E_VAR_BIT) {
            f_buf[0] = intval?1:0;
        } else if (var_type <= E_VAR_UINT8) {
            f_buf[0] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT16) {
            f_buf[0] = (intval>>8)&0xFF;
            f_buf[1] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT32) {
            f_buf[0] = (intval>>24)&0xFF;
            f_buf[1] = (intval>>16)&0xFF;
            f_buf[2] = (intval>>8)&0xFF;
            f_buf[3] = intval&0xFF;
        } else if (var_type <= E_VAR_FLOAT) {
            var_uint8_t tmp;
            memcpy(f_buf, &val, 4);
            tmp = f_buf[0]; f_buf[0] = f_buf[3]; f_buf[3] = tmp;
            tmp = f_buf[1]; f_buf[1] = f_buf[2]; f_buf[2] = tmp;
        } else if (var_type == E_VAR_DOUBLE) {
            var_uint8_t tmp;
            double dbval = (double)val;
            memcpy(f_buf, &dbval, 8);
            tmp = f_buf[0]; f_buf[0] = f_buf[7]; f_buf[7] = tmp;
            tmp = f_buf[1]; f_buf[1] = f_buf[6]; f_buf[6] = tmp;
            tmp = f_buf[2]; f_buf[2] = f_buf[5]; f_buf[5] = tmp;
            tmp = f_buf[3]; f_buf[3] = f_buf[4]; f_buf[4] = tmp;
        }
        rt_enter_critical();
        {
            __refresh_regs(f_buf, addr, nregs>4?4:nregs);
        }
        rt_exit_critical();
        __RefreshExtDataWithRegs(addr,nregs);
    }

    return ret;
}

static void vVarManage_Dlt645TryClearExtDataWithErr(dlt645_addr_t *dlt645_addr, var_uint16_t addr, var_uint32_t op)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                0 == memcmp(&node->xIo.param.dlt645.addr, dlt645_addr, sizeof(dlt645_addr_t)) &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dlt645.op == op ) {
                if(E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, (node->xIo.btVarSize+1)/2);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

static var_int_t __GetExtDataIoInfoWithDust(
    var_uint16_t    addr, 
    var_uint8_t     op, 
    var_int8_t      *var_type
)
{
    var_int_t nregs = 0;
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dust.op == op ) {
                *var_type = node->xIo.btVarType;
                nregs = (node->xIo.btVarSize+1)/2;
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
    return nregs;
}

var_bool_t bVarManage_RefreshExtDataWithDust(var_uint16_t addr, var_uint8_t op, Dust_ResultData_t *result)
{
    var_bool_t ret = VAR_FALSE;
    var_int8_t var_type = -1;
    var_int_t nregs = __GetExtDataIoInfoWithDust(addr, op, &var_type);
    if( nregs > 0 ) {
        var_uint8_t f_buf[8] = {0};
        long intval = 0;
        switch(op) {
        case DUST_OP_PM25: intval = result->PM25; break;
        case DUST_OP_PM10: intval = result->PM10; break;
        case DUST_OP_PM25_SUM: intval = result->PM25Sum; break;
        case DUST_OP_PM10_SUM: intval = result->PM10Sum; break;
        case DUST_OP_PM25_NUM: intval = result->PM25Num; break;
        case DUST_OP_PM10_NUM: intval = result->PM10Num; break;
        case DUST_OP_T: intval = result->T; break;
        case DUST_OP_H: intval = result->H; break;
        default: ret = VAR_FALSE; goto end;
        }
        if (var_type <= E_VAR_BIT) {
            f_buf[0] = intval?1:0;
        } else if (var_type <= E_VAR_UINT8) {
            f_buf[0] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT16) {
            f_buf[0] = (intval>>8)&0xFF;
            f_buf[1] = intval&0xFF;
        } else if (var_type <= E_VAR_UINT32) {
            f_buf[0] = (intval>>24)&0xFF;
            f_buf[1] = (intval>>16)&0xFF;
            f_buf[2] = (intval>>8)&0xFF;
            f_buf[3] = intval&0xFF;
        }
        rt_enter_critical();
        {
            __refresh_regs(f_buf, addr, nregs>2?2:nregs);
        }
        rt_exit_critical();
        __RefreshExtDataWithRegs(addr,nregs);
    }
end:
    return ret;
}

static void vVarManage_DustTryClearExtDataWithErr(var_uint16_t addr, var_uint8_t op)
{
    rt_enter_critical();
    if (s_xExtDataList.pList) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (node->bEnable &&
                node->xIo.usAddr == addr &&
                node->xIo.param.dust.op == op ) {
                if(E_ERR_OP_CLEAR == node->xIo.btErrOp) {
                    if(node->xIo.btErrNum >= node->xIo.btErrCnt) {
                        __clear_regs(node->xIo.usAddr, (node->xIo.btVarSize+1)/2);
                        memset(&node->xIo.xValue, 0, sizeof(node->xIo.xValue));
                        node->xIo.btErrNum = 0;
                    }
                    node->xIo.btErrNum++;
                }
                break;
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}


static var_bool_t prvCheckExeDataEnable(ExtData_t *pData)
{
    return (pData->bEnable &&
            pData->xIo.btVarType <= E_VAR_ARRAY &&
            pData->xIo.usAddr >= USER_REG_EXT_DATA_START && pData->xIo.usAddr <= USER_REG_EXT_DATA_END &&
            PROTO_DEV_CHECK(pData->xIo.usDevType));
}

void vVarManage_RefreshExtDataStorageAvg(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            data->xStorage.xAvgReal = node->xStorage.xAvgReal;
            data->xStorage.xAvgMin = node->xStorage.xAvgMin;
            data->xStorage.xAvgHour = node->xStorage.xAvgHour;
        }
    }
    rt_exit_critical();
}

void vVarManage_ClearExtDataStorageAvgReal(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            node->xStorage.xAvgReal.val_avg = 0;
            node->xStorage.xAvgReal.count = 0;
            node->xStorage.xAvgReal.time = rt_tick_get();
        }
    }
    rt_exit_critical();
}

void vVarManage_ClearExtDataStorageAvgMin(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            node->xStorage.xAvgMin.val_avg = 0;
            node->xStorage.xAvgMin.count = 0;
            node->xStorage.xAvgMin.time = rt_tick_get();
        }
    }
    rt_exit_critical();
}

void vVarManage_ClearExtDataStorageAvgHour(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            node->xStorage.xAvgHour.val_avg = 0;
            node->xStorage.xAvgHour.count = 0;
            node->xStorage.xAvgHour.time = rt_tick_get();
        }
    }
    rt_exit_critical();
}

static void vVarManage_RefreshExtDataUpAvg(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            data->xUp.xAvgUp = node->xUp.xAvgUp;
        }
    }
    rt_exit_critical();
}

static void vVarManage_ClearExtDataUp(ExtData_t *data)
{
    rt_enter_critical();
    {
        ExtData_t *node = __GetExtDataWithName(data->xName.szName);
        if( node ) {
            node->xUp.xAvgUp.val_avg = 0;
            node->xUp.xAvgUp.count = 0;
            node->xUp.xAvgUp.time = rt_tick_get();
        }
    }
    rt_exit_critical();
}

// add by jay 2016/11/25
// 不安全函数, 必须在 rt_enter_critical rt_exit_critical 中调用
ExtData_t* pVarManage_GetExtDataWithUpProto(
    ExtData_t       *first,
    var_uint16_t    usDevType,
    var_uint16_t    usDevNum,
    var_uint16_t    usProtoType
    )
{
    ExtData_t *ret = VAR_NULL;
    if (s_xExtDataList.pList) {
        ExtData_t *node = (first ? first->next : s_xExtDataList.pList);
        while (node) {
            if (node->bEnable && node->xUp.bEnable &&
                usDevType == node->xUp.usDevType &&
                usDevNum == node->xUp.usDevNum &&
                usProtoType == node->xUp.usProtoType) {
                ret = node;
                break;
            }
            node = node->next;
        }
    }
    return ret;
}

rt_bool_t cfg_recover_busy(void);

void vVarManage_ExtDataRefreshTask(void *parameter)
{
    rt_thread_delay(5*RT_TICK_PER_SECOND);
    int index = (int)(long)parameter;
    while (1) {
        sync_slave_t *slave = VAR_NULL;
        sync_ext_t *ext = VAR_NULL;
        var_bool_t com_flag[BOARD_UART_MAX] = {0};
        for(int i = PROTO_DEV_RS1; i <= PROTO_DEV_RS_MAX; i++ ) {
            com_flag[i] = VAR_TRUE;
        }
        while (!cfg_recover_busy()) {
            rt_thddog_feed("");
            sync_node_t sync_node;
            rt_tick_t now_tick = rt_tick_get();
            rt_tick_t diff_tick = 0;
            if (__get_next_sync_ext_node(&slave, &ext, &sync_node)) {
                if (s_varmanage_run[index].dev_type != sync_node.dev_type || 
                    s_varmanage_run[index].dev_num != sync_node.dev_num) {
                    continue;
                }
                if (__proto_is_rtu_self(sync_node.dev_type, sync_node.dev_num)) {
                    if (sync_node.dev_num < 8) {
                        // AI 1个通道占用两个寄存器
                        rt_enter_critical();
                        {
                            var_uint16_t reg = g_xAIResultReg.regs[sync_node.dev_num*2];
                            g_xExtDataRegs[sync_node.param.self.addr-USER_REG_EXT_DATA_START+1] = (reg << 8) | (reg >> 8);
                            reg = g_xAIResultReg.regs[sync_node.dev_num*2+1];
                            g_xExtDataRegs[sync_node.param.self.addr-USER_REG_EXT_DATA_START] = (reg << 8) | (reg >> 8);
                        }
                        rt_exit_critical();
                        rt_thddog_suspend("__proto_is_rtu_self");
                        __RefreshExtDataWithRegs(sync_node.param.self.addr, 2);
                        rt_thddog_resume();
                    }
                } else if (__proto_is_rtu_self_mid(sync_node.dev_type)) {
                    //中间变量1个占用四个寄存器
                    rt_thddog_suspend("__proto_is_rtu_self_mid");
                    __RefreshExtDataWithRegs(sync_node.param.self.addr, 4);
                    rt_thddog_resume();
                } else if (__proto_is_modbus(sync_node.dev_type, sync_node.proto_type)) {
                    var_int8_t _p = -1;
                    var_uint8_t _sa = sync_node.param.modbus.slave_addr;
                    var_uint16_t _ea = sync_node.param.modbus.ext_addr;
                    var_uint8_t _es = sync_node.param.modbus.ext_size;
                    var_bool_t _flag = VAR_TRUE;
                    if (PROTO_DEV_IS_RS(sync_node.dev_type)) {
                        rt_thddog_suspend("__proto_is_modbus com");
                        _p = nUartGetInstance(sync_node.dev_type);
                        diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                        _flag = var_check_diff(now_tick, s_com_last_tick[_p], diff_tick);
                        if(com_flag[_p]) com_flag[_p] = _flag;
                    } else if (PROTO_DEV_IS_ZIGBEE(sync_node.dev_type)) {
                        extern rt_bool_t g_zigbee_init;
                        if (g_zigbee_init) {
                            if (zgb_set_dst_node(g_zigbee_cfg.nProtoType, sync_node.param.modbus.slave_addr)) {
                                rt_thddog_suspend("__proto_is_modbus zigbee");
                                _p = BOARD_ZIGBEE_UART;
                            }
                        }
                    } else if (PROTO_DEV_IS_NET(sync_node.dev_type)) { 
                        rt_thddog_suspend("__proto_is_modbus net");
                        _p = sync_node.dev_num;
                    } else if (PROTO_DEV_IS_GPRS(sync_node.dev_type)) { 
                        rt_thddog_suspend("__proto_is_modbus gprs");
                        _p = sync_node.dev_num + BOARD_ENET_TCPIP_NUM;
                    }

                    if(_p >= 0 && _flag) {
                        int rc = 0;
                        switch(sync_node.param.modbus.op) {
                        case MODBUS_FC_READ_HOLDING_REGISTERS:
                            rc = modbus_read_registers_with(sync_node.dev_type, _p, sync_node.dev_num, _sa, _ea, _es);
                            break;
                        case MODBUS_FC_READ_INPUT_REGISTERS:
                            rc = modbus_read_input_registers_with(sync_node.dev_type, _p, sync_node.dev_num, _sa, _ea, _es);
                            break;
                        }
                        if(rc < 0) {
                            vVarManage_ModbusMasterTryClearExtDataWithErr(sync_node.dev_type, sync_node.dev_num, _sa, _ea, _es);
                        }
                        rt_thread_delay(20);
                    }
                    rt_thddog_resume();
                } else if (__proto_is_dlt645_2007(sync_node.dev_type, sync_node.proto_type)) {
                    S_DLT645_A_t dlt645_addr;
                    S_DLT645_Result_t dlt645_result;
                    rt_tick_t start_tick = rt_tick_get();
                    memcpy(&dlt645_addr, &sync_node.param.dlt645.dlt645_addr, sizeof(S_DLT645_A_t));
                    rt_thddog_suspend("__proto_is_dlt645_2007->bDlt645ReadData");
                    if( bDlt645ReadData(sync_node.dev_type, dlt645_addr, sync_node.param.dlt645.op, 1000, &dlt645_result) ) {
                        rt_thddog_suspend("__proto_is_dlt645_2007->RefreshExtDataWithDlt645");
                        bVarManage_RefreshExtDataWithDlt645(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, dlt645_result.op, dlt645_result.val);
                    } else {
                        vVarManage_Dlt645TryClearExtDataWithErr(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, sync_node.param.dlt645.op);
                    }
                    rt_thddog_resume();
                    int remain_tick = rt_tick_from_millisecond(500) - (rt_tick_get() - start_tick);
                    if (remain_tick > 0) rt_thread_delay(remain_tick);
                } else if (__proto_is_dlt645_1997(sync_node.dev_type, sync_node.proto_type)) {
                    S_DLT645_1997_A_t dlt645_addr;
                    S_DLT645_1997_Result_t dlt645_result;
                    rt_tick_t start_tick = rt_tick_get();
                    memcpy(&dlt645_addr, &sync_node.param.dlt645.dlt645_addr, sizeof(S_DLT645_1997_A_t));
                    rt_thddog_suspend("__proto_is_dlt645_1997->bDlt645ReadData");
                    if( bDlt645_1997ReadData(sync_node.dev_type, dlt645_addr, sync_node.param.dlt645.op, 1000, &dlt645_result) ) {
                        rt_thddog_suspend("__proto_is_dlt645_1997->RefreshExtDataWithDlt645");
                        bVarManage_RefreshExtDataWithDlt645(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, dlt645_result.op, dlt645_result.val);
                    } else {
                        vVarManage_Dlt645TryClearExtDataWithErr(&sync_node.param.dlt645.dlt645_addr, sync_node.param.dlt645.addr, sync_node.param.dlt645.op);
                    }
                    rt_thddog_resume();
                    int remain_tick = rt_tick_from_millisecond(500) - (rt_tick_get() - start_tick);
                    if (remain_tick > 0) rt_thread_delay(remain_tick);
                } else if (__proto_is_dust(sync_node.dev_type, sync_node.proto_type)) {
                    var_int8_t _p = -1;
                    var_bool_t _flag = VAR_TRUE;
                    Dust_ResultData_t dust_data;

                    _p = nUartGetInstance(sync_node.dev_type);
                    diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                    _flag = var_check_diff(now_tick, s_com_last_tick[_p], diff_tick);
                    if(com_flag[_p]) com_flag[_p] = _flag;

                    if(_flag) {
                        bDustStartSample(sync_node.dev_type, 1000);
                    }
                    
                    rt_thddog_suspend("__proto_is_dust->bDust_ReadData");
                    if (bDust_ReadData(sync_node.dev_type, 0, &dust_data)) {
                        rt_thddog_suspend("__proto_is_dust->RefreshExtDataWithDlt645");
                        bVarManage_RefreshExtDataWithDust(sync_node.param.dust.addr, sync_node.param.dust.op, &dust_data);
                    } else {
                        //vVarManage_DustTryClearExtDataWithErr(sync_node.param.dust.addr, sync_node.param.dust.op);
                    }
                    rt_thddog_resume();
                }
            } else {
                if (s_varmanage_run[index].dev_type <= PROTO_DEV_RS_MAX) {
                    rt_thread_delay(100);
                } else if (s_varmanage_run[index].dev_type <= PROTO_DEV_NET) {
                    rt_thread_delay(100);
                } else if (s_varmanage_run[index].dev_type <= PROTO_DEV_ZIGBEE) {
                    rt_thread_delay(500);
                } else if (s_varmanage_run[index].dev_type <= PROTO_DEV_GPRS) {
                    rt_thread_delay(500);
                }
                break;
            }
        }

        if(!cfg_recover_busy()) {
            for(int _type = PROTO_DEV_RS1; _type <= PROTO_DEV_RS_MAX; _type++ ) {
                int _p = nUartGetInstance(_type);
                rt_tick_t now_tick = rt_tick_get();
                rt_tick_t diff_tick = rt_tick_from_millisecond(g_uart_cfgs[_p].interval);
                if (s_varmanage_run[index].dev_type != _type) {
                    continue;
                }
                if(var_check_diff(now_tick, s_com_last_tick[_p], diff_tick) && com_flag[_p]) {
                    s_com_last_tick[_p] = now_tick;
                }
            }

            {
                ExtData_t *node = s_xExtDataList.pList;
                int num = s_xExtDataList.n;
                for (int n = 0; n < num; n++) {
                    node = __GetExtDataWithIndex(n);
                    if (!node) break;
                    if (s_varmanage_run[index].dev_type == node->xIo.usDevType &&
                        s_varmanage_run[index].dev_num == node->xIo.usDevNum && 
                        prvCheckExeDataEnable(node)) {
                        //vVarManage_RefreshExtDataStorageAvg(node);
                        if (g_storage_cfg.bEnable && node->xStorage.bEnable) {
                            // real
                            if (rt_tick_get() - node->xStorage.xAvgReal.time >= rt_tick_from_millisecond(node->xStorage.ulStep * 60 * 1000)) {
                                if (node->xStorage.xAvgReal.count > 0) {
                                    var_double_t value = (node->xStorage.xAvgReal.val_avg / node->xStorage.xAvgReal.count);
                                    rt_thddog_suspend("bStorageAddData real");
                                    bStorageAddData(ST_T_RT_DATA, node->xName.szName, value, node->xAlias.szAlias);
                                    rt_thddog_resume();
                                    vVarManage_ClearExtDataStorageAvgReal(node);
                                }
                            }
                            //min
                            if (rt_tick_get() - node->xStorage.xAvgMin.time >= rt_tick_from_millisecond(60 * 1000)) {
                                if (node->xStorage.xAvgMin.count > 0) {
                                    var_double_t value = (node->xStorage.xAvgMin.val_avg / node->xStorage.xAvgMin.count);
                                    rt_thddog_suspend("bStorageAddData min");
                                    bStorageAddData(ST_T_MINUTE_DATA, node->xName.szName, value, node->xAlias.szAlias);
                                    rt_thddog_resume();
                                    vVarManage_ClearExtDataStorageAvgMin(node);
                                }
                            }
                            //hour
                            if (rt_tick_get() - node->xStorage.xAvgHour.time >= rt_tick_from_millisecond(60 * 60 * 1000)) {
                                if (node->xStorage.xAvgHour.count > 0) {
                                    var_double_t value = (node->xStorage.xAvgHour.val_avg / node->xStorage.xAvgHour.count);
                                    rt_thddog_suspend("bStorageAddData hour");
                                    bStorageAddData(ST_T_HOURLY_DATA, node->xName.szName, value, node->xAlias.szAlias);
                                    rt_thddog_resume();
                                    vVarManage_ClearExtDataStorageAvgHour(node);
                                }
                            }
                        }
                    }
                    vVarManage_FreeData(node);
                }
            }
        }
        rt_thddog_feed("");
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
    }
    rt_thddog_exit();
}

static int __varmanage_run_get(    var_uint16_t     dev_type, var_uint16_t dev_num)
{
    int n;
    for (n = 0; n < BOARD_COLL_MAX; n++) {
        if (!s_varmanage_run[n].use) {
            return n;
        } else if (s_varmanage_run[n].use && 
            s_varmanage_run[n].dev_type == dev_type && 
            s_varmanage_run[n].dev_num == dev_num) {
            return n;
        }
    }
    return -1;
}

void varmanage_update(void)
{
    rt_enter_critical();
    {
        rt_bool_t first = RT_TRUE;
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            int index = __varmanage_run_get(node->xIo.usDevType, node->xIo.usDevNum);
            if (index >= 0 && !s_varmanage_run[index].use) {
                s_varmanage_run[index].dev_type = node->xIo.usDevType;
                s_varmanage_run[index].dev_num = node->xIo.usDevNum;
                BOARD_CREAT_NAME(run_name, "col_refresh[%d:%d]", node->xIo.usDevType, node->xIo.usDevNum);
                s_varmanage_run[index].thread = \
                    rt_thread_create(run_name, vVarManage_ExtDataRefreshTask, (void *)(long)index, 0, 0, 0);
                if (s_varmanage_run[index].thread) {
                    s_varmanage_run[index].use = 1;
                    rt_thddog_register(s_varmanage_run[index].thread, 60);
                    rt_thread_startup(s_varmanage_run[index].thread);
                }
            }
            node = node->next;
        }
    }
    rt_exit_critical();
}

void varmanage_start(void)
{
    varmanage_update();
}

/*
void *vVarManage_ExtDataUpTask(void *parameter)
{
    while (1) {
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            if (prvCheckExeDataEnable(node)) {
                if (node->xUp.bEnable) {
                    var_int32_t up_interval = ulVarManage_GetExtDataUpInterval(node);
                    //up
                    if (up_interval > 0 && rt_tick_get() - node->xUp.xAvgUp.time >= rt_tick_from_millisecond(up_interval * 1000)) {
                        if (node->xUp.xAvgUp.count > 0) {
                            var_double_t value = (node->xUp.xAvgUp.val_avg / node->xUp.xAvgUp.count);
                            node->xUp.xAvgUp.val_avg = 0;
                            node->xUp.xAvgUp.count = 0;
                            node->xUp.xAvgUp.time = rt_tick_get();
                            //bStorageAddData(ST_T_HOURLY_DATA, node->xName.szName, value, node->xAlias.szAlias);

                        }
                    }
                }
            }
            node = node->next;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
    }
}
*/

// S 为单位
var_int32_t lVarManage_GetExtDataUpInterval( var_uint16_t usDevType, var_uint16_t usDevNum )
{
    switch (usDevType) {
    case PROTO_DEV_RS1:
        return g_uart_cfgs[0].interval;
    case PROTO_DEV_RS2:
        return g_uart_cfgs[1].interval;
    case PROTO_DEV_RS3:
        return g_uart_cfgs[2].interval;
    case PROTO_DEV_RS4:
        return g_uart_cfgs[3].interval;
    case PROTO_DEV_NET:
    case PROTO_DEV_GPRS:
        return g_tcpip_cfgs[usDevNum].interval;
    case PROTO_DEV_ZIGBEE:
        return -1;
    case PROTO_DEV_LORA:
        return -1;
    }
    return -1;
}

extern void prvSaveUartCfgsToFs(void);
extern void prvSaveTcpipCfgsToFs(void);

void vVarManage_SetExtDataUpInterval( var_uint16_t usDevType, var_uint16_t usDevNum, var_uint32_t interval )
{
    switch (usDevType) {
    case PROTO_DEV_RS1:
    case PROTO_DEV_RS2:
    case PROTO_DEV_RS3:
    case PROTO_DEV_RS4:
        g_uart_cfgs[usDevType].interval = interval;
        prvSaveUartCfgsToFs();
        break;
    case PROTO_DEV_NET:
        g_tcpip_cfgs[usDevNum].interval = interval;
        prvSaveTcpipCfgsToFs();
        break;
    case PROTO_DEV_ZIGBEE:
        break;
    case PROTO_DEV_GPRS:
        g_tcpip_cfgs[BOARD_ENET_TCPIP_NUM + usDevNum].interval = interval;
        prvSaveTcpipCfgsToFs();
        break;
    case PROTO_DEV_LORA:
        break;
    }
}

var_bool_t jsonParseExtDataInfo(cJSON *pItem, ExtData_t *data)
{
    if ( pItem && data) {
        cJSON *pIO = cJSON_GetObjectItem(pItem,"io");
        cJSON *pAlarm = cJSON_GetObjectItem(pItem,"alarm");
        cJSON *pStorage = cJSON_GetObjectItem(pItem,"storage");
        cJSON *pUp = cJSON_GetObjectItem(pItem,"up");
        int itmp;
        //double dtmp;
        const char *str;

        itmp = cJSON_GetInt(pItem, "en", -1);
        if (itmp >= 0) data->bEnable = (itmp != 0 ? VAR_TRUE : VAR_FALSE);
        str = cJSON_GetString(pItem, "na", VAR_NULL);
        if (str) {
            memset(&data->xName, 0, sizeof(VarName_t)); strncpy(data->xName.szName, str, sizeof(VarName_t));
        }
        str = cJSON_GetString(pItem, "al", VAR_NULL);
        if (str) {
            memset(&data->xAlias, 0, sizeof(VarAlias_t)); strncpy(data->xAlias.szAlias, str, sizeof(VarAlias_t));
        }
        if (pIO) {
            data->xIo.btErrOp = cJSON_GetInt(pIO, "eop", data->xIo.btErrOp);
            data->xIo.btErrCnt = cJSON_GetInt(pIO, "ecnt", data->xIo.btErrCnt);
            data->xIo.btRWType = cJSON_GetInt(pIO, "rw", data->xIo.btRWType);
            data->xIo.btVarType = cJSON_GetInt(pIO, "vt", data->xIo.btVarType);
            data->xIo.btVarSize = cJSON_GetInt(pIO, "vs", data->xIo.btVarSize);
            data->xIo.btVarRuleType = cJSON_GetInt(pIO, "vrl", data->xIo.btVarRuleType);
            if(data->xIo.btVarRuleType >= 4) data->xIo.btVarRuleType = 0;
            data->xIo.bUseMax = cJSON_GetObjectItem(pIO, "vma") ? RT_TRUE : RT_FALSE;
            data->xIo.bUseMin = cJSON_GetObjectItem(pIO, "vmi") ? RT_TRUE : RT_FALSE;
            data->xIo.bUseInit = cJSON_GetObjectItem(pIO, "vii") ? RT_TRUE : RT_FALSE;
            data->xIo.bUseRatio = cJSON_GetObjectItem(pIO, "vrt") ? RT_TRUE : RT_FALSE;
            if (data->xIo.bUseMax) data->xIo.fMax = (var_float_t)cJSON_GetDouble(pIO, "vma", data->xIo.fMax);
            if (data->xIo.bUseMin) data->xIo.fMin = (var_float_t)cJSON_GetDouble(pIO, "vmi", data->xIo.fMin);
            if (data->xIo.bUseInit) data->xIo.fInit = (var_float_t)cJSON_GetDouble(pIO, "vii", data->xIo.fInit);
            if (data->xIo.bUseRatio) data->xIo.fRatio = (var_float_t)cJSON_GetDouble(pIO, "vrt", data->xIo.fRatio);

            data->xIo.usDevType = cJSON_GetInt(pIO, "dt", data->xIo.usDevType);
            data->xIo.usDevNum = cJSON_GetInt(pIO, "dtn", data->xIo.usDevNum);
            data->xIo.usProtoType = cJSON_GetInt(pIO, "pt", data->xIo.usProtoType);
            str = cJSON_GetString(pIO, "pnm", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xIo.szProtoName);
                if (strlen(str) > 0) data->xIo.szProtoName = rt_strdup(str);
            }

            data->xIo.usAddr = cJSON_GetInt(pIO, "ad", data->xIo.usAddr);

            if (__proto_is_modbus(data->xIo.usDevType, data->xIo.usProtoType)) {
                data->xIo.param.modbus.btOpCode = cJSON_GetInt(pIO, "mbop", data->xIo.param.modbus.btOpCode);
                if(0 == data->xIo.param.modbus.btOpCode) data->xIo.param.modbus.btOpCode = 3;
                data->xIo.param.modbus.nSyncFAddr = cJSON_GetInt(pIO, "sfa", data->xIo.param.modbus.nSyncFAddr);
                data->xIo.param.modbus.btSlaveAddr = cJSON_GetInt(pIO, "sa", data->xIo.param.modbus.btSlaveAddr);
                data->xIo.param.modbus.usExtAddr = cJSON_GetInt(pIO, "ea", data->xIo.param.modbus.usExtAddr);
                data->xIo.param.modbus.usAddrOfs = cJSON_GetInt(pIO, "ao", data->xIo.param.modbus.usAddrOfs);
            } else if (__proto_is_dlt645(data->xIo.usDevType, data->xIo.usProtoType)) {
                const char *addr_str = cJSON_GetString(pIO, "dltad", VAR_NULL);
                if (addr_str && strlen(addr_str)==(2*sizeof(dlt645_addr_t))) {
                    for( int i = 0; i < sizeof(dlt645_addr_t); i++ ) {
                		data->xIo.param.dlt645.addr.addr[5-i] = (HEX_CH_TO_NUM(addr_str[2*i]) << 4) + HEX_CH_TO_NUM(addr_str[2*i+1]);
                	}
                }
                data->xIo.param.dlt645.op = cJSON_GetInt(pIO, "dltop", data->xIo.param.dlt645.op);
            } else if (__proto_is_dust(data->xIo.usDevType, data->xIo.usProtoType)) {
                data->xIo.param.dust.op = cJSON_GetInt(pIO, "dustop", data->xIo.param.dust.op);
            }
            
            str = cJSON_GetString(pIO, "exp", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xIo.szExp);
                if (strlen(str) > 0) data->xIo.szExp = rt_strdup(str);
            }
        }

        if (pStorage) {
            itmp = cJSON_GetInt(pStorage, "se", -1);
            if (itmp >= 0) data->xStorage.bEnable = (itmp != 0 ? VAR_TRUE : VAR_FALSE);
            data->xStorage.ulStep = cJSON_GetInt(pStorage, "ss", data->xStorage.ulStep);
            data->xStorage.btType = cJSON_GetInt(pStorage, "st", data->xStorage.btType);
            itmp = cJSON_GetInt(pStorage, "sc", -1);
            if (itmp >= 0) data->xStorage.bCheck = (itmp != 0 ? VAR_TRUE : VAR_FALSE);
            data->xStorage.fMax = cJSON_GetDouble(pStorage, "sma", data->xStorage.fMax);
            data->xStorage.fMin = cJSON_GetDouble(pStorage, "smi", data->xStorage.fMin);
        }

        if (pAlarm) {
            itmp = cJSON_GetInt(pAlarm, "en", -1);
            if (itmp >= 0) data->xAlarm.bEnable = (itmp != 0 ? VAR_TRUE : VAR_FALSE);
        }

        if (pUp) {
            itmp = cJSON_GetInt(pUp, "en", -1);
            if (itmp >= 0) data->xUp.bEnable = (itmp != 0 ? VAR_TRUE : VAR_FALSE);

            str = cJSON_GetString(pUp, "nid", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szNid);
                if (strlen(str) > 0) data->xUp.szNid = rt_strdup(str);
            }
            str = cJSON_GetString(pUp, "fid", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szFid);
                if (strlen(str) > 0) data->xUp.szFid = rt_strdup(str);
            }
            str = cJSON_GetString(pUp, "unit", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szUnit);
                if (strlen(str) > 0) data->xUp.szUnit = rt_strdup(str);
            }
            
            data->xUp.pi = cJSON_GetInt(pUp, "pi", 4);

            data->xUp.usDevType = cJSON_GetInt(pUp, "dt", data->xUp.usDevType);
            data->xUp.usDevNum = cJSON_GetInt(pUp, "dtn", data->xUp.usDevNum);
            data->xUp.usProtoType = cJSON_GetInt(pUp, "pt", data->xUp.usProtoType);
            str = cJSON_GetString(pUp, "pnm", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szProtoName);
                if (strlen(str) > 0) data->xUp.szProtoName = rt_strdup(str);
            }

            str = cJSON_GetString(pUp, "desc", VAR_NULL);
            if (str) {
                VAR_MANAGE_FREE(data->xUp.szDesc);
                if (strlen(str) > 0) data->xUp.szDesc = rt_strdup(str);
            }
        }

        return VAR_TRUE;
    }

    return VAR_FALSE;
}

void cfgSetVarManageExtDataWithJson(cJSON *pCfg)
{
    ExtData_t *data = VAR_MANAGE_CALLOC(sizeof(ExtData_t), 1);
    if(data) {
        jsonParseExtDataInfo(pCfg, data);
        board_cfg_varext_add(data);
        VAR_MANAGE_FREE(data);
    }
}

void setVarManageExtDataWithJson(cJSON *pCfg)
{
    ExtData_t data_bak;
    int n = cJSON_GetInt(pCfg, "n", -1);
    if (n >= 0) {
        ExtData_t *data = RT_NULL;
        ExtData_t *datacopy = RT_NULL;
        var_bool_t add = RT_FALSE;

        // add by jay 2016/11/25
        // 该过程禁止打断, 不能有任何挂起操作
        rt_enter_critical();
        {
            if (s_xExtDataList.n < n+1) {
                add = RT_TRUE;
                data = VAR_MANAGE_CALLOC(sizeof(ExtData_t), 1);
                if(data) {
                    vVarManage_InsertNode(data);
                }
            } else {
                data = s_xExtDataList.pList;
                while (data && n--) {
                    data = data->next;
                }
            }

            if(data) {
                memcpy(&data_bak, data, sizeof(ExtData_t));
                jsonParseExtDataInfo(pCfg, data);

                if (memcmp(&data_bak, data, sizeof(ExtData_t)) != 0) {
                    ExtData_t *node = s_xExtDataList.pList;

                    while (node) {
                        node->xUp.xAvgUp.count = 0;
                        node->xUp.xAvgUp.val_avg = 0;
                        node->xUp.xAvgUp.val_cur = 0;
                        node->xUp.xAvgUp.time = rt_tick_get();
                        node = node->next;
                    }

                    memset(&data->xStorage.xAvgReal, 0, sizeof(VarAvg_t));
                    data->xStorage.xAvgReal.time = rt_tick_get();
                    memset(&data->xStorage.xAvgMin, 0, sizeof(VarAvg_t));
                    data->xStorage.xAvgMin.time = rt_tick_get();
                    memset(&data->xStorage.xAvgHour, 0, sizeof(VarAvg_t));
                    data->xStorage.xAvgHour.time = rt_tick_get();
                }
                datacopy = pVarManage_CopyData( data );
            }
        }
        rt_exit_critical();
        if( datacopy ) {
            if (add) {
                board_cfg_varext_add(datacopy);
            } else {
                board_cfg_varext_update(data_bak.xName.szName, datacopy);
            }
            vVarManage_FreeData(datacopy);
        }
        __sort_extdata_link();
        __RefreshRegsFlag();
        __creat_sync_slave_link();
        varmanage_update();
    }
}

DEF_CGI_HANDLER(setVarManageExtData)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setVarManageExtDataWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(delVarManageExtData)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        const char *name = cJSON_GetString(pCfg, "na", VAR_NULL);
        if (name && strlen(name) > 0) {
            if (board_cfg_varext_del(name)) {
                vVarManage_RemoveNodeWithName(name);
            } else {
                err = RT_ERROR;
            }
            __sort_extdata_link();
            __RefreshRegsFlag();
            __creat_sync_slave_link();
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

// return str len
static int __getValAsStr( var_uint8_t type, VarValue_v *value, char *str, int size )
{
    if (value && str && size>0) {
        switch (type) {
        case E_VAR_BIT:
            snprintf(str, size, "%u", value->val_bit);
            break;
        case E_VAR_INT8:
            snprintf(str, size, "%d", value->val_i8);
            break;
        case E_VAR_UINT8:
            snprintf(str, size, "%u", value->val_u8);
            break;
        case E_VAR_INT16:
            snprintf(str, size, "%d", value->val_i16);
            break;
        case E_VAR_UINT16:
            snprintf(str, size, "%u", value->val_u16);
            break;
        case E_VAR_INT32:
            snprintf(str, size, "%d", value->val_i32);
            break;
        case E_VAR_UINT32:
            snprintf(str, size, "%u", (uint32_t)value->val_u32);
            break;
        case E_VAR_FLOAT:
            snprintf(str, size, "%.4f", value->val_f);
            break;
        case E_VAR_DOUBLE:
            snprintf(str, size, "%.4f", value->val_db);
            break;
        case E_VAR_ARRAY:
            snprintf(str, size, "%s", "ARRAY");
            break;
        }
        return strlen(str);
    }
    
    return 0;
}

var_bool_t jsonFillExtDataInfo(ExtData_t *data, cJSON *pItem)
{
    if (data && pItem) {
        cJSON *pIO = cJSON_CreateObject();
        cJSON *pAlarm = cJSON_CreateObject();
        cJSON *pStorage = cJSON_CreateObject();
        cJSON *pUp = cJSON_CreateObject();
        cJSON_AddNumberToObject(pItem, "en", data->bEnable ? 1 : 0);
        cJSON_AddStringToObject(pItem, "na", data->xName.szName);
        cJSON_AddStringToObject(pItem, "al", data->xAlias.szAlias);
        cJSON_AddItemToObject(pItem, "io", pIO);
        cJSON_AddItemToObject(pItem, "alarm", pAlarm);
        cJSON_AddItemToObject(pItem, "storage", pStorage);
        cJSON_AddItemToObject(pItem, "up", pUp);

        cJSON_AddNumberToObject(pIO, "eop", data->xIo.btErrOp);
        cJSON_AddNumberToObject(pIO, "ecnt", data->xIo.btErrCnt);
        cJSON_AddNumberToObject(pIO, "rw", data->xIo.btRWType);
        cJSON_AddNumberToObject(pIO, "vt", data->xIo.btVarType);
        cJSON_AddNumberToObject(pIO, "vs", data->xIo.btVarSize);
        cJSON_AddNumberToObject(pIO, "vrl", data->xIo.btVarRuleType);
        {
            char szStr[24] = { 0 };
            __getValAsStr(data->xIo.btVarType, &data->xIo.xValue, szStr, sizeof(szStr));
            if (szStr[0]) {
                cJSON_AddStringToObject(pIO, "va", szStr);
            }
        }

        if (data->xIo.bUseMax) {
            cJSON_AddNumberToObject(pIO, "vma", data->xIo.fMax);
        }
        if (data->xIo.bUseMin) {
            cJSON_AddNumberToObject(pIO, "vmi", data->xIo.fMin);
        }
        if (data->xIo.bUseInit) {
            cJSON_AddNumberToObject(pIO, "vii", data->xIo.fInit);
        }
        if (data->xIo.bUseRatio) {
            cJSON_AddNumberToObject(pIO, "vrt", data->xIo.fRatio);
        }

        cJSON_AddNumberToObject(pIO, "dt", data->xIo.usDevType);
        cJSON_AddNumberToObject(pIO, "dtn", data->xIo.usDevNum);
        cJSON_AddNumberToObject(pIO, "pt", data->xIo.usProtoType);
        cJSON_AddStringToObject(pIO, "pnm", data->xIo.szProtoName ? data->xIo.szProtoName : "");

        cJSON_AddNumberToObject(pIO, "ad", data->xIo.usAddr);
        
        if (__proto_is_modbus(data->xIo.usDevType, data->xIo.usProtoType)) {
            cJSON_AddNumberToObject(pIO, "mbop", data->xIo.param.modbus.btOpCode);
            cJSON_AddNumberToObject(pIO, "sa", data->xIo.param.modbus.btSlaveAddr);
            cJSON_AddNumberToObject(pIO, "sfa", data->xIo.param.modbus.nSyncFAddr);
            cJSON_AddNumberToObject(pIO, "ea", data->xIo.param.modbus.usExtAddr);
            cJSON_AddNumberToObject(pIO, "ao", data->xIo.param.modbus.usAddrOfs);
        } else if (__proto_is_dlt645(data->xIo.usDevType, data->xIo.usProtoType)) {
            {
                char szStr[24] = {0};
                var_uint8_t *addr = data->xIo.param.dlt645.addr.addr;
                sprintf(szStr, "%02X%02X%02X%02X%02X%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
                cJSON_AddStringToObject(pIO, "dltad", szStr);
            }
            cJSON_AddNumberToObject(pIO, "dltop", data->xIo.param.dlt645.op);
        } else if (__proto_is_dust(data->xIo.usDevType, data->xIo.usProtoType)) {
            cJSON_AddNumberToObject(pIO, "dustop", data->xIo.param.dust.op);
        }
        
        cJSON_AddStringToObject(pIO, "exp", data->xIo.szExp ? data->xIo.szExp : "");

        cJSON_AddNumberToObject(pStorage, "se", data->xStorage.bEnable ? 1 : 0);
        cJSON_AddNumberToObject(pStorage, "ss", data->xStorage.ulStep);
        cJSON_AddNumberToObject(pStorage, "st", data->xStorage.btType);
        cJSON_AddNumberToObject(pStorage, "sc", data->xStorage.bCheck);
        cJSON_AddNumberToObject(pStorage, "sma", data->xStorage.fMax);
        cJSON_AddNumberToObject(pStorage, "smi", data->xStorage.fMin);

        cJSON_AddNumberToObject(pAlarm, "en", data->xAlarm.bEnable);

        cJSON_AddNumberToObject(pUp, "en", data->xUp.bEnable ? 1 : 0);

        cJSON_AddStringToObject(pUp, "nid", data->xUp.szNid ? data->xUp.szNid : "");
        cJSON_AddStringToObject(pUp, "fid", data->xUp.szFid ? data->xUp.szFid : "");
        cJSON_AddStringToObject(pUp, "unit", data->xUp.szUnit ? data->xUp.szUnit : "");
        
        cJSON_AddNumberToObject(pUp, "pi", data->xUp.pi);

        cJSON_AddNumberToObject(pUp, "dt", data->xUp.usDevType);
        cJSON_AddNumberToObject(pUp, "dtn", data->xUp.usDevNum);
        cJSON_AddNumberToObject(pUp, "pt", data->xUp.usProtoType);
        cJSON_AddStringToObject(pUp, "pnm", data->xUp.szProtoName ? data->xUp.szProtoName : "");

        cJSON_AddStringToObject(pUp, "desc", data->xUp.szDesc ? data->xUp.szDesc : "");

        return VAR_TRUE;
    }

    return VAR_FALSE;
}

int nExtDataListCnt(void)
{
    return s_xExtDataList.n;
}

void jsonFillExtDataInfoWithNum(int n, cJSON *pItem)
{
    if(pItem && n >= 0 && n < s_xExtDataList.n) {
        ExtData_t *data = __GetExtDataWithIndex(n);
        if(data) {
            cJSON_AddNumberToObject(pItem, "n", n);
            jsonFillExtDataInfo(data, pItem);
            vVarManage_FreeData(data);
        }
    }
}


DEF_CGI_HANDLER(getVarManageExtDataVals)
{
    char *szRetJSON = RT_NULL;
    // add by jay 2016/11/25
    // 该过程禁止打断, 不能有任何挂起操作
    rt_enter_critical();
    {
        rt_bool_t first = RT_TRUE;
        int len = 32;
        ExtData_t *node = s_xExtDataList.pList;
        while (node) {
            char str[24];
            len += __getValAsStr(node->xIo.btVarType, &node->xIo.xValue, str, sizeof(str) );
            len+=3; // add ',', '"', '"'
            node = node->next;
        }
        szRetJSON = VAR_MANAGE_CALLOC(1, len);
        if (szRetJSON) {
            ExtData_t *node = s_xExtDataList.pList;
            char *p = szRetJSON;
            p += sprintf(p, "%s", "{\"ret\":0,\"list\":[");
            while (node) {
                char str[24] = {0};
                __getValAsStr(node->xIo.btVarType, &node->xIo.xValue, str, sizeof(str));
                if (!first) *p++ = ',';
                first = RT_FALSE;
                *p++ = '\"';
                p += sprintf(p, "%s", str);
                *p++ = '\"';
                node = node->next;
            }
            p += sprintf(p, "%s", "]}");
        }
    }
    rt_exit_critical();
    if( szRetJSON ) {
        WEBS_PRINTF(szRetJSON);
        rt_free(szRetJSON);
    } else {
        WEBS_PRINTF("{\"ret\":1}");
    }
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getVarManageExtData)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    char *szRetJSON = RT_NULL;
    rt_bool_t first = RT_TRUE;

    if (pCfg) {
        int all = cJSON_GetInt(pCfg, "all", 0);
        if (all) {
            first = RT_TRUE;
            WEBS_PRINTF("{\"ret\":0,\"list\":[");
            int num = s_xExtDataList.n;
            for( int i = 0; i < num; i++ ) {
                ExtData_t *node = __GetExtDataWithIndex(i);
                cJSON *pItem = cJSON_CreateObject();
                if(pItem && node) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    jsonFillExtDataInfo(node, pItem);
                    szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                    vVarManage_FreeData(node);
                }
                cJSON_Delete(pItem);
            }

            // add matser protolist
            WEBS_PRINTF("],\"protolist\":{");

            // rs proto
            WEBS_PRINTF("\"rs\":[");
            first = RT_TRUE;
            for (int n = 0; n < 4; n++) {
                rt_int8_t instance = nUartGetInstance(n);
                if (instance >= 0 && !g_xfer_net_dst_uart_occ[instance]) {
                    if (
                            PROTO_DLT645 == g_uart_cfgs[instance].proto_type || 
                            PROTO_DLT645_1997 == g_uart_cfgs[instance].proto_type || 
                            PROTO_DUST == g_uart_cfgs[instance].proto_type
                        ) 
                    {
                        g_uart_cfgs[instance].proto_ms = PROTO_MASTER;
                    }
                    if (PROTO_MASTER == g_uart_cfgs[instance].proto_ms) {
                        cJSON *pItem = cJSON_CreateObject();
                        if(pItem) {
                            if (!first) WEBS_PRINTF(",");
                            first = RT_FALSE;
                            cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_RS1 + n);
                            cJSON_AddNumberToObject(pItem, "idn", 0);
                            cJSON_AddNumberToObject(pItem, "po", g_uart_cfgs[instance].proto_type);
                            char *szRetJSON = cJSON_PrintUnformatted(pItem);
                            if(szRetJSON) {
                                WEBS_PRINTF(szRetJSON);
                                rt_free(szRetJSON);
                            }
                        }
                        cJSON_Delete(pItem);
                    }
                }
            }
            WEBS_PRINTF("],");

            // net proto
            first = RT_TRUE;
            WEBS_PRINTF("\"net\":[");
            for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
                if (g_tcpip_cfgs[n].enable && NET_IS_NORMAL(n) && PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_NET);
                        cJSON_AddNumberToObject(pItem, "idn", n);
                        cJSON_AddNumberToObject(pItem, "po", g_tcpip_cfgs[n].cfg.normal.proto_type);
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }
            WEBS_PRINTF("],");

            // zigbee proto
            first = RT_TRUE;
            WEBS_PRINTF("\"zigbee\":[");
            if (ZIGBEE_WORK_COORDINATOR == g_zigbee_cfg.xInfo.WorkMode && ZGB_TM_GW == g_zigbee_cfg.tmode) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_ZIGBEE);
                    cJSON_AddNumberToObject(pItem, "idn", 0);
                    cJSON_AddNumberToObject(pItem, "po", PROTO_MODBUS_RTU);
                    char *szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
            WEBS_PRINTF("],");

            // gprs proto
            first = RT_TRUE;
            WEBS_PRINTF("\"gprs\":[");
            for (int n = BOARD_ENET_TCPIP_NUM; n < BOARD_ENET_TCPIP_NUM + BOARD_GPRS_TCPIP_NUM; n++) {
                if(NET_IS_NORMAL(n) && PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_GPRS);
                        cJSON_AddNumberToObject(pItem, "idn", n - BOARD_ENET_TCPIP_NUM);
                        cJSON_AddNumberToObject(pItem, "po", g_tcpip_cfgs[n].cfg.normal.proto_type);
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }

            // add upload protolist
            WEBS_PRINTF("]},\"upprotolist\":{");

            // rs proto
            WEBS_PRINTF("\"rs\":[");
            WEBS_PRINTF("],");

            // net proto
            first = RT_TRUE;
            WEBS_PRINTF("\"net\":[");
            for (int n = 0; n < BOARD_ENET_TCPIP_NUM; n++) {
                if(g_tcpip_cfgs[n].enable && NET_IS_NORMAL(n) && 
                  (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_DM101 == g_tcpip_cfgs[n].cfg.normal.proto_type
                  )) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_NET);
                        cJSON_AddNumberToObject(pItem, "idn", n);
                        cJSON_AddNumberToObject(pItem, "po", g_tcpip_cfgs[n].cfg.normal.proto_type);
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }
            WEBS_PRINTF("],");

            // zigbee proto
            first = RT_TRUE;
            WEBS_PRINTF("\"zigbee\":[");
            WEBS_PRINTF("],");

            // gprs proto
            first = RT_TRUE;
            WEBS_PRINTF("\"gprs\":[");
            for (int n = BOARD_ENET_TCPIP_NUM; n < BOARD_TCPIP_MAX; n++) {
                if(g_tcpip_cfgs[n].enable && NET_IS_NORMAL(n) && 
                  (
                    PROTO_CC_BJDC == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_HJT212 == g_tcpip_cfgs[n].cfg.normal.proto_type || 
                    PROTO_DM101 == g_tcpip_cfgs[n].cfg.normal.proto_type
                  )) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        if (!first) WEBS_PRINTF(",");
                        first = RT_FALSE;
                        cJSON_AddNumberToObject(pItem, "id", PROTO_DEV_GPRS);
                        cJSON_AddNumberToObject(pItem, "idn", n);
                        cJSON_AddNumberToObject(pItem, "po", g_tcpip_cfgs[n].cfg.normal.proto_type);
                        char *szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                }
            }
            WEBS_PRINTF("]}}");        // }} end array and end json

        } else {
            int n = cJSON_GetInt(pCfg, "n", -1);
            if (n >= 0 && n < VAR_EXT_DATA_SIZE) {
                ExtData_t *data = __GetExtDataWithIndex(n);
                if (data) {
                    cJSON *pItem = cJSON_CreateObject();
                    if(pItem) {
                        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
                        jsonFillExtDataInfo(data, pItem);
                        szRetJSON = cJSON_PrintUnformatted(pItem);
                        if(szRetJSON) {
                            WEBS_PRINTF(szRetJSON);
                            rt_free(szRetJSON);
                        }
                    }
                    cJSON_Delete(pItem);
                    vVarManage_FreeData(data);
                }
            }
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    if (err != RT_EOK) {
        WEBS_PRINTF("{\"ret\":%d}", err);
    }
    WEBS_DONE(200);
}

