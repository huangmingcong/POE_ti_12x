#ifndef _SSA_POE_MOM_H_
#define _SSA_POE_MOM_H_

/* rg_mom */
#include <libpub/rg_thread/rg_thread.h>
#include <libpub/rg_mom/rg_mom.h>
#include <libpub/rg_mom/rg_global.h>
#include <libpub/rg_mom/rg_mom_common.h>
#include <libpub/rg_mom/rg_mom_sync.h>

#include "app_proc.h"
#include "app_mom.h"
#include "obj_udp_poe.h"
#include "obj_intf_poe.h"
#include "obj_sys_poe.h"
#include "poe_ssa_debug.h"
#include "app_pipe.h"

/* 
 * 轮询线程的轮询时间粒度 125ms
 * 粒度过小，不按优先级上电概率会增大
 * 粒度过大，端口上电时间会增大
*/                                                 /* 修正值 */
#define TI_POLL_PRTICLE_PERIOD              (125000)  /* 0 */
#define TI_RESTART_DET_CLS_PERIOD           (0)
/* 轮询线程的读检测中断周期 1 ×500 = 500ms */
#define TI_READ_DETCLS_IRQ_PERIOD           (4)
/* 轮询线程上电周期          3×500 = 1500ms */
#define TI_WRITE_POWER_ON_PERIOD            (5)
/* 轮询线程的读下电中断周期 2 ×500 = 1s */
#define TI_READ_PGC_IRQ_PERIOD              (7)
/* 轮询线程更新端口信息 */
#define TI_UPDATE_PORT_DATA_PERIOD          (6)
/* 轮询线程更新端口led灯          */
#define UPDATE_LED_MODE_PERIOD              (7)
/* 轮询线程更新cool down时间 */
#define TI_UPDATE_COOL_DOWN_PERIOD          (7)

/* 轮询线程检测分级使能周期 1s */
#define TI_DET_CLS_PERIOD           (8)

/* 每个轮询周期可以上电的最大端口数 */
#define POWER_UP_PORT_NUM           (4)

#define SSA_POE_TI_CHIP_INVALID(chip) ((chip) < 1 || (chip) > TI_CHIP_NUM)
#define SSA_POE_TI_CHIP_PORT_INVALID(chip_port) ((chip_port) < 1 || (chip_port) > 2)

/* 按芯片号和芯片端口号遍历 */
#define SSA_POE_FOR_EACH_CHIP_AND_CHIP_PORT(chip, chip_port) \
    POE_SSA_FOR_EACH_CHIP(chip) \
        for ((chip_port) = 1; (chip_port) <= 2; (chip_port)++)

/* 按优先级由低到高遍历端口，一般用于PM下电  */
#define SSA_POE_FOR_EACH_LPORT_PRI_LOW_TO_HIGH(lport) \
        int _prio_; \
        for (_prio_ = POE_PORT_PRI_LOW; _prio_ >= POE_PORT_PRI_CRITICAL; _prio_--) \
            for ((lport) = POE_MAX_PORT; (lport) >= 1; (lport)--) \
                if (TI_PORT_CONFIG_PRIORITY(TI_LPORT_INFO(lport)) == _prio_)

/* 按优先级由高到低遍历端口 */
#define SSA_POE_FOR_EACH_LPORT_PRI_HIGH_TO_LOW(lport) \
        uint32_t _prio_; \
        for (_prio_ = POE_PORT_PRI_CRITICAL; _prio_ <= POE_PORT_PRI_LOW; _prio_++) \
            for ((lport) = 1; (lport) <= POE_MAX_PORT; (lport)++) \
                if (TI_PORT_CONFIG_PRIORITY(TI_LPORT_INFO(lport)) == _prio_)

typedef enum {
    TI_RESTART_DET_CLS,
    TI_READ_DETCLS_IRQ,
    TI_UPDATE_PORT_DATA,
    TI_UPDATE_COOL_DOWN,
    UPDATE_LED_MODE,
    TI_TASK_MAX
} ti_thread_task_t;

typedef struct ti_main_thread_task_info_e {
    ti_thread_task_t task_type;
    uint32_t task_period;
    int (*task_func)(void);
} ti_main_thread_task_info_t;

extern app_info_t *poe_ssa_get_app_info(void);
extern app_mom_t *poe_ssa_get_app_mom(void);
extern pipe_obj_t *poe_ssa_get_app_pipe(void);

int poe_port_status_handler_init(void);
int poe_ssa_irq_handle(void);

/* 轮询主线程 */
void *poe_ti_main_thread(void *arg);
void power_up_test(void);
/* 热重启不下电时恢复数据 */
int poe_ssa_ti_ups_recover_data(void);

/* 通告端口信息 */
int poe_port_status_update(int lport, int notify);

int ssa_poe_intf_conf_proc(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj);
int ssa_poe_sys_conf_proc(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj);

#endif
