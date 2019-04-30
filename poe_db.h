
#ifndef __POE_DB_H__
#define __POE_DB_H__

#include <stdio.h>
#include "obj_intf_poe.h"
#include "tps238x.h"
#include "poe_tps_cfg.h"

#define POE_MAX_PORT                (24)
#define POE_MIN_I2C_ADDR            (0x20)
#define POE_MAX_I2C_ADDR            (0x2b)
#define TI_CHIP_NUM                 (12)

#define POE_MAX_POWER_INVALID           0xFFFFFFFFUL
#define POE_MAX_POWER_DEFAULT           POE_MAX_POWER_INVALID
#define SSA_POE_MAXPOWER_DEF            POE_MAX_POWER_DEFAULT

#define POE_LPORT_INVALID(lport)        ((lport) < 1 || (lport) > POE_MAX_PORT)
#define POE_I2C_ADDRESS_INVALID(i2c)    ((i2c) < POE_MIN_I2C_ADDR || (i2c) > POE_MAX_I2C_ADDR)

#define POE_SUPPORT_ICUT_INVALID     0xFFFFFFFFUL
#define POE_SUPPORT_ICUT_DEFAULT     POE_SUPPORT_ICUT_INVALID

/*
 * cool-down时间，芯片设置默认为1s，改成4s
*/
#define TI_COOL_DOWN_PERIOD                   (1)

/*
 * 高温下电，软件自定义一个冷却期不让上电
*/
#define TI_TEMP_COOL_DOWN_PERIOD              (60)

/*
 * 欠压下电，软件自定义一个冷却期不让上电
*/
#define TI_UV_COOL_DOWN_PERIOD                (45)

/* poe端口下电原因 */
typedef enum {
    POE_NORMAL = 0,        /* 正常供电 */
    POE_OVER_CUT,          /* 供电阶段，由于电流太大而断开 */
    POE_OVERLOAD,          /* PD设备过载而断开 */
    POE_SHORT_CIRCUIT,     /* PD设备短路而断开 */
    POE_OVER_TEMP,         /* 高温保护而关闭 */
    POE_PM_OFF,            /* 功率管理而关闭 */
    POE_PSE_ERROR,         /* 因为PSE芯片被复位，硬件故障而关闭 */
    POE_OVER_VOL,          /* 输出电压超限而关闭 */
    //POE_PSE_SW_DISABLE,       /* 线卡使能下电 */
    //POE_PSE_HW_DISABLE,     /* 线卡按钮下电 */
} poe_fault_t;

/* poe软件上的分级 */
typedef enum poe_pd_class_e {
    PD_CLASS_0 = 0,
    PD_CLASS_1,
    PD_CLASS_2,
    PD_CLASS_3,
    PD_CLASS_4,
    PD_CLASS_5,         
    PD_CLASS_6,
    PD_CLASS_7,
    PD_CLASS_8,
    PD_CLASS_4PLUS_TYPE1,
    PD_CLASS4_4P_DUAL,     /* class 5 dual */
    PD_CLASS_UNKNOWN,
    PD_CLASS_MAX,
} poe_pd_class_t;

/* TPS23880芯片端口限流值, 单位: W */
typedef enum {
    TI_PCUT_4       = 8,
    TI_PCUT_7       = 14,
    TI_PCUT_15_4    = 31,
    TI_PCUT_30      = 60,
    TI_PCUT_45      = 90,
    TI_PCUT_60      = 120,
    TI_PCUT_75      = 150,
    TI_PCUT_90      = 180
} ti_pcut_ctrl_t;

typedef enum {
    POE_LLDP_PDTYPE_1 = 1,
    POE_LLDP_PDTYPE_2,
    POE_LLDP_PDTYPE_CLASS,
    POE_LLDP_PDTYPE_MAX,
} poe_lldp_pdtype_t;

/* poe端口状态 */
typedef struct poe_port_status_s {
    /* 芯片信息 */
    bool power_enable;                      /* 端口是否上电 */
    TPS238x_Detection_Status_t detect;      /* 检测结果 */
    int class;                              /* 分级结果 */
    uint32_t cur;                           /* 端口电流，mA */
    uint32_t vol;                           /* 端口电压，mV */
    uint32_t power;                         /* 端口供电功率, mW, current ×voltage */
    uint32_t resistence;                    /* 端口检测电阻, kΩ */
    poe_fault_t reason;                     /* 端口下电原因 */
    uint32_t pdtype;                        /* LLDP 检测pd 类型*/

    int phyid;                              /* 端口对应的phyid */
} poe_port_status_t;

typedef enum {
    POE_PORT_PRI_NONE = 0,
    POE_PORT_PRI_CRITICAL = 1,  /* 最高优先级 */
    POE_PORT_PRI_HIGH,          /* 高优先级 */
    POE_PORT_PRI_LOW,           /* 低优先级 */
    POE_PORT_PRI_MAX,
} poe_port_pri_t;

/* 端口配置信息，来自ss */
typedef struct ssa_poe_ti_port_config_e {
    bool           poe_enable;            /* 端口是否使能poe功能 */
    bool           force_on;              /* 是否配置强制供电 */
    bool           legacy;                /* 是否开启非标PD兼容 */
    poe_port_pri_t port_priority;         /* 端口优先级, 自动模式和节能模式有效 */
    uint32_t       static_power;          /* 静态模式固定分配功率 */
    uint32_t       max_power;             /* 最大功率, 非静态模式下有效 */
} ssa_poe_ti_port_config_t;

/* 软件状态机 */
typedef enum {
    TI_PORT_OFF,    /* PoE disable */
    TI_PORT_ON,     /* PoE enable and work well */
    TI_PORT_STRTF,  /* Overload during start-up */
    TI_PORT_ICUT,   /* Icut fault */
    TI_PORT_ILIM,   /* Ilim fault */
    TI_PORT_OVL,    /* Port overload */
    TI_PORT_TEMP,   /* Over temperature */
    TI_PORT_PM,     /* Power management shutdown */
    TI_PORT_VDUV,   /* VDUV fault */
    TI_PORT_VPUV    /* VPUV fault */
} ti_port_state_t;

/* 端口控制信息，来自本层 */
typedef struct ssa_poe_ti_port_ctrl_e {
    uint32_t          lport;              /* 该端口对应的面板口 */
    bool              pwon;               /* 端口是否已经上电使能，ps:上电需手动控制，所以这里标记一下 */
    uint32_t          support_icut;       /* 硬件电流阀值记录，只在设置SUPPORT_ICUT时设置，隐藏命令设置 */
    ti_pcut_ctrl_t    pcut_ctrl;          /* 设置端口icut值 */
    poe_fault_t       power_off_reason;   /* 端口的下电原因，目前有记录，但没有使用，若需要可读取 */
    ti_port_state_t   port_state;         /* 端口状态 */
    uint32_t          cool_down_time;     /* 发生start/icut/ilim事件时，芯片端口会进入冷却，软件也要计算冷却时间好恢复端口状态 */
    uint32_t          icutf_cnt;          /* 端口icut过流计数 */
    uint32_t          ilimf_cnt;          /* 端口ilim过流计数 */
    uint32_t          strtf_cnt;          /* 端口冲击电流过大计数 */
    uint32_t          alloc_power;        /* 端口分配功率,静态模式为可以上电的静态功率 420178 */
    poe_lldp_pdtype_t lldp_pdtype;        /* PD的LLDP二次分级类型 */
} ssa_poe_ti_port_ctrl_t;

/* 单端口信息 */
typedef struct ssa_poe_ti_port_info_e {
    poe_port_status_t        ti_port_data;
    ssa_poe_ti_port_config_t ti_port_config;
    ssa_poe_ti_port_ctrl_t   ti_port_ctrl;
} ssa_poe_ti_port_info_t;

typedef enum {
    POE_PM_AUTO = 1,          /* auto模式 */
    POE_PM_ENERGYSAVE,      /* energy-saving模式 */
    POE_PM_STATIC,             /* static模式 */
    POE_PM_MAX,
} poe_pm_t;

typedef enum {
    POE_DISCONNECT_DC    = 0x1,
    POE_DISCONNECT_AC    = 0x2,
    POE_DISCONNECT_BOTH  = 0x3,
} poe_disc_t;

/* 单芯片信息(这里的芯片指一个i2c地址) */
typedef struct ssa_poe_ti_chip_info_e {
    ssa_poe_ti_port_info_t ti_port_info[2 + 1];
} ssa_poe_ti_chip_info_t;

/* 系统信息 */
typedef struct ssa_poe_ti_sys_info_e {
    bool       glb_poe_enable;     /* 全局使能供电 */
    bool       power_exist;        /* 是否有接电源 */
    //bool       cpld_reset;         /* CPLD的reset寄存器是否使能过，冷启动会使能一次 */
    //bool     pse_hw_enable;      /* 硬件全局使能供电 */
    bool       lldp_enable;        /* PD的LLDP是否开启 */
    bool       ups_enable;         /* 是否开启了热启动不间断供电功能 */
    poe_ti_port_map_t *ti_port_map;
    poe_pm_t   pm_mode;            /* 供电管理模式 */
    poe_disc_t disc_mode;          /* 断接侦测模式 */
    uint32_t   poe_total_power;    /* 电源总功率 */
    uint32_t   reserve_power_rate; /* 预留功率比例，节能模式有效 */
    uint32_t   reserve_power;      /* 预留功率值， poe_total_power ×reserve_power_rate ÷100 */
    uint32_t   total_cons_power;   /* 总消耗功率 */
    uint32_t   total_alloc_power;  /* 总分配功率 */
    uint32_t   remain_power;       /* 剩余分配功率 */
    uint32_t   glb_cool_down_time; /* 全局冷却 */
    ssa_poe_ti_chip_info_t ti_chip_info[TI_CHIP_NUM + 1];
} ssa_poe_ti_sys_info_t;

/* 
 * system info access macros
 */
extern ssa_poe_ti_sys_info_t ssa_ti_sys_info;

#define SSA_TI_SYS_INFO ssa_ti_sys_info

#define TI_SYS_GLB_POE_EN        (SSA_TI_SYS_INFO.glb_poe_enable)
#define TI_SYS_POWER_EXIST       (SSA_TI_SYS_INFO.power_exist)
//#define TI_SYS_CPLD_RESET        (SSA_TI_SYS_INFO.cpld_reset)
#define TI_SYS_LLDP_EN           (SSA_TI_SYS_INFO.lldp_enable)
#define TI_SYS_UPS_EN            (SSA_TI_SYS_INFO.ups_enable)
#define TI_SYS_PM_MODE           (SSA_TI_SYS_INFO.pm_mode)
#define TI_SYS_DISC_MODE         (SSA_TI_SYS_INFO.disc_mode)
#define TI_SYS_POE_TOTAL_POWER   (SSA_TI_SYS_INFO.poe_total_power)
#define TI_SYS_RESERVE_PWR_RATE  (SSA_TI_SYS_INFO.reserve_power_rate)
#define TI_SYS_RESERVE_POWER     (SSA_TI_SYS_INFO.reserve_power)
#define TI_SYS_CONSUME_POWER     (SSA_TI_SYS_INFO.total_cons_power)
#define TI_SYS_ALLOC_POWER       (SSA_TI_SYS_INFO.total_alloc_power)
#define TI_SYS_REMAIN_POWER      (SSA_TI_SYS_INFO.remain_power)
#define TI_SYS_CHIP_INFO(_chip)  (SSA_TI_SYS_INFO.ti_chip_info[_chip])
#define TI_SYS_PORT_MAP          (SSA_TI_SYS_INFO.ti_port_map)
#define TI_SYS_COOL_DOWN_TIME    (SSA_TI_SYS_INFO.glb_cool_down_time)

/* 
 * 根据lport获得i2c address和chip_port
*/
#define TI_GET_I2CADDR(_lport)    (TI_SYS_PORT_MAP[_lport].i2c_address)
#define TI_GET_CHIPID(_lport)    (TI_SYS_PORT_MAP[_lport].chipid)
#define TI_GET_CHIP_PORT(_lport) (TI_SYS_PORT_MAP[_lport].chip_port)

/* 0x20 对应chip 1 */
#define TI_GET_I2C(_chip) (_chip + 31)  
/* 
 * port info access macros
*/
/* get _port_info by chipid and chip_port */
#define TI_PORT_INFO(_chip, _chip_port) &(TI_SYS_CHIP_INFO(_chip).ti_port_info[_chip_port])
/* get _port_info by lport */
#define TI_LPORT_INFO(_lport) TI_PORT_INFO(TI_GET_CHIPID(_lport), TI_GET_CHIP_PORT(_lport))

/* 
 * port ctrl info access macros 
*/
#define TI_PORT_CTRL(_port_info)                ((_port_info)->ti_port_ctrl)
#define TI_PORT_CTRL_LPORT(_port_info)          (TI_PORT_CTRL(_port_info).lport)
#define TI_PORT_CTRL_PWON(_port_info)           (TI_PORT_CTRL(_port_info).pwon)
#define TI_PORT_CTRL_SUPPORT_ICUT(_port_info)   (TI_PORT_CTRL(_port_info).support_icut)
#define TI_PORT_CTRL_ICUT(_port_info)           (TI_PORT_CTRL(_port_info).pcut_ctrl)
#define TI_PORT_CTRL_OFF_REASON(_port_info)     (TI_PORT_CTRL(_port_info).power_off_reason)
#define TI_PORT_CTRL_STATE(_port_info)          (TI_PORT_CTRL(_port_info).port_state)
#define TI_PORT_CTRL_COOL_DOWN_TIME(_port_info) (TI_PORT_CTRL(_port_info).cool_down_time)
#define TI_PORT_CTRL_ICUT_COUNT(_port_info)     (TI_PORT_CTRL(_port_info).icutf_cnt)
#define TI_PORT_CTRL_ILIM_COUNT(_port_info)     (TI_PORT_CTRL(_port_info).ilimf_cnt)
#define TI_PORT_CTRL_ISTRT_COUNT(_port_info)    (TI_PORT_CTRL(_port_info).strtf_cnt)
#define TI_PORT_CTRL_ALLOC_POWER(_port_info)    (TI_PORT_CTRL(_port_info).alloc_power)
#define TI_PORT_CTRL_LLDP_PDTYPE(_port_info)    (TI_PORT_CTRL(_port_info).lldp_pdtype)

/* 
 * port config info access macros 
*/
#define TI_PORT_CONFIG(_port_info)              ((_port_info)->ti_port_config)
#define TI_PORT_CONFIG_POE_EN(_port_info)       (TI_PORT_CONFIG(_port_info).poe_enable)
#define TI_PORT_CONFIG_FORCEON_EN(_port_info)   (TI_PORT_CONFIG(_port_info).force_on)
#define TI_PORT_CONFIG_LEGACY_EN(_port_info)    (TI_PORT_CONFIG(_port_info).legacy)
#define TI_PORT_CONFIG_PRIORITY(_port_info)     (TI_PORT_CONFIG(_port_info).port_priority)
#define TI_PORT_CONFIG_STATIC_POWER(_port_info) (TI_PORT_CONFIG(_port_info).static_power)
#define TI_PORT_CONFIG_MAX_POWER(_port_info)    (TI_PORT_CONFIG(_port_info).max_power)

/*
 * port data info access macros
*/
#define TI_PORT_DATA(_port_info)                ((_port_info)->ti_port_data)
#define TI_PORT_DATA_POWER_UP(_port_info)       (TI_PORT_DATA(_port_info).power_enable)
#define TI_PORT_DATA_DET_STATUS(_port_info)     (TI_PORT_DATA(_port_info).detect)
#define TI_PORT_DATA_CLS_STATUS(_port_info)     (TI_PORT_DATA(_port_info).class)
#define TI_PORT_DATA_CURRENT(_port_info)        (TI_PORT_DATA(_port_info).cur)
#define TI_PORT_DATA_VOLTAGE(_port_info)        (TI_PORT_DATA(_port_info).vol)
#define TI_PORT_DATA_CONSUME_POWER(_port_info)  (TI_PORT_DATA(_port_info).power)
#define TI_PORT_DATA_DET_RESISTANCE(_port_info) (TI_PORT_DATA(_port_info).resistence)
#define TI_PORT_DATA_PHTID(_port_info)          (TI_PORT_DATA(_port_info).phyid)

poe_port_status_t *poe_db_get_port(int lport);

#endif
