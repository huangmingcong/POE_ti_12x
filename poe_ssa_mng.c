/*
 * Copyright(C) 2013 Ruijie Network. All rights reserved.
 */
/*
 * ssa_poe_tps23861_mng.c
 * Original Author: zhongyunjing@ruijie.com.cn 2017-12
 *
 * ���ʹ���
 *
 */

#include "poe_ssa_mng.h"

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
    uint32_t off_reason, bool need_notify)
{
    uint32_t lport;
    int rv;
    
    /* �ڲ�ʹ�ã��������� */
    
    TI_PORT_CTRL_STATE(port_info) = port_state;
    TI_PORT_CTRL_OFF_REASON(port_info) = off_reason;

    if (need_notify) {
        lport = TI_PORT_CTRL_LPORT(port_info);
        //ssa_poe_push(POE_PORT_SINGLE_INFO, lport);
        //ssa_poe_ti_led_update(TI_GET_CHIPID(lport), TI_GET_CHIP_PORT(lport));
    }
    
    return;
}

/* class4����type1��type2��ȡ���� */
static uint32_t ssa_poe_ti_get_class4_power(ssa_poe_ti_port_info_t *port_info)
{
    uint32_t power;
    /* XXX ��ʱֻ�н���֧��lldp */
    if (TI_SYS_LLDP_EN && TI_SYS_PM_MODE == POE_PM_ENERGYSAVE) {
        switch (TI_PORT_CTRL_LLDP_PDTYPE(port_info)) {
        case POE_LLDP_PDTYPE_1:
            power = 14000;
            break;
        case POE_LLDP_PDTYPE_2:
            power = 30000;
            break;
        default:
            power = 30000;
            break;
        }
    } else {
        power = 30000;
    }
    
    return power;
}

/* ����class�����ȡ���� */
uint32_t ssa_poe_ti_get_power_need(ssa_poe_ti_port_info_t *port_info)
{
    uint32_t power;

    switch (TI_PORT_DATA_CLS_STATUS(port_info)) {
    case PD_CLASS_0:
    case PD_CLASS_3:
        power = 14000;
        break;
    case PD_CLASS_1:
        power = 4000;
        break;
    case PD_CLASS_2:
        power = 6700;
        break;
    case PD_CLASS_4:
        power = ssa_poe_ti_get_class4_power(port_info);
        break;
    case PD_CLASS_4PLUS_TYPE1:
        power = 14000;
        break;
    case PD_CLASS_5:
    case PD_CLASS4_4P_DUAL:
        power = 45000;
        break;
    case PD_CLASS_6:
        power = 60000;
        break;
    case PD_CLASS_7:
        power = 75000;
        break;
    case PD_CLASS_8:
        power = 90000;
        break;
    default:
        /* TI_CLASS_OVERCURRENT��TI_CLASS_MISMATCH */
        power = 0;
        break;
    }

    return power;
}

#if 0 /* ����pcut */
static int ssa_poe_ti_get_class4_icut(ssa_poe_ti_port_info_t *port_info)
{
    int icut_val;

    /* XXX ��ʱֻ�н���ģʽ֧��lldp */
    if (TI_SYS_LLDP_EN && TI_SYS_PM_MODE == POE_PM_ENERGYSAVE) {
        switch (TI_PORT_CTRL_LLDP_PDTYPE(port_info)) {
        case POE_LLDP_PDTYPE_1:
            icut_val = PD_CLASS_4_TYPE1_ICUT;
            break;
        case POE_LLDP_PDTYPE_2:
            icut_val = PD_CLASS_4_TYPE2_ICUT;
            break;
        default:
            icut_val = PD_CLASS_4_TYPE2_ICUT;
            break;
        }
    } else {
        icut_val = PD_CLASS_4_ICUT;
    }

    return icut_val;
}

/* ����port�����ȡicut */
int ssa_poe_ti_get_icut(ssa_poe_ti_port_info_t *port_info)
{
    int icut_val;

    if (port_info == NULL) {
        DBG_TI_ERR("port is invalid!!!\n");
        return -SS_E_PARAM;
    }

    /* XXX support icut */
    if (TI_PORT_CTRL_SUPPORT_ICUT(port_info) != POE_SUPPORT_ICUT_DEFAULT) {
        icut_val = TI_PORT_CTRL_SUPPORT_ICUT(port_info);
        return icut_val;
    }

    /* ͬ����ǰ��Ŀ��bug 201866, 202120 �ھ�̬ģʽ�Ͷ˿������������ʱ��icutҪ��Ϊ���ֵ */
    if (TI_SYS_PM_MODE == POE_PM_STATIC 
        || TI_PORT_CONFIG_MAX_POWER(port_info) != SSA_POE_MAXPOWER_DEF) {
        icut_val = TI_ICUT_MAX;
        return icut_val;
    }

    switch (TI_PORT_DATA_CLS_STATUS(port_info)) {
    case TI_CLASS_0:
        icut_val = PD_CLASS_0_ICUT;
        break;
    case TI_CLASS_1:
        icut_val = PD_CLASS_1_ICUT;
        break;
    case TI_CLASS_2:
        icut_val = PD_CLASS_2_ICUT;
        break;
    case TI_CLASS_3:
        icut_val = PD_CLASS_3_ICUT;
        break;
    case TI_CLASS_4:
        icut_val = ssa_poe_ti_get_class4_icut(port_info);
        break;
    default:
        icut_val = PD_CLASS_0_ICUT;
        break;
    }

    return icut_val;
}
#endif
/* �������µ��ʱ�򣬰Ѷ˿ڹ�����Ϣ��� */
void ssa_poe_ti_clear_port_power_info(ssa_poe_ti_port_info_t *port_info)
{
    /* �ڲ�ʹ�ã��������� */
    
    TI_PORT_CTRL_ALLOC_POWER(port_info)   = 0;
    TI_PORT_DATA_CURRENT(port_info)       = 0;
    TI_PORT_DATA_VOLTAGE(port_info)       = 0;
    TI_PORT_DATA_CONSUME_POWER(port_info) = 0;

    return;
}

/* ���µ��ʱ�򣬰Ѷ˿�һЩ������� */
void ssa_poe_ti_clear_port_detect_info(ssa_poe_ti_port_info_t *port_info)
{
    /* �ڲ�ʹ�ã��������� */
    
    TI_PORT_CTRL_LLDP_PDTYPE(port_info) = POE_LLDP_PDTYPE_CLASS;
    TI_PORT_DATA_DET_STATUS(port_info)  = DETECT_UNKNOWN;
    TI_PORT_DATA_CLS_STATUS(port_info)  = PD_CLASS_UNKNOWN;

    return;
}

/* �Ѷ˿ڹ���������� */
void ssa_poe_ti_clear_port_i_count_info(ssa_poe_ti_port_info_t *port_info)
{
    /* �ڲ�ʹ�ã��������� */
    
    TI_PORT_CTRL_ICUT_COUNT(port_info)  = 0;
    TI_PORT_CTRL_ILIM_COUNT(port_info)  = 0;
    TI_PORT_CTRL_ISTRT_COUNT(port_info) = 0;

    return;
}
#if 0
/**
 * ssa_poe_ti_recover_icut_ilim_config - �˿�icut��ilim�ָ�Ĭ��ֵ������оƬ�����
 * IN:
 * @chip: оƬ��
 * @chip_port: оƬ�˿ں�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
*/
int ssa_poe_ti_recover_icut_ilim_def_config(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    if (SSA_POE_TI_CHIP_INVALID(chip) || SSA_POE_TI_CHIP_PORT_INVALID(chip_port)) {
        POE_DRV_ZLOG_ERROR("params err, chip = %d, chip_port = %d\n", chip, chip_port);
        return -POE_E_FAIL;
    }

    port_info = TI_PORT_INFO(chip, chip_port);

    /* ���ԭ����class4�ϵ�ģ�Ҫ�ָ�plus�Ĵ�����ʹ��1 ��ilim */
    usleep(TI_COMMAND_DELAY_SHORT);
    rv = ssa_poe_ti_write_port_poe_plus_enable(chip, chip_port, false);
    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_write_port_poe_plus_enable failed, rv = %d\n", rv);
        return rv;
    }
    
    /* Icut�ָ�Ĭ��ֵ */
    if (TI_PORT_CTRL_ICUT(port_info) != TI_ICUT_374) {
        usleep(TI_COMMAND_DELAY_SHORT);
        rv = ssa_poe_ti_write_port_icut_config(chip, chip_port, TI_ICUT_374);
        if (rv != POE_E_NONE) {
            DBG_TI_ERR("ssa_poe_ti_write_port_icut_config failed, rv = %d\n", rv);
            return rv;
        }
    
        TI_PORT_CTRL_ICUT(port_info) = TI_ICUT_374;
    }
    
    return POE_E_NONE;
}

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
int ssa_poe_ti_set_icut_ilim_config(uint32_t chip, uint32_t chip_port, ti_icut_ctrl_t icut)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;

    DBG_TI_MNG("Chip-port[%d, %d], icut = %d\n", chip, chip_port, icut);
    
    if (SSA_POE_TI_CHIP_INVALID(chip) || SSA_POE_TI_CHIP_PORT_INVALID(chip_port) 
        || (icut < 0) || (icut > TI_ICUT_920)) {
        DBG_TI_ERR("params err, chip = %d, chip_port = %d, Icut = %d\n", chip, chip_port, icut);
        return -SS_E_PARAM;
    }

    port_info = TI_PORT_INFO(chip, chip_port);

    if (icut == TI_PORT_CTRL_ICUT(port_info)) {
        return POE_E_NONE;
    }

    /* ����icutֵ */
    usleep(TI_COMMAND_DELAY_SHORT);
    rv = ssa_poe_ti_write_port_icut_config(chip, chip_port, icut);
    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_write_port_icut_config failed, rv = %d\n", rv);
        return rv;
    }

    TI_PORT_CTRL_ICUT(port_info) = icut;

    /* �����class4��Ҫ������plus�Ĵ�����ʹ��2 ��ilim */
    usleep(TI_COMMAND_DELAY_SHORT);
    if (icut > TI_ICUT_374_AGAIN) {
        rv = ssa_poe_ti_write_port_poe_plus_enable(chip, chip_port, true);
    } else {
        rv = ssa_poe_ti_write_port_poe_plus_enable(chip, chip_port, false);
    }

    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_write_port_poe_plus_enable failed, rv = %d\n", rv);
        return rv;
    }

    return POE_E_NONE;
}
#endif
/* �˿��ܷ��ϵ��ж� */
static bool ssa_poe_ti_pwr_up_check(ssa_poe_ti_port_info_t *port_info)
{
    POE_DRV_ZLOG_WARN("\n");

    /* ȫ�ֹ���رգ����ϵ� */
    if (!TI_SYS_GLB_POE_EN) {
        POE_DRV_ZLOG_WARN("PoE no global enable!\n");
        return false;
    }

    /* 
     * �˿ڹر��˲��ϵ硣
    */
    if (!TI_PORT_CONFIG_POE_EN(port_info)) {
        POE_DRV_ZLOG_WARN("Port no enable!\n");
        return false;
    }

    /* ����ǿ�ƹ�����һֱ���ڹ���״̬�����Բ����ϵ� */
    if (TI_PORT_CONFIG_FORCEON_EN(port_info)) {
        POE_DRV_ZLOG_WARN("Port is force on!\n");
        return false;
    }

    /* �Ѿ��ϵ�Ķ˿ڲ��ϵ� */
    if (TI_PORT_CTRL_STATE(port_info) == TI_PORT_ON) {
        POE_DRV_ZLOG_WARN("Port is on!\n");
        return false;
    }

    /* �����ϵ籣����ȴ�Ķ˿ڲ��ϵ磬����icut/ilim/istart/temp�� */
    if (TI_PORT_CTRL_COOL_DOWN_TIME(port_info) != 0 || TI_SYS_COOL_DOWN_TIME != 0) {
        POE_DRV_ZLOG_WARN("Port is in cool-down period!\n");
        return false;
    }
#if 0
    /* ��̬ģʽ�Ҷ˿ڹ̶����书��Ϊ0���ϵ� */
    if (TI_SYS_PM_MODE == POE_PM_STATIC && TI_PORT_CONFIG_STATIC_POWER(port_info) == 0) {
        DBG_TI_MNG("Port's static alloc power is zero!\n");
        return false;
    }
#endif
    /*
     * �Ǿ�̬ģʽ�Ҷ˿������Ϊ0���ϵ�
     * ����ʼ�ʹС�ڷּ����ʣ�Ҳ�����ϵ磬������ssc����
    */
    if (TI_SYS_PM_MODE != POE_PM_STATIC && TI_PORT_CONFIG_MAX_POWER(port_info) == 0) {
        POE_DRV_ZLOG_WARN("Port's max power is zero!\n");
        return false;
    }

    return true;
}

static void ssa_poe_ti_update_syspwr_at_auto(void)
{
    uint32_t chip;
    uint32_t chip_port;
    ssa_poe_ti_port_info_t *port_info;

    TI_SYS_CONSUME_POWER = 0;
    TI_SYS_ALLOC_POWER = 0;

    SSA_POE_FOR_EACH_CHIP_AND_CHIP_PORT(chip, chip_port) {
        port_info = TI_PORT_INFO(chip, chip_port);
        TI_SYS_CONSUME_POWER += TI_PORT_DATA_CONSUME_POWER(port_info);
        TI_SYS_ALLOC_POWER += TI_PORT_CTRL_ALLOC_POWER(port_info);
    }

    if (TI_SYS_ALLOC_POWER > TI_SYS_POE_TOTAL_POWER) {
        TI_SYS_REMAIN_POWER = 0;
    } else {
        TI_SYS_REMAIN_POWER = TI_SYS_POE_TOTAL_POWER - TI_SYS_ALLOC_POWER;
    }

    return;
}

static void ssa_poe_ti_update_syspwr_at_energysave(void)
{
    uint32_t chip;
    uint32_t chip_port;
    ssa_poe_ti_port_info_t *port_info;

    TI_SYS_CONSUME_POWER = 0;
    TI_SYS_ALLOC_POWER = 0;
    
    SSA_POE_FOR_EACH_CHIP_AND_CHIP_PORT(chip, chip_port) {
        port_info = TI_PORT_INFO(chip, chip_port);
        TI_SYS_CONSUME_POWER += TI_PORT_DATA_CONSUME_POWER(port_info);
        TI_SYS_ALLOC_POWER += TI_PORT_CTRL_ALLOC_POWER(port_info);
    }
    
    if (TI_SYS_ALLOC_POWER > (TI_SYS_POE_TOTAL_POWER - TI_SYS_RESERVE_POWER)) {
        TI_SYS_REMAIN_POWER = 0;
    } else {
        TI_SYS_REMAIN_POWER = TI_SYS_POE_TOTAL_POWER - TI_SYS_ALLOC_POWER - TI_SYS_RESERVE_POWER;
    }

    return;
}

static void ssa_poe_ti_update_syspwr_at_static(void)
{
    uint32_t lport;
    ssa_poe_ti_port_info_t *port_info;
    uint32_t chip;
    uint32_t chip_port;
    int rv;

    TI_SYS_CONSUME_POWER = 0;
    TI_SYS_ALLOC_POWER = 0;
    
    SSA_POE_FOR_EACH_LPORT(lport) {
        port_info = TI_LPORT_INFO(lport);
        /* 
           ���Pd�ּ�����>�˿ڷ��书�ʣ���Pdʵʱ����<�˿ڷ��书������£�Ϊ�˶˿ڹ��硣
        */
        if (TI_PORT_DATA_CONSUME_POWER(port_info) > TI_PORT_CONFIG_STATIC_POWER(port_info) 
                && TI_PORT_CTRL_PWON(port_info)) {
            chip = TI_GET_CHIPID(lport);
            chip_port = TI_GET_CHIP_PORT(lport);
            rv = ssa_poe_ti_port_power_down(chip, chip_port);
            if (rv != POE_E_NONE) {
                printf("ssa_poe_ti_port_power_down failed, rv = %d\n", rv);
                continue;
            }
        }
                
        TI_SYS_CONSUME_POWER += TI_PORT_DATA_CONSUME_POWER(port_info);
        TI_SYS_ALLOC_POWER += TI_PORT_CONFIG_STATIC_POWER(port_info);
        if (TI_SYS_ALLOC_POWER > TI_SYS_POE_TOTAL_POWER) {
            TI_PORT_CTRL_ALLOC_POWER(port_info) = 0;
        } else {
            TI_PORT_CTRL_ALLOC_POWER(port_info) = TI_PORT_CONFIG_STATIC_POWER(port_info);
        }
    }

    if (TI_SYS_ALLOC_POWER > TI_SYS_POE_TOTAL_POWER) {
        TI_SYS_REMAIN_POWER = 0;
    } else {
        TI_SYS_REMAIN_POWER = TI_SYS_POE_TOTAL_POWER - TI_SYS_ALLOC_POWER;
    }

    return;
}

/**
 * ssa_poe_ti_update_system_power - �˿ڷ��书�ʻ����Ĺ��ʸı�ʱ�����¼���ϵͳ����
 *
 * ����ֵ:��
*/
void ssa_poe_ti_update_system_power(void)
{
    if (TI_SYS_PM_MODE == POE_PM_AUTO) {
        ssa_poe_ti_update_syspwr_at_auto();
    } else if (TI_SYS_PM_MODE == POE_PM_ENERGYSAVE) {
        ssa_poe_ti_update_syspwr_at_energysave();
    } else {
        ssa_poe_ti_update_syspwr_at_static();
    }
    POE_DRV_ZLOG_WARN("Cur tot-pwr = %dmW, rem-pwr = %dmW, alc-pwr = %dmW, cons-pwr = %dmW\n", 
        TI_SYS_POE_TOTAL_POWER, TI_SYS_REMAIN_POWER, TI_SYS_ALLOC_POWER, TI_SYS_CONSUME_POWER);

    return;
}

/* ���ʣ�๦��������ö˿��ϵ� */
static bool ssa_poe_ti_remain_pwr_enough_check(uint32_t need_power)
{
#if 0 /* ���ܾ�̬ģʽ�� */
    if (TI_SYS_PM_MODE != POE_PM_STATIC) {
        /* �ϵ繦�����������ʣ���� */
        if (need_power > TI_SYS_REMAIN_POWER) {
            return false;
        }
    } else {
        /* ��̬ģʽ�˿ڹ��ʹ̶���Ϊ0��ǰ���Ѿ����˵��ˣ���Ϊ0���������ϵ磬������ssc���� */
        return true;
    }

#endif
    POE_DRV_ZLOG_INFO("remain power = %d, need_power = %d\n", TI_SYS_REMAIN_POWER, need_power);

    /* �ϵ繦�����������ʣ���� */
    if (need_power > TI_SYS_REMAIN_POWER) {
        return false;
    }
    
    return true;
}

uint32_t ssa_ti_pwon_intervals = TI_PWON_INTERVALS;

static int ssa_poe_ti_exec_port_power_up(uint32_t chip, uint32_t chip_port)
{
    int rv;
    bool pg;
    ssa_poe_ti_port_info_t *port_info;
    int need_icut;
    
    POE_DRV_ZLOG_WARN("[ssa_poe_ti_exec_port_power_up]Chip-port[%d, %d]\n", chip, chip_port);
    
    port_info = TI_PORT_INFO(chip, chip_port);
#if 0
    /*
     * ����icut/ilimֵ
    */
    need_icut = ssa_poe_ti_get_icut(port_info);
    rv = ssa_poe_ti_set_icut_ilim_config(chip, chip_port, need_icut);
    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_set_icut_ilim_config failed, rv = %d\n", rv);
        return rv;
    }
#endif
    rv = poe_ssa_ti_read_port_power_status(TI_PORT_CTRL_LPORT(port_info), &pg);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_check_port_power_good failed, rv = %d\n", rv);
        pg = false;
    }

    if (!pg) {
        usleep(TI_COMMAND_DELAY_SHORT);
        /* power up */
        rv = poe_ssa_ti_write_port_power_enable(TI_PORT_CTRL_LPORT(port_info), true);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_write_port_power_enable failed, rv = %d\n", rv);
            return rv;
        }
    }
    /* �ϵ��� */
    usleep(ssa_ti_pwon_intervals);

    return POE_E_NONE;
}

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
int ssa_poe_ti_port_power_down(uint32_t chip, uint32_t chip_port)
{
    int rv;
    //ti_opmode_t opmode;
    bool pg;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    if (SSA_POE_TI_CHIP_INVALID(chip) || SSA_POE_TI_CHIP_PORT_INVALID(chip_port)) {
        POE_DRV_ZLOG_ERROR("chip or chip_port invalid, chip = %d, chip_port = %d\n", chip, chip_port);
        return -POE_E_FAIL;
    }

    port_info = TI_PORT_INFO(chip, chip_port);
    
    /* ǿ�ƹ������ʱ���� */
    if (TI_PORT_CONFIG_FORCEON_EN(port_info)) {
        POE_DRV_ZLOG_WARN("Chip-port[%d, %d] is Force-On\n", chip, chip_port);
        return POE_E_NONE;
    }
    
    rv = poe_ssa_ti_read_port_power_status(TI_PORT_CTRL_LPORT(port_info), &pg);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_WARN("ssa_poe_ti_check_port_power_good failed, rv = %d\n", rv);
        pg = false;
    }
    
    /* PGΪ0�����ܽ���power off���� */
    if (pg) {
        usleep(TI_COMMAND_DELAY_MEDIUM);
        rv = poe_ssa_ti_write_port_power_enable(TI_PORT_CTRL_LPORT(port_info), false);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_write_port_power_enable failed, rv = %d\n", rv);
            return rv;
        }
    } else {
        POE_DRV_ZLOG_WARN("Chip-port[%d, %d] is power no good!!!\n", chip, chip_port);
        return POE_E_NONE;
    }

    /* 
     * OFFģʽ�Ķ˿ڣ����detect/class enable�Ĵ���Ҳ��գ�����Ҫ���´�detect/class
     * OFFģʽ�е�semiauto��manualģʽ��Ȼ���detect/class enable����Ҫ��ʱ1.2ms
    */
    if (!TI_PORT_CONFIG_LEGACY_EN(port_info)) {
        rv = poe_ssa_ti_write_port_detect_class_enable(TI_PORT_CTRL_LPORT(port_info), true);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_try_to_enable_port_det_cls failed, rv = %d\n", rv);
            return rv;
        }
    /* �Ǳ�Ҳ���ʹ��һ�� */
    } else {
        usleep(TI_COMMAND_DELAY_SHORT);
        rv = poe_ssa_ti_write_port_detect_class_enable(TI_PORT_CTRL_LPORT(port_info), true);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_write_port_detect_class_enable failed, rv = %d\n", rv);
            return rv;
        }
    }
    
    return POE_E_NONE;
}

static int ssa_poe_ti_port_power_up_energy_auto(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    ssa_poe_ti_port_info_t *off_port_info;
    uint32_t port_need_power;
    uint32_t off_lport;
    
    POE_DRV_ZLOG_WARN("[ssa_poe_ti_port_power_up_energy_auto]\n");

    /* 
     * �������ǽ��ܻ��Զ�ģʽ�Ĵ��� 
    */
    port_info = TI_PORT_INFO(chip, chip_port);
    port_need_power = ssa_poe_ti_get_power_need(port_info);
    if (port_need_power == 0) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_get_power_need failed\n");
        return -POE_E_FAIL;
    }

    SSA_POE_FOR_EACH_LPORT_PRI_LOW_TO_HIGH(off_lport) {
        /* ���ʳ��㣬�����ϵ� */
        if (ssa_poe_ti_remain_pwr_enough_check(port_need_power)) {
            rv = ssa_poe_ti_exec_port_power_up(chip, chip_port);
            if (rv != POE_E_NONE) {
                POE_DRV_ZLOG_ERROR("ssa_poe_ti_exec_port_power_up failed, rv = %d\n", rv);
                return rv;
            }

            /* ���书�ʸ��� */
            TI_PORT_CTRL_ALLOC_POWER(port_info) = port_need_power;

            /* �ϵ��ͨ�棬��֤�����ȼ�˳��ͨ�� */
            ssa_poe_ti_set_port_state(port_info, TI_PORT_ON, POE_NORMAL, true);
            TI_PORT_DATA_POWER_UP(port_info) = true;
            rv = poe_port_status_update(TI_PORT_CTRL_LPORT(port_info), true);
            if (rv != 0) {
                POE_DRV_ZLOG_ERROR("poe_port_status_update fail. rv = %d\n", rv);
            }
            POE_DRV_ZLOG_WARN(" ****** lport : %d power up\n", TI_PORT_CTRL_LPORT(port_info));
            /* �˿ڹ����б䣬ϵͳ����ҲҪ���� */
            ssa_poe_ti_update_system_power();
                        
            return POE_E_NONE;
        }
        /* 
         * ���ʲ��㣬�����ȼ���͵Ķ˿������µ�
        */
        off_port_info = TI_LPORT_INFO(off_lport);

        /* ������ϵ�˿����ȼ�������������ȼ��˿����ȼ�����ֱ�ӷ������� */
        if (TI_PORT_CONFIG_PRIORITY(off_port_info) <= TI_PORT_CONFIG_PRIORITY(port_info)) {
            POE_DRV_ZLOG_WARN("Port's Priority is %d, can't take others, PM!!!\n", 
                TI_PORT_CONFIG_PRIORITY(port_info));
            goto port_pm;
        }

            /* Ҫ���ڹ���Ķ˿� */
        if (TI_PORT_CTRL_STATE(off_port_info) != TI_PORT_ON
            /* ǿ�ƹ���Ķ˿ڲ��� */
            || TI_PORT_CONFIG_FORCEON_EN(off_port_info)) {
            continue;
        }
        /* ִ���µ� */
        rv = ssa_poe_ti_port_power_down(TI_GET_CHIPID(off_lport), TI_GET_CHIP_PORT(off_lport));
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_port_power_down failed, rv = %d\n", rv);
            continue;
        }
        /* �µ���pgc down��ͨ�� */
        ssa_poe_ti_set_port_state(off_port_info, TI_PORT_PM, POE_PM_OFF, false);
        ssa_poe_ti_clear_port_power_info(off_port_info);
        /* �˿ڹ����б䣬ϵͳ����ҲҪ���� */
        ssa_poe_ti_update_system_power();
    }

port_pm:
    if (TI_PORT_CTRL_STATE(port_info) != TI_PORT_PM) {
        ssa_poe_ti_clear_port_power_info(port_info);
        ssa_poe_ti_update_system_power();
        ssa_poe_ti_set_port_state(port_info, TI_PORT_PM, POE_PM_OFF, true);
    }

    return POE_E_NONE;
}

static int ssa_poe_ti_port_power_up_static(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    uint32_t port_need_power;

    port_info = TI_PORT_INFO(chip, chip_port);
    
    POE_DRV_ZLOG_WARN("\n");
    port_need_power = ssa_poe_ti_get_power_need(port_info);
    if (port_need_power == 0) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_get_power_need failed\n");
        return -POE_E_FAIL;
    }

    /* ��̬ģʽ�Ҷ˿ڹ̶����书��Ϊ0���ϵ� ���书��С�ڷּ����ʲ��ϵ� */
    if (TI_PORT_CONFIG_STATIC_POWER(port_info) == 0 
            || port_need_power > TI_PORT_CONFIG_STATIC_POWER(port_info)) {
        POE_DRV_ZLOG_ERROR("Port's static alloc power is zero!\n");
        return POE_E_NONE;
    }

    ssa_poe_ti_update_system_power();
    /* �ɲ��˫��Դ���ܻ���־�̬���书�ʳ�����Դ���ʵ���� */
    if (TI_PORT_CONFIG_STATIC_POWER(port_info) != 0 && TI_PORT_CTRL_ALLOC_POWER(port_info) == 0) {
        if (TI_PORT_CTRL_STATE(port_info) != TI_PORT_PM) {
            ssa_poe_ti_set_port_state(port_info, TI_PORT_PM, POE_PM_OFF, true);
        }

        return POE_E_NONE;
    }

    rv = ssa_poe_ti_exec_port_power_up(chip, chip_port);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_exec_port_power_up failed, rv = %d\n", rv);
        return rv;
    }
    /* �ϵ��ͨ�棬��֤�����ȼ�˳��ͨ�� */
    ssa_poe_ti_set_port_state(port_info, TI_PORT_ON, POE_NORMAL, true);

    return POE_E_NONE;
}

/**
 * ssa_poe_ti_port_power_up - ��������ö˿��ϵ�
 * IN:
 * @chip: оƬ��
 * @chip_port: оƬ�˿ں�
 *
 * ����ֵ:
 *     �ɹ� POE_E_NONE, ʧ�� - ����
*/
int ssa_poe_ti_port_power_up(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    
    POE_DRV_ZLOG_WARN("[ssa_poe_ti_port_power_up] Chip-port[%d, %d]\n", chip, chip_port);

    if (SSA_POE_TI_CHIP_INVALID(chip) || SSA_POE_TI_CHIP_PORT_INVALID(chip_port)) {
        POE_DRV_ZLOG_ERROR("chip or chip_port invalid, chip = %d, chip_port = %d\n", chip, chip_port);
        return -POE_E_FAIL;
    }

    port_info = TI_PORT_INFO(chip, chip_port);

    /* �����ϵ�Ķ˿�ֱ�ӷ��� */
    if (!ssa_poe_ti_pwr_up_check(port_info)) {
        POE_DRV_ZLOG_WARN("Can't be powered on\n");
        return POE_E_NONE;
    }

    /* ��̬ģʽ���˿���ǰ��û�����˵Ļ�������ֱ���ϵ� */
    if (TI_SYS_PM_MODE == POE_PM_STATIC) {
        rv = ssa_poe_ti_port_power_up_static(chip, chip_port);
    } else {
        rv = ssa_poe_ti_port_power_up_energy_auto(chip, chip_port);
    }

    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_port_power_up failed, rv = %d\n", rv);
    }
    
    return rv;
}

/**
 * ssa_poe_ti_mng_init - TIоƬ�������ʹ����ʼ������
 *
 * return: �ɹ�SS_E_NONE, ʧ�ܷ��ظ���
 */
int ssa_poe_ti_mng_init(void)
{
    return POE_E_NONE;
}
