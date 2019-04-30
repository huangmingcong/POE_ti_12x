/*
 * Copyright(C) 2013 Ruijie Network. All rights reserved.
 */
/*
 * poe_ssa_mng.h
 * Original Author: zhongyunjing@ruijie.com.cn 2017-12
 *
 * ���ʹ���
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

#define TI_PWON_INTERVALS                   (100000)    /* �ϵ�������200ms�ᵼ��bug 413898 */
#define TI_FASTSHT_INTERVALS                (100000)

/* ����1150p��740*2=1480W */
#define TI_POE_POWER_MAX                    (1480000)

/* 64λλͼ���� */
#define POE_PBMP_PORT_CLEAR(pbm)            ((pbm) = 0ULL)
#define POE_PBMP_PORT_ADD(pbm, lport)       ((pbm) |= (1ULL << ((lport) - 1)))
#define POE_PBMP_PORT_DEL(pbm, lport)       ((pbm) &= ~(1ULL << ((lport) - 1)))
#define POE_PBMP_PORT_NOT_NULL(pbm)         ((pbm) != 0ULL)
#define POE_PBMP_PORT_TEST(pbm, lport)      ((pbm) & (1ULL << ((lport) - 1)))

/**
 * ssa_poe_ti_set_port_state - ���¶˿�״̬
 * IN:
 * @port_info: �˿���Ϣ
 * @port_state: �˿�״̬
 * @off_reason: �˿��µ�ԭ��
 * @need_notify: �Ƿ�Ҫͨ�沢����led��
 *
 * ����ֵ:
 *     void
*/
void ssa_poe_ti_set_port_state(ssa_poe_ti_port_info_t *port_info, ti_port_state_t port_state, 
    uint32_t off_reason, bool need_notify);

/**
 * ssa_poe_ti_recover_icut_ilim_config - �˿�icut��ilim�ָ�Ĭ��ֵ������оƬ�����
 * IN:
 * @chip: оƬ��
 * @chip_port: оƬ�˿ں�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
*/
int ssa_poe_ti_recover_icut_ilim_def_config(uint32_t chip, uint32_t chip_port);

/**
 * ssa_poe_ti_set_icut_ilim_config - ���ö˿�icut��ilimֵ������оƬ�����
 * IN:
 * @chip: оƬ��
 * @chip_port: оƬ�˿ں�
 * @icut: Ҫ���õ�icutö��
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
*/
//int ssa_poe_ti_set_icut_ilim_config(uint32_t chip, uint32_t chip_port, ti_icut_ctrl_t icut);

/**
 * ssa_poe_ti_get_icut - ��ȡ�˿�Ӧ�����õ�icutֵ
 * IN:
 * @port_info: оƬ��Ϣ
 *
 * ����ֵ:
 *     icutö�� - ti_icut_ctrl_t
*/
int ssa_poe_ti_get_icut(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_get_power_need - ��ȡ�˿�Ӧ�÷���Ĺ���
 * IN:
 * @port_info: оƬ��Ϣ
 *
 * ����ֵ:
 *     powerֵ
*/
uint32_t ssa_poe_ti_get_power_need(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_clear_port_power_info - �������µ��ʱ�򣬰Ѷ˿ڹ�����Ϣ���
 * IN:
 * @port_info: оƬ��Ϣ
 *
 * ����ֵ:
 *     void
*/
void ssa_poe_ti_clear_port_power_info(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_clear_port_detect_info - ���µ��ʱ�򣬰Ѷ˿�һЩ�������
 * IN:
 * @port_info: оƬ��Ϣ
 *
 * ����ֵ:
 *     void
*/
void ssa_poe_ti_clear_port_detect_info(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_clear_port_i_count_info - �Ѷ˿ڹ����������
 * IN:
 * @port_info: оƬ��Ϣ
 *
 * ����ֵ:
 *     void
*/
void ssa_poe_ti_clear_port_i_count_info(ssa_poe_ti_port_info_t *port_info);

/**
 * ssa_poe_ti_update_system_power - �˿ڷ��书�ʻ����Ĺ��ʸı�ʱ�����¼���ϵͳ����
 *
 * ����ֵ:��
*/
void ssa_poe_ti_update_system_power(void);

/**
 * ssa_poe_ti_port_power_down - �ö˿��µ�
 * IN:
 * @chip: оƬ��
 * @chip_port: оƬ�˿ں�
 *
 * note:�ö˿��µ粻��PWOFF��������ȹر��ٴ�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
*/
int ssa_poe_ti_port_power_down(uint32_t chip, uint32_t chip_port);

/**
 * ssa_poe_ti_port_power_up - ��������ö˿��ϵ�
 * IN:
 * @chip: оƬ��
 * @chip_port: оƬ�˿ں�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
*/
int ssa_poe_ti_port_power_up(uint32_t chip, uint32_t chip_port);

/**
 * ssa_poe_ti_mng_init - TIоƬ�������ʹ����ʼ������
 *
 * return: �ɹ�SS_E_NONE, ʧ�ܷ��ظ���
*/
int ssa_poe_ti_mng_init(void);

#endif /* _SSA_POE_TPS23861_MNG_H_ */
