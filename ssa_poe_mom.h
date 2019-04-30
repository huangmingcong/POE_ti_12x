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
 * ��ѯ�̵߳���ѯʱ������ 125ms
 * ���ȹ�С���������ȼ��ϵ���ʻ�����
 * ���ȹ��󣬶˿��ϵ�ʱ�������
*/                                                 /* ����ֵ */
#define TI_POLL_PRTICLE_PERIOD              (125000)  /* 0 */
#define TI_RESTART_DET_CLS_PERIOD           (0)
/* ��ѯ�̵߳Ķ�����ж����� 1 ��500 = 500ms */
#define TI_READ_DETCLS_IRQ_PERIOD           (4)
/* ��ѯ�߳��ϵ�����          3��500 = 1500ms */
#define TI_WRITE_POWER_ON_PERIOD            (5)
/* ��ѯ�̵߳Ķ��µ��ж����� 2 ��500 = 1s */
#define TI_READ_PGC_IRQ_PERIOD              (7)
/* ��ѯ�̸߳��¶˿���Ϣ */
#define TI_UPDATE_PORT_DATA_PERIOD          (6)
/* ��ѯ�̸߳��¶˿�led��          */
#define UPDATE_LED_MODE_PERIOD              (7)
/* ��ѯ�̸߳���cool downʱ�� */
#define TI_UPDATE_COOL_DOWN_PERIOD          (7)

/* ��ѯ�̼߳��ּ�ʹ������ 1s */
#define TI_DET_CLS_PERIOD           (8)

/* ÿ����ѯ���ڿ����ϵ�����˿��� */
#define POWER_UP_PORT_NUM           (4)

#define SSA_POE_TI_CHIP_INVALID(chip) ((chip) < 1 || (chip) > TI_CHIP_NUM)
#define SSA_POE_TI_CHIP_PORT_INVALID(chip_port) ((chip_port) < 1 || (chip_port) > 2)

/* ��оƬ�ź�оƬ�˿ںű��� */
#define SSA_POE_FOR_EACH_CHIP_AND_CHIP_PORT(chip, chip_port) \
    POE_SSA_FOR_EACH_CHIP(chip) \
        for ((chip_port) = 1; (chip_port) <= 2; (chip_port)++)

/* �����ȼ��ɵ͵��߱����˿ڣ�һ������PM�µ�  */
#define SSA_POE_FOR_EACH_LPORT_PRI_LOW_TO_HIGH(lport) \
        int _prio_; \
        for (_prio_ = POE_PORT_PRI_LOW; _prio_ >= POE_PORT_PRI_CRITICAL; _prio_--) \
            for ((lport) = POE_MAX_PORT; (lport) >= 1; (lport)--) \
                if (TI_PORT_CONFIG_PRIORITY(TI_LPORT_INFO(lport)) == _prio_)

/* �����ȼ��ɸߵ��ͱ����˿� */
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

/* ��ѯ���߳� */
void *poe_ti_main_thread(void *arg);
void power_up_test(void);
/* ���������µ�ʱ�ָ����� */
int poe_ssa_ti_ups_recover_data(void);

/* ͨ��˿���Ϣ */
int poe_port_status_update(int lport, int notify);

int ssa_poe_intf_conf_proc(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj);
int ssa_poe_sys_conf_proc(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj);

#endif
