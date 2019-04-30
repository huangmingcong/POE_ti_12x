
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
 * cool-downʱ�䣬оƬ����Ĭ��Ϊ1s���ĳ�4s
*/
#define TI_COOL_DOWN_PERIOD                   (1)

/*
 * �����µ磬����Զ���һ����ȴ�ڲ����ϵ�
*/
#define TI_TEMP_COOL_DOWN_PERIOD              (60)

/*
 * Ƿѹ�µ磬����Զ���һ����ȴ�ڲ����ϵ�
*/
#define TI_UV_COOL_DOWN_PERIOD                (45)

/* poe�˿��µ�ԭ�� */
typedef enum {
    POE_NORMAL = 0,        /* �������� */
    POE_OVER_CUT,          /* ����׶Σ����ڵ���̫����Ͽ� */
    POE_OVERLOAD,          /* PD�豸���ض��Ͽ� */
    POE_SHORT_CIRCUIT,     /* PD�豸��·���Ͽ� */
    POE_OVER_TEMP,         /* ���±������ر� */
    POE_PM_OFF,            /* ���ʹ�����ر� */
    POE_PSE_ERROR,         /* ��ΪPSEоƬ����λ��Ӳ�����϶��ر� */
    POE_OVER_VOL,          /* �����ѹ���޶��ر� */
    //POE_PSE_SW_DISABLE,       /* �߿�ʹ���µ� */
    //POE_PSE_HW_DISABLE,     /* �߿���ť�µ� */
} poe_fault_t;

/* poe����ϵķּ� */
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

/* TPS23880оƬ�˿�����ֵ, ��λ: W */
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

/* poe�˿�״̬ */
typedef struct poe_port_status_s {
    /* оƬ��Ϣ */
    bool power_enable;                      /* �˿��Ƿ��ϵ� */
    TPS238x_Detection_Status_t detect;      /* ����� */
    int class;                              /* �ּ���� */
    uint32_t cur;                           /* �˿ڵ�����mA */
    uint32_t vol;                           /* �˿ڵ�ѹ��mV */
    uint32_t power;                         /* �˿ڹ��繦��, mW, current ��voltage */
    uint32_t resistence;                    /* �˿ڼ�����, k�� */
    poe_fault_t reason;                     /* �˿��µ�ԭ�� */
    uint32_t pdtype;                        /* LLDP ���pd ����*/

    int phyid;                              /* �˿ڶ�Ӧ��phyid */
} poe_port_status_t;

typedef enum {
    POE_PORT_PRI_NONE = 0,
    POE_PORT_PRI_CRITICAL = 1,  /* ������ȼ� */
    POE_PORT_PRI_HIGH,          /* �����ȼ� */
    POE_PORT_PRI_LOW,           /* �����ȼ� */
    POE_PORT_PRI_MAX,
} poe_port_pri_t;

/* �˿�������Ϣ������ss */
typedef struct ssa_poe_ti_port_config_e {
    bool           poe_enable;            /* �˿��Ƿ�ʹ��poe���� */
    bool           force_on;              /* �Ƿ�����ǿ�ƹ��� */
    bool           legacy;                /* �Ƿ����Ǳ�PD���� */
    poe_port_pri_t port_priority;         /* �˿����ȼ�, �Զ�ģʽ�ͽ���ģʽ��Ч */
    uint32_t       static_power;          /* ��̬ģʽ�̶����书�� */
    uint32_t       max_power;             /* �����, �Ǿ�̬ģʽ����Ч */
} ssa_poe_ti_port_config_t;

/* ���״̬�� */
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

/* �˿ڿ�����Ϣ�����Ա��� */
typedef struct ssa_poe_ti_port_ctrl_e {
    uint32_t          lport;              /* �ö˿ڶ�Ӧ������ */
    bool              pwon;               /* �˿��Ƿ��Ѿ��ϵ�ʹ�ܣ�ps:�ϵ����ֶ����ƣ�����������һ�� */
    uint32_t          support_icut;       /* Ӳ��������ֵ��¼��ֻ������SUPPORT_ICUTʱ���ã������������� */
    ti_pcut_ctrl_t    pcut_ctrl;          /* ���ö˿�icutֵ */
    poe_fault_t       power_off_reason;   /* �˿ڵ��µ�ԭ��Ŀǰ�м�¼����û��ʹ�ã�����Ҫ�ɶ�ȡ */
    ti_port_state_t   port_state;         /* �˿�״̬ */
    uint32_t          cool_down_time;     /* ����start/icut/ilim�¼�ʱ��оƬ�˿ڻ������ȴ�����ҲҪ������ȴʱ��ûָ��˿�״̬ */
    uint32_t          icutf_cnt;          /* �˿�icut�������� */
    uint32_t          ilimf_cnt;          /* �˿�ilim�������� */
    uint32_t          strtf_cnt;          /* �˿ڳ������������� */
    uint32_t          alloc_power;        /* �˿ڷ��书��,��̬ģʽΪ�����ϵ�ľ�̬���� 420178 */
    poe_lldp_pdtype_t lldp_pdtype;        /* PD��LLDP���ηּ����� */
} ssa_poe_ti_port_ctrl_t;

/* ���˿���Ϣ */
typedef struct ssa_poe_ti_port_info_e {
    poe_port_status_t        ti_port_data;
    ssa_poe_ti_port_config_t ti_port_config;
    ssa_poe_ti_port_ctrl_t   ti_port_ctrl;
} ssa_poe_ti_port_info_t;

typedef enum {
    POE_PM_AUTO = 1,          /* autoģʽ */
    POE_PM_ENERGYSAVE,      /* energy-savingģʽ */
    POE_PM_STATIC,             /* staticģʽ */
    POE_PM_MAX,
} poe_pm_t;

typedef enum {
    POE_DISCONNECT_DC    = 0x1,
    POE_DISCONNECT_AC    = 0x2,
    POE_DISCONNECT_BOTH  = 0x3,
} poe_disc_t;

/* ��оƬ��Ϣ(�����оƬָһ��i2c��ַ) */
typedef struct ssa_poe_ti_chip_info_e {
    ssa_poe_ti_port_info_t ti_port_info[2 + 1];
} ssa_poe_ti_chip_info_t;

/* ϵͳ��Ϣ */
typedef struct ssa_poe_ti_sys_info_e {
    bool       glb_poe_enable;     /* ȫ��ʹ�ܹ��� */
    bool       power_exist;        /* �Ƿ��нӵ�Դ */
    //bool       cpld_reset;         /* CPLD��reset�Ĵ����Ƿ�ʹ�ܹ�����������ʹ��һ�� */
    //bool     pse_hw_enable;      /* Ӳ��ȫ��ʹ�ܹ��� */
    bool       lldp_enable;        /* PD��LLDP�Ƿ��� */
    bool       ups_enable;         /* �Ƿ���������������Ϲ��繦�� */
    poe_ti_port_map_t *ti_port_map;
    poe_pm_t   pm_mode;            /* �������ģʽ */
    poe_disc_t disc_mode;          /* �Ͻ����ģʽ */
    uint32_t   poe_total_power;    /* ��Դ�ܹ��� */
    uint32_t   reserve_power_rate; /* Ԥ�����ʱ���������ģʽ��Ч */
    uint32_t   reserve_power;      /* Ԥ������ֵ�� poe_total_power ��reserve_power_rate ��100 */
    uint32_t   total_cons_power;   /* �����Ĺ��� */
    uint32_t   total_alloc_power;  /* �ܷ��书�� */
    uint32_t   remain_power;       /* ʣ����书�� */
    uint32_t   glb_cool_down_time; /* ȫ����ȴ */
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
 * ����lport���i2c address��chip_port
*/
#define TI_GET_I2CADDR(_lport)    (TI_SYS_PORT_MAP[_lport].i2c_address)
#define TI_GET_CHIPID(_lport)    (TI_SYS_PORT_MAP[_lport].chipid)
#define TI_GET_CHIP_PORT(_lport) (TI_SYS_PORT_MAP[_lport].chip_port)

/* 0x20 ��Ӧchip 1 */
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
