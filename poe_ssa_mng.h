/*
 * Copyright(C) 2013 Ruijie Network. All rights reserved.
 */
/*
 * poe_ssa_mng.h
 * Original Author: zhongyunjing@ruijie.com.cn 2017-12
 *
 * 功率管理
 *
 */

#ifndef _POE_SSA_MNG_H_
#define _POE_SSA_MNG_H_

#include "poe_tps_cfg.h"
#include <tps238x.h>
#include "poe_db.h"
#include "poe_ssa_debug.h"
#include "ssa_poe_mom.h"
#include "poe_ssa_init.h"
#include "poe_tps_drv.h"

#define TI_PWON_INTERVALS                   (100000)    /* 上电间隔超过200ms会导致bug 413898 */
#define TI_FASTSHT_INTERVALS                (100000)

/* 两个1150p，740*2=1480W */
#define TI_POE_POWER_MAX                    (1480000)

/* 64位位图操作 */
#define POE_PBMP_PORT_CLEAR(pbm)            ((pbm) = 0ULL)
#define POE_PBMP_PORT_ADD(pbm, lport)       ((pbm) |= (1ULL << ((lport) - 1)))
#define POE_PBMP_PORT_DEL(pbm, lport)       ((pbm) &= ~(1ULL << ((lport) - 1)))
#define POE_PBMP_PORT_NOT_NULL(pbm)         ((pbm) != 0ULL)
#define POE_PBMP_PORT_TEST(pbm, lport)      ((pbm) & (1ULL << ((lport) - 1)))

/**
 * ssa_poe_ti_set_port_state - 更新端口状态
 * IN:
 * @port_info: 端口信息
 * @port_state: 端口状态
 * @off_reason: 端口下电原因
 * @need_notify: 是否要通告并更新led灯
 *
 * 返回值:
 *     void
*/
void ssa_poe_ti_set_port_state(ssa_poe_ti_port_info_t *port_info, ti_port_state_t port_state, 
    uint32_t off_reason, bool need_notify);

/**
 * ssa_poe_ti_recover_icut_ilim_config - 端口icut和ilim恢复默认值，包括芯片和软件
 * IN:
 * @chip: 芯片号
 * @chip_port: 芯片端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
*/
int ssa_poe_ti_recover_icut_ilim_def_config(uint32_t chip, uint32_t chip_port);

/**
 * ssa_poe_ti_set_icut_ilim_config - 设置端口icut和ilim值，包括芯片和软件
 * IN:
 * @chip: 芯片号
 * @chip_port: 芯片端口号
 * @icut: 要设置的icut枚举
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
*/
//int ssa_poe_ti_set_icut_ilim_config(uint32_t chip, uint32_t chip_port, ti_icut_ctrl_t icut);

/**
 * ssa_poe_ti_get_icut - 获取端口应该设置的icut值
 * IN:
 * @port_info: 芯片信息
 *
 * 返回值:
 *     icut枚举 - ti_icut_ctrl_t
*/
int ssa_poe_ti_get_icut(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_get_power_need - 获取端口应该分配的功率
 * IN:
 * @port_info: 芯片信息
 *
 * 返回值:
 *     power值
*/
uint32_t ssa_poe_ti_get_power_need(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_clear_port_power_info - 当主动下电的时候，把端口功率信息清空
 * IN:
 * @port_info: 芯片信息
 *
 * 返回值:
 *     void
*/
void ssa_poe_ti_clear_port_power_info(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_clear_port_detect_info - 当下电的时候，把端口一些数据清空
 * IN:
 * @port_info: 芯片信息
 *
 * 返回值:
 *     void
*/
void ssa_poe_ti_clear_port_detect_info(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_clear_port_i_count_info - 把端口过流计数清空
 * IN:
 * @port_info: 芯片信息
 *
 * 返回值:
 *     void
*/
void ssa_poe_ti_clear_port_i_count_info(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_update_system_power - 端口分配功率或消耗功率改变时，重新计算系统功率
 *
 * 返回值:无
*/
void ssa_poe_ti_update_system_power(void);

/**
 * ssa_poe_ti_port_power_down - 让端口下电
 * IN:
 * @chip: 芯片号
 * @chip_port: 芯片端口号
 *
 * note:让端口下电不用PWOFF命令，而是先关闭再打开
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
*/
int ssa_poe_ti_port_power_down(uint32_t chip, uint32_t chip_port);

/**
 * ssa_poe_ti_port_power_up - 如果允许，让端口上电
 * IN:
 * @chip: 芯片号
 * @chip_port: 芯片端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
*/
int ssa_poe_ti_port_power_up(uint32_t chip, uint32_t chip_port);

/**
 * ssa_poe_ti_mng_init - TI芯片驱动功率管理初始化函数
 *
 * return: 成功SS_E_NONE, 失败返回负数
*/
int ssa_poe_ti_mng_init(void);

#endif /* _SSA_POE_TPS23861_MNG_H_ */
