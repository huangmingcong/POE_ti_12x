#ifndef  _POE_TPS_DRV_H_
#define  _POE_TPS_DRV_H_

#include "poe_tps_cfg.h"
#include <tps238x.h>
#include "poe_db.h"
#include "poe_tps_cpld.h"
#include "ssa_poe_mom.h"
#include "poe_ssa_init.h"

/* rg_mom */
#include <libpub/rg_thread/rg_thread.h>
#include <libpub/rg_mom/rg_mom.h>
#include <libpub/rg_mom/rg_global.h>
#include <libpub/rg_mom/rg_mom_common.h>
#include <libpub/rg_mom/rg_mom_sync.h>

/* rg_proto */
//#include "sdp_poe.pb-c.h"

/* rg_at */
#include <rg_at/ssat_intf.h>

#define CHANNEL_1 (0)
#define CHANNEL_2 (1)

/* 芯片命令间隔时间 短 */
#define TI_COMMAND_DELAY_SHORT              (2000)
/* 芯片命令间隔时间 中 */
#define TI_COMMAND_DELAY_MEDIUM             (5000)
/* 芯片命令间隔时间 长 */
#define TI_COMMAND_DELAY_LONG               (10000)

/* TPS23880芯片中断类型 */
typedef enum {
    TI_IRQ_PEC    = 0,    /* power enable status change */
    TI_IRQ_PGC    = 1,    /* power good status change */
    TI_IRQ_DISF   = 2,    /* disconnect event occurred */
    TI_IRQ_DETC   = 3,    /* at least one detection cycle occurred */
    TI_IRQ_CLASC  = 4,    /* at least one classification cycle occurred */
    TI_IRQ_IFAULT = 5,    /* an ICUT or ILIM fault occurred */
    TI_IRQ_STRTF  = 6,    /* a START fault occurred */
    TI_IRQ_SUPF   = 7,    /* a supply event fault occurred */
    TI_IRQ_MAX   = 8
} ti_irq_type_t;

/* 中断对应的各种事件 */
typedef enum {
    TI_IRQ_EVENT_NONE = 0,
    TI_SUPF_TSD,            /* 高温下电事件 */
    TI_SUPF_VPUV,           /* poe电源欠压事件 */
    TI_SUPF_VDUV,           /* 3.3V电源欠压事件 */
    TI_IFAULT_ILIM,         /* ILIM事件 */
    TI_IFAULT_PCUT,         /* 2 pair PCUT事件 */
    TI_STARTF,              /* ISTART事件 */
    TI_DISF,                /* 断接事件 */
    TI_PGC_GOOD,            /* 上电下电事件 */
    TI_PGC_NO_GOOD,         /* 上电下电事件 */
    TI_PEC_ENABLE,          /* 上电下电事件 */
    TI_PEC_NO_ENABLE,       /* 上电下电事件 */
    TI_DETECT_CLSC,         /* 分级事件 */
    TI_DETECT_DET,          /* 检测事件 */
    //TI_DETECT_LEGACY,     /* 检测到非标PD */
    TI_SUPF_OSSE,           /* OSSE 事件 */
    TI_SUPF_PCUT,           /* 4 pair 过载事件 */
    TI_SUPF_RAMFLT,         /* sram异常事件 */
    TI_IRQ_EVENT_MAX
} ti_irq_event_type_t;

typedef struct ti_port_irq_info_e {
    uint32_t port_event_num;
    ti_irq_event_type_t port_event[TI_IRQ_EVENT_MAX];
} ti_port_irq_info_t;

/* 暂存芯片中断信息的结构体 */
typedef struct poe_ssa_ti_chip_irq_info_e {
    ti_port_irq_info_t port_irq_info[2 + 1];    /* 每个i2c地址上有两个chip_port */
} ti_chip_irq_info_t;

#define POE_SSA_FOR_EACH_I2C(i2c_adr) for ((i2c_adr) = 0x20; (i2c_adr) <= 0x2b; (i2c_adr)++)
#define POE_SSA_FOR_EACH_CHIP(chip) for ((chip) = 1; (chip) <= 12; (chip)++)

/**
 * poe_ssa_ti_write_port_disconnect_enable - 设置端口断接侦测使能
 * IN:
 * @lport: 逻辑端口号
 * @enable: true为使能false为失能
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_disconnect_enable(uint32_t lport, bool enable);

/**
 * poe_ssa_ti_write_port_operating_mode - 设置端口操作模式
 * IN:
 * @lport: 逻辑端口号
 * @opmode: 要设置的操作模式
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_operating_mode(uint32_t lport, TPS238x_Operating_Modes_t opmode);

/**
 * poe_ssa_ti_write_port_detect_class_enable - 使能端口检测
 * IN:
 * @lport: 逻辑端口号
 * @enable: ture为打开false为关闭
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_detect_class_enable(uint32_t lport, bool enable);

/**
 * poe_ssa_ti_write_lport_reset - 复位功能
 * IN:
 * @lport: 逻辑端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_lport_reset(uint32_t lport);

/**
 * ssa_poe_ti_write_port_power_enable - 使能端口上电
 * IN:
 * @lport: 逻辑端口号
 * @pwon: ture为上电false为下电
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_power_enable(uint32_t lport, bool pwon);

/**
 * ssa_poe_ti_write_port_pcut_config - 设置端口Pcut
 * IN:
 * @lport: 逻辑端口号
 * @icut: 要设置的icut枚举
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_pcut_config(uint32_t lport, ti_pcut_ctrl_t pcut);

/**
 * ssa_poe_ti_read_port_pcut_config - 设置端口Pcut
 * IN:
 * @lport: 逻辑端口号
 * @icut: 要设置的icut枚举
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_pcut_config(uint32_t lport, ti_pcut_ctrl_t *pcut);

/**
 * ssa_poe_ti_read_port_pcut_config - 读端口Icut
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @icut: icut枚举读存
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_port_pcut_enable(uint32_t lport, bool enable);

/**
 * poe_ssa_ti_write_port_force_on - 设置端口强制供电
 * IN:
 * @lport: 逻辑端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_force_on(uint32_t lport);

/**
 * poe_ssa_ti_write_port_force_off - 关闭端口强制供电
 * IN:
 * @lport: 逻辑端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_force_off(uint32_t lport);

/**
 * poe_ssa_ti_read_port_power_status - 读芯片端口供电状态
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @state: 端口供电状态,false为不供电,true为供电
 *
 * 读端口供电状态 POWER_STATUS_REGISTER 10
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_power_status(uint32_t lport, bool *state);

/**
 * ssa_poe_ti_read_port_detect_class_status - 读端口detect状态
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @detect_status: 芯片端口detect状态
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_detect_status(uint32_t lport, TPS238x_Detection_Status_t *detect_status);

/**
 * poe_ssa_ti_read_port_class_status - 读端口class
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @class_status: 芯片端口clsaa状态
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_class_status(uint32_t lport, int *port_class);

/**
 * poe_ssa_ti_get_port_measurements - 读端口电流值、电压值, mA mV
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @current: 芯片端口current
 * @voltage: 芯片端口current
 *
 * Note: 在POWER_STATUS_REGISTER对应的PGn为1时，才能保证读取到的数值有效
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_get_port_measurements(uint32_t lport, uint32_t *current, uint32_t *voltage);

/**
 * poe_ssa_ti_read_port_resistance - 读端口电阻，kΩ
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @resistance: 芯片端口resistance,不可填NULL
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_resistance(uint32_t lport, uint32_t *resistance);

/**
 * poe_ssa_ti_get_port_power - 读端口消耗功率 mW
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @power: 芯片端口power
 *
 * Note: 在POWER_STATUS_REGISTER对应的PGn为1时，才能保证读取到的数值有效
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_get_port_power(int lport, int *power);

/**
 * poe_ssa_ti_read_device_temperature - 读芯片温度, ℃
 * IN:
 * @chip: 芯片号
 * OUT:
 * @temp: 芯片温度,不可填NULL
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_device_temperature(uint32_t chip, uint32_t *temp);

/**
 * poe_ssa_ti_check_port_detect_class_enable - 检查端口是否det/cls使能
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @enable: ture为打开false为关闭
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_check_port_detect_class_enable(uint32_t lport, bool *enable);

/**
 * poe_ssa_ti_chip_is_work_normal - 判断TI芯片是否正常工作
 *
 * 判断TI芯片是否正常工作 用于热启动不间断供电
 * 任意读取一块TI芯片的 INTERRUPT_ENABLE_REGISTER 寄存器值
 * 如果为 , 说明芯片从热启动不间断供电中正常恢复，不需要复位PSE芯片
 *
 * 返回值:
 *     TI芯片正常工作返回TRUE，否则返回FALSE
 */
bool poe_ssa_ti_chip_is_work_normal(uint8_t i2c_addr);

int poe_ssa_ti_parse_irq(ti_chip_irq_info_t ti_irq_tmp_info[]);

/* pm变化关闭所有端口上电 */
int poe_ssa_all_port_power_off(void);

/* pm变化重新打开所有端口检测 */
int poe_ssa_all_port_reopen(void);

/**
 * ssa_poe_ti_try_to_enable_port_det_cls - 努力使能打开端口的det/cls 
 * IN:
 * @lport: 逻辑端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int ssa_poe_ti_try_to_enable_port_det_cls(uint32_t lport);

/**
 * ssa_poe_ti_recover_chip_config - TI芯片恢复配置
 * @chip  芯片号
 *
 * TI芯片初始化
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - SS_E_FAIL
*/
int ssa_poe_ti_recover_chip_config(uint32_t chip);

int poe_ti_fac_set_port_onoff(int lport, bool mode);

int poe_ti_fac_check_port_overload(int lport, bool *is_overload);

int poe_ti_fac_set_port_max_power(int lport, int maxpower);

int poe_ssa_ti_init(void);

#endif
