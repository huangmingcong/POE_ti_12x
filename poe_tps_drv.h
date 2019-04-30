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

/* оƬ������ʱ�� �� */
#define TI_COMMAND_DELAY_SHORT              (2000)
/* оƬ������ʱ�� �� */
#define TI_COMMAND_DELAY_MEDIUM             (5000)
/* оƬ������ʱ�� �� */
#define TI_COMMAND_DELAY_LONG               (10000)

/* TPS23880оƬ�ж����� */
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

/* �ж϶�Ӧ�ĸ����¼� */
typedef enum {
    TI_IRQ_EVENT_NONE = 0,
    TI_SUPF_TSD,            /* �����µ��¼� */
    TI_SUPF_VPUV,           /* poe��ԴǷѹ�¼� */
    TI_SUPF_VDUV,           /* 3.3V��ԴǷѹ�¼� */
    TI_IFAULT_ILIM,         /* ILIM�¼� */
    TI_IFAULT_PCUT,         /* 2 pair PCUT�¼� */
    TI_STARTF,              /* ISTART�¼� */
    TI_DISF,                /* �Ͻ��¼� */
    TI_PGC_GOOD,            /* �ϵ��µ��¼� */
    TI_PGC_NO_GOOD,         /* �ϵ��µ��¼� */
    TI_PEC_ENABLE,          /* �ϵ��µ��¼� */
    TI_PEC_NO_ENABLE,       /* �ϵ��µ��¼� */
    TI_DETECT_CLSC,         /* �ּ��¼� */
    TI_DETECT_DET,          /* ����¼� */
    //TI_DETECT_LEGACY,     /* ��⵽�Ǳ�PD */
    TI_SUPF_OSSE,           /* OSSE �¼� */
    TI_SUPF_PCUT,           /* 4 pair �����¼� */
    TI_SUPF_RAMFLT,         /* sram�쳣�¼� */
    TI_IRQ_EVENT_MAX
} ti_irq_event_type_t;

typedef struct ti_port_irq_info_e {
    uint32_t port_event_num;
    ti_irq_event_type_t port_event[TI_IRQ_EVENT_MAX];
} ti_port_irq_info_t;

/* �ݴ�оƬ�ж���Ϣ�Ľṹ�� */
typedef struct poe_ssa_ti_chip_irq_info_e {
    ti_port_irq_info_t port_irq_info[2 + 1];    /* ÿ��i2c��ַ��������chip_port */
} ti_chip_irq_info_t;

#define POE_SSA_FOR_EACH_I2C(i2c_adr) for ((i2c_adr) = 0x20; (i2c_adr) <= 0x2b; (i2c_adr)++)
#define POE_SSA_FOR_EACH_CHIP(chip) for ((chip) = 1; (chip) <= 12; (chip)++)

/**
 * poe_ssa_ti_write_port_disconnect_enable - ���ö˿ڶϽ����ʹ��
 * IN:
 * @lport: �߼��˿ں�
 * @enable: trueΪʹ��falseΪʧ��
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_write_port_disconnect_enable(uint32_t lport, bool enable);

/**
 * poe_ssa_ti_write_port_operating_mode - ���ö˿ڲ���ģʽ
 * IN:
 * @lport: �߼��˿ں�
 * @opmode: Ҫ���õĲ���ģʽ
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_write_port_operating_mode(uint32_t lport, TPS238x_Operating_Modes_t opmode);

/**
 * poe_ssa_ti_write_port_detect_class_enable - ʹ�ܶ˿ڼ��
 * IN:
 * @lport: �߼��˿ں�
 * @enable: tureΪ��falseΪ�ر�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_write_port_detect_class_enable(uint32_t lport, bool enable);

/**
 * poe_ssa_ti_write_lport_reset - ��λ����
 * IN:
 * @lport: �߼��˿ں�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_write_lport_reset(uint32_t lport);

/**
 * ssa_poe_ti_write_port_power_enable - ʹ�ܶ˿��ϵ�
 * IN:
 * @lport: �߼��˿ں�
 * @pwon: tureΪ�ϵ�falseΪ�µ�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_write_port_power_enable(uint32_t lport, bool pwon);

/**
 * ssa_poe_ti_write_port_pcut_config - ���ö˿�Pcut
 * IN:
 * @lport: �߼��˿ں�
 * @icut: Ҫ���õ�icutö��
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_write_port_pcut_config(uint32_t lport, ti_pcut_ctrl_t pcut);

/**
 * ssa_poe_ti_read_port_pcut_config - ���ö˿�Pcut
 * IN:
 * @lport: �߼��˿ں�
 * @icut: Ҫ���õ�icutö��
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_read_port_pcut_config(uint32_t lport, ti_pcut_ctrl_t *pcut);

/**
 * ssa_poe_ti_read_port_pcut_config - ���˿�Icut
 * IN:
 * @lport: �߼��˿ں�
 * OUT:
 * @icut: icutö�ٶ���
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_port_pcut_enable(uint32_t lport, bool enable);

/**
 * poe_ssa_ti_write_port_force_on - ���ö˿�ǿ�ƹ���
 * IN:
 * @lport: �߼��˿ں�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_write_port_force_on(uint32_t lport);

/**
 * poe_ssa_ti_write_port_force_off - �رն˿�ǿ�ƹ���
 * IN:
 * @lport: �߼��˿ں�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_write_port_force_off(uint32_t lport);

/**
 * poe_ssa_ti_read_port_power_status - ��оƬ�˿ڹ���״̬
 * IN:
 * @lport: �߼��˿ں�
 * OUT:
 * @state: �˿ڹ���״̬,falseΪ������,trueΪ����
 *
 * ���˿ڹ���״̬ POWER_STATUS_REGISTER 10
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_read_port_power_status(uint32_t lport, bool *state);

/**
 * ssa_poe_ti_read_port_detect_class_status - ���˿�detect״̬
 * IN:
 * @lport: �߼��˿ں�
 * OUT:
 * @detect_status: оƬ�˿�detect״̬
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_read_port_detect_status(uint32_t lport, TPS238x_Detection_Status_t *detect_status);

/**
 * poe_ssa_ti_read_port_class_status - ���˿�class
 * IN:
 * @lport: �߼��˿ں�
 * OUT:
 * @class_status: оƬ�˿�clsaa״̬
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_read_port_class_status(uint32_t lport, int *port_class);

/**
 * poe_ssa_ti_get_port_measurements - ���˿ڵ���ֵ����ѹֵ, mA mV
 * IN:
 * @lport: �߼��˿ں�
 * OUT:
 * @current: оƬ�˿�current
 * @voltage: оƬ�˿�current
 *
 * Note: ��POWER_STATUS_REGISTER��Ӧ��PGnΪ1ʱ�����ܱ�֤��ȡ������ֵ��Ч
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_get_port_measurements(uint32_t lport, uint32_t *current, uint32_t *voltage);

/**
 * poe_ssa_ti_read_port_resistance - ���˿ڵ��裬k��
 * IN:
 * @lport: �߼��˿ں�
 * OUT:
 * @resistance: оƬ�˿�resistance,������NULL
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_read_port_resistance(uint32_t lport, uint32_t *resistance);

/**
 * poe_ssa_ti_get_port_power - ���˿����Ĺ��� mW
 * IN:
 * @lport: �߼��˿ں�
 * OUT:
 * @power: оƬ�˿�power
 *
 * Note: ��POWER_STATUS_REGISTER��Ӧ��PGnΪ1ʱ�����ܱ�֤��ȡ������ֵ��Ч
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_get_port_power(int lport, int *power);

/**
 * poe_ssa_ti_read_device_temperature - ��оƬ�¶�, ��
 * IN:
 * @chip: оƬ��
 * OUT:
 * @temp: оƬ�¶�,������NULL
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_read_device_temperature(uint32_t chip, uint32_t *temp);

/**
 * poe_ssa_ti_check_port_detect_class_enable - ���˿��Ƿ�det/clsʹ��
 * IN:
 * @lport: �߼��˿ں�
 * OUT:
 * @enable: tureΪ��falseΪ�ر�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int poe_ssa_ti_check_port_detect_class_enable(uint32_t lport, bool *enable);

/**
 * poe_ssa_ti_chip_is_work_normal - �ж�TIоƬ�Ƿ���������
 *
 * �ж�TIоƬ�Ƿ��������� ��������������Ϲ���
 * �����ȡһ��TIоƬ�� INTERRUPT_ENABLE_REGISTER �Ĵ���ֵ
 * ���Ϊ , ˵��оƬ������������Ϲ����������ָ�������Ҫ��λPSEоƬ
 *
 * ����ֵ:
 *     TIоƬ������������TRUE�����򷵻�FALSE
 */
bool poe_ssa_ti_chip_is_work_normal(uint8_t i2c_addr);

int poe_ssa_ti_parse_irq(ti_chip_irq_info_t ti_irq_tmp_info[]);

/* pm�仯�ر����ж˿��ϵ� */
int poe_ssa_all_port_power_off(void);

/* pm�仯���´����ж˿ڼ�� */
int poe_ssa_all_port_reopen(void);

/**
 * ssa_poe_ti_try_to_enable_port_det_cls - Ŭ��ʹ�ܴ򿪶˿ڵ�det/cls 
 * IN:
 * @lport: �߼��˿ں�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
 */
int ssa_poe_ti_try_to_enable_port_det_cls(uint32_t lport);

/**
 * ssa_poe_ti_recover_chip_config - TIоƬ�ָ�����
 * @chip  оƬ��
 *
 * TIоƬ��ʼ��
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - SS_E_FAIL
*/
int ssa_poe_ti_recover_chip_config(uint32_t chip);

int poe_ti_fac_set_port_onoff(int lport, bool mode);

int poe_ti_fac_check_port_overload(int lport, bool *is_overload);

int poe_ti_fac_set_port_max_power(int lport, int maxpower);

int poe_ssa_ti_init(void);

#endif
