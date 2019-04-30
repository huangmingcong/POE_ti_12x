
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/time.h>

#include "ssa_poe_mom.h"
#include "poe_tps_drv.h"
#include "poe_ssa_mng.h"
#include "poe_tps_cpld.h"

static ti_chip_irq_info_t irq_info_cache[NUM_OF_TPS2388x * NUM_OF_QUARD + 1]; /* 用于缓存每次中断读出来的事件 */
/* 用于缓存每个周期要上电的端口 */
int power_up_port[POWER_UP_PORT_NUM] = {0};

typedef int (*handler_func_t)(uint32_t chip, uint32_t chip_port);

int m_poe_hw_scan = 1;
int m_poe_notify = 3;  /* 每3s 进行一次端口信息更新 */
int g_poe_ti_led_mode = TI_LED_MODE_SWITCH;
extern bool g_ti_sys_inited;

static int ssa_poe_ti_power_up(void);
static int ssa_poe_ti_restart_port(void);
static int ssa_poe_ti_parse_power_down(void);
static int ssa_poe_ti_parse_detcls(void);
static int ssa_poe_ti_update_port_data(void);
static int ssa_poe_ti_update_port_cool_down(void);
static int ssa_poe_ti_update_led_mode(void);

static ti_main_thread_task_info_t ti_task[TI_TASK_MAX] = {
    {TI_RESTART_DET_CLS,  TI_RESTART_DET_CLS_PERIOD,  ssa_poe_ti_restart_port},
    {TI_READ_DETCLS_IRQ,  TI_READ_DETCLS_IRQ_PERIOD,  ssa_poe_ti_parse_detcls},
    {TI_UPDATE_PORT_DATA, TI_UPDATE_PORT_DATA_PERIOD, ssa_poe_ti_update_port_data},
    {TI_UPDATE_COOL_DOWN, TI_UPDATE_COOL_DOWN_PERIOD, ssa_poe_ti_update_port_cool_down},
    {UPDATE_LED_MODE,     UPDATE_LED_MODE_PERIOD,     ssa_poe_ti_update_led_mode},
};

static int ssa_poe_ti_none_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_tsd_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_vpuv_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_vduv_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_ilim_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_icut_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_istart_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_disc_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_pgc_up_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_pgc_down_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_pec_on_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_pec_off_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_clsc_event_handler(uint32_t chip, uint32_t chip_port);
static int ssa_poe_ti_detc_event_handler(uint32_t chip, uint32_t chip_port);

/* 处理各种事件 */
static handler_func_t ssa_poe_ti_event_handler_func[TI_IRQ_EVENT_MAX] = {
    ssa_poe_ti_none_event_handler,
    ssa_poe_ti_tsd_event_handler,
    ssa_poe_ti_vpuv_event_handler,
    ssa_poe_ti_vduv_event_handler,
    ssa_poe_ti_ilim_event_handler,
    ssa_poe_ti_icut_event_handler,
    ssa_poe_ti_istart_event_handler,
    ssa_poe_ti_disc_event_handler,
    ssa_poe_ti_pgc_up_event_handler,
    ssa_poe_ti_pgc_down_event_handler,
    ssa_poe_ti_pec_on_event_handler,
    ssa_poe_ti_pec_off_event_handler,
    ssa_poe_ti_clsc_event_handler,
    ssa_poe_ti_detc_event_handler,
};

/* 通告端口信息 */
int poe_port_status_update(int lport, int notify)
{
    int ret;
    uint32_t resistence;
    uint32_t phyid;
    uint32_t cur, vol;
    app_mom_t *mom;
    ssa_poe_ti_port_info_t *port_info;
    
    SPoe__PoeIntfIndex poe_intf_index;
    SSsapoe__SsaPoeIntfSta ssa_poe_intf_sta;
    SSsapoe__SsaPoePortSta sta_info = S_SSAPOE__SSA_POE_PORT_STA__INIT;
    
    s_poe__poe_intf_index__init(&poe_intf_index);
    s_ssapoe__ssa_poe_intf_sta__init(&ssa_poe_intf_sta);    

    mom = poe_ssa_get_app_mom();

    port_info = TI_LPORT_INFO(lport);
   
    ret = poe_ssa_ti_read_port_resistance(lport, &resistence);
    if (ret != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("Driver poe_ssa_ti_read_port_resistance error!");
    }
    TI_PORT_DATA_DET_RESISTANCE(port_info) = resistence;
/*  冗余
    ret = poe_ssa_ti_read_port_class_status(lport, &port_class);
    if (ret != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("Driver poe_ssa_ti_read_port_class_status error!");
    }
    TI_PORT_DATA_CLS_STATUS(port_info) = port_class;

    ret = poe_ssa_ti_read_port_detect_status(lport, (TPS238x_Detection_Status_t *)&port_detect);
    if (ret != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("Driver poe_ssa_ti_read_port_detect_status error!");
    }
    TI_PORT_DATA_DET_STATUS(port_info) = port_detect;
*/ 
    ret = poe_ssa_ti_get_port_measurements(TI_PORT_CTRL_LPORT(port_info), &cur, &vol);
    if (ret != POE_E_NONE) {
       POE_DRV_ZLOG_ERROR("ssa_poe_ti_read_port_current failed, ret = %d\n", ret);
    }

    TI_PORT_DATA_CURRENT(port_info) = cur;
    TI_PORT_DATA_VOLTAGE(port_info) = vol;
    TI_PORT_DATA_CONSUME_POWER(port_info) = cur * vol / 1000;

    POE_DRV_ZLOG_WARN("detect = %d, class = %d, cur = %d, vol = %d, power = %d\n",
    TI_PORT_DATA_DET_STATUS(port_info), TI_PORT_DATA_CLS_STATUS(port_info), 
    TI_PORT_DATA_CURRENT(port_info), TI_PORT_DATA_VOLTAGE(port_info), 
    TI_PORT_DATA_CONSUME_POWER(port_info));
    
    if (notify) {
        /* lport to phyid */
        ret = poe_ssa_get_phyid_by_lport(lport, &phyid);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("poe_ssa_get_phyid_by_lport fail, ret:%d", ret);
            return POE_E_FAIL;
        }

        /* 上电的端口才通告 */
      //  if (TI_PORT_DATA_POWER_UP(port_info)) {
           poe_intf_index.phyid = phyid;
           ssa_poe_intf_sta.index = &poe_intf_index;
           ssa_poe_intf_sta.ntf = S_POE__POE_NTF_E__POE_PORT_SINGLE_INFO;

           sta_info.power_up = TI_PORT_DATA_POWER_UP(port_info);
           sta_info.detect = TI_PORT_DATA_DET_STATUS(port_info);
           sta_info.class_ = TI_PORT_DATA_CLS_STATUS(port_info);
           sta_info.icut = TI_PORT_DATA_CURRENT(port_info);
           sta_info.vol = TI_PORT_DATA_VOLTAGE(port_info);
           sta_info.power_cons = TI_PORT_DATA_CONSUME_POWER(port_info);
           sta_info.resis_det = TI_PORT_DATA_DET_RESISTANCE(port_info);
           sta_info.reason = TI_PORT_CTRL_OFF_REASON(port_info);
            
           ssa_poe_intf_sta.sta = &sta_info;
    
           ret = app_mom_db_set(mom, RG_MOM_ASIC_DB, &ssa_poe_intf_sta);
           if (ret != 0) {
               POE_DRV_ZLOG_ERROR("app_mom_db_set fail, ret:%d", ret);
               return POE_E_FAIL;
           } 
        }
 //   }

    return 0;
}

int poe_ports_sw_state_update(int time, void *usr, void *app)
{
    int rv;
    int i;
    app_info_t *app_info;

    app_info = (app_info_t *)app; 
    
    for (i = 1; i <= POE_MAX_PORT; i++) {
        rv = poe_port_status_update(i, true);
        POE_DRV_ZLOG_ERROR("poe_port_hw_stat_update fail, rv:%d", rv);
    }
   
    return time;
}

/* create the thread to polling the informations */
int poe_port_status_handler_init(void)
{
    app_info_t *app_info;    

    POE_DRV_ZLOG_DEBUG("poe_port_status_handler_init begin");

    app_info = poe_ssa_get_app_info();
 
    app_proc_timer(app_info, poe_ports_sw_state_update, m_poe_notify, NULL);
    
    POE_DRV_ZLOG_DEBUG("poe_port_status_handler_init end");

    return POE_E_NONE;
}

/* 热重启不下电时恢复数据 */
int poe_ssa_ti_ups_recover_data(void)
{
    int rv;
    uint32_t lport;
    bool pg;
    ssa_poe_ti_port_info_t *port_info;
    
    POE_DRV_ZLOG_INFO("poe_ssa_ti_ups_recover_data Enter!\n");

    /* 遍历端口顺序恢复数据 */
    SSA_POE_FOR_EACH_LPORT_PRI_HIGH_TO_LOW(lport) {
        /* 获取该芯片端口供电状态 */
        rv = poe_ssa_ti_read_port_power_status(lport, &pg);
        if (rv != POE_E_NONE) {
            printf("poe_ssa_ti_read_port_power_status failed, rv = %d\n", rv);
            continue;
        }
        
        port_info = TI_LPORT_INFO(lport);

        /* 恢复供电状态 */
        TI_PORT_DATA_POWER_UP(port_info) = pg;
    
        /* 如果端口不处于上电状态，则读下一个端口 */
        if (!TI_PORT_DATA_POWER_UP(port_info)) {
            continue;
        }

        /* 恢复端口的detect和class结果 */
        rv = poe_ssa_ti_read_port_detect_status(lport, &TI_PORT_DATA_DET_STATUS(port_info));
        rv += poe_ssa_ti_read_port_class_status(lport, &TI_PORT_DATA_CLS_STATUS(port_info));
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_read_port_detect_class_status failed, rv = %d\n", rv);
            continue;
        }

        /* 如果detect结果是非标 */
        if (TI_PORT_DATA_DET_STATUS(port_info) == DETECT_RESIST_LOW
            || TI_PORT_DATA_DET_STATUS(port_info) == DETECT_RESIST_HIGH) {
            /* 非标PD由软件分级为class 0 */
            TI_PORT_DATA_CLS_STATUS(port_info) = PD_CLASS_0;
        }

        /* 恢复端口电流、电压 */
        rv = poe_ssa_ti_get_port_measurements(lport, &TI_PORT_DATA_CURRENT(port_info), &TI_PORT_DATA_VOLTAGE(port_info));
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_read_port_current failed, rv = %d\n", rv);
            continue;
        }

        /* 功率 mA * mV / 1000 = mW */
        TI_PORT_DATA_CONSUME_POWER(port_info) = 
            TI_PORT_DATA_CURRENT(port_info) * TI_PORT_DATA_VOLTAGE(port_info) / 1000;

        if (TI_SYS_PM_MODE == POE_PM_ENERGYSAVE) {
            TI_PORT_CTRL_ALLOC_POWER(port_info) = TI_PORT_DATA_CONSUME_POWER(port_info);
        } else if (TI_SYS_PM_MODE == POE_PM_AUTO) {
            TI_PORT_CTRL_ALLOC_POWER(port_info) = ssa_poe_ti_get_power_need(port_info);
        } else {

        }

        /* 恢复端口icut配置 */
        rv = poe_ssa_ti_read_port_pcut_config(lport, &TI_PORT_CTRL_ICUT(port_info));
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_read_port_icut_config failed, rv = %d\n", rv);
            continue;
        }
        
        TI_PORT_CTRL_PWON(port_info) = true;
        POE_DRV_ZLOG_INFO("Port[%d] is power up, det=%d, cls=%d, cur=%d, vol=%d\n", 
            TI_PORT_CTRL_LPORT(port_info), TI_PORT_DATA_DET_STATUS(port_info), 
            TI_PORT_DATA_CLS_STATUS(port_info), TI_PORT_DATA_CURRENT(port_info), 
            TI_PORT_DATA_VOLTAGE(port_info));
    }
    /* 再更新系统功率 */
    ssa_poe_ti_update_system_power();

    return POE_E_NONE;
}

static int ssa_poe_ti_bud_dec_down_port_energy_auto(void)
{
    int rv;
    uint32_t chip;
    uint32_t chip_port;
    uint32_t off_lport;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_INFO("Enter!\n");

    SSA_POE_FOR_EACH_LPORT_PRI_LOW_TO_HIGH(off_lport) {
        POE_DRV_ZLOG_WARN("Current remain power = %d\n", TI_SYS_REMAIN_POWER);
        /* 有剩余功率，不再继续下电，返回 */
        if (TI_SYS_REMAIN_POWER > 0) {
            POE_DRV_ZLOG_WARN("Remain power is enough, return!\n");
            return POE_E_NONE;
        }

        port_info = TI_LPORT_INFO(off_lport);
        chip = TI_GET_CHIPID(off_lport);
        chip_port = TI_GET_CHIP_PORT(off_lport);
        if (!TI_PORT_DATA_POWER_UP(port_info) || TI_PORT_CONFIG_FORCEON_EN(port_info)) {
            continue;
        }
        
        POE_DRV_ZLOG_INFO("Lport = %d is going to be powered down!\n", off_lport);
        rv = ssa_poe_ti_port_power_down(chip, chip_port);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_port_power_down failed, rv = %d\n", rv);
            continue;
        }

        /* 这里需要循环计算功率，其它的放到PGC里清 */
        ssa_poe_ti_clear_port_power_info(port_info);
        ssa_poe_ti_update_system_power();
        /* 状态重置为PM */
        ssa_poe_ti_set_port_state(port_info, TI_PORT_PM, POE_PM_OFF, false);
    }
    
    return POE_E_NONE;
}

static int ssa_poe_ti_bud_dec_down_port_static(void)
{
    int rv;
    uint32_t chip;
    uint32_t chip_port;
    uint32_t off_lport;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_INFO("Enter!\n");

    for (off_lport = POE_MAX_PORT; off_lport > 0; off_lport--) {
        port_info = TI_LPORT_INFO(off_lport);
        POE_DRV_ZLOG_INFO("lport %d static_power = %d, alloc_power = %d\n", off_lport, 
            TI_PORT_CONFIG_STATIC_POWER(port_info), TI_PORT_CTRL_ALLOC_POWER(port_info));

        /* 可以分配的功率满足静态分配的功率 */
        if (TI_PORT_CTRL_ALLOC_POWER(port_info) == TI_PORT_CONFIG_STATIC_POWER(port_info)) {
            continue;
        }

        if (!TI_PORT_DATA_POWER_UP(port_info) || TI_PORT_CONFIG_FORCEON_EN(port_info)) {
            continue;
        }
        
        POE_DRV_ZLOG_INFO("lport = %d is going to be powered down!\n", off_lport);
        chip = TI_GET_CHIPID(off_lport);
        chip_port = TI_GET_CHIP_PORT(off_lport);
        rv = ssa_poe_ti_port_power_down(chip, chip_port);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_port_power_down failed, rv = %d\n", rv);
            continue;
        }

        /* 状态重置为PM */
        ssa_poe_ti_set_port_state(port_info, TI_PORT_PM, POE_PM_OFF, false);
    }
    
    return POE_E_NONE;
}

/* 系统总功率降低，一些端口可能需要下电 */
static int ssa_poe_ti_bud_dec_set_port_power_down(void)
{
    int rv;
    
    POE_DRV_ZLOG_INFO("Enter!\n");

    if (TI_SYS_PM_MODE == POE_PM_STATIC) {
        rv = ssa_poe_ti_bud_dec_down_port_static();
    } else {
        rv = ssa_poe_ti_bud_dec_down_port_energy_auto();
    }

    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_bud_dec_set_port_power_down failed, rv = %d\n", rv);
    }

    return rv;
}

/* lldp使能 */
static int mom_event_set_lldp_enable(int lldp_en)
{
    bool lldp_enable;

    POE_DRV_ZLOG_INFO("lldp_en = %d\n", lldp_en);

    lldp_enable = lldp_en > 0 ? true : false;

    TI_SYS_LLDP_EN = lldp_enable;

    return POE_E_NONE;
}

/* 设置系统保留功率 */
static int mom_event_set_power_reserve(int power_reserve)
{
    POE_DRV_ZLOG_INFO("power_reserve = %d, Current power_reserve = %d\n", 
        power_reserve, TI_SYS_RESERVE_PWR_RATE);

    if (TI_SYS_RESERVE_PWR_RATE == power_reserve) {
        POE_DRV_ZLOG_WARN("power_reserve is not changed, return!\n");
        return POE_E_NONE;
    } else {
        TI_SYS_RESERVE_PWR_RATE = power_reserve;
        TI_SYS_RESERVE_POWER = TI_SYS_POE_TOTAL_POWER * TI_SYS_RESERVE_PWR_RATE / 100;
        POE_DRV_ZLOG_WARN("Total_power = %d, Reserve_power = %d!\n", 
            TI_SYS_POE_TOTAL_POWER, TI_SYS_RESERVE_POWER);
        ssa_poe_ti_update_system_power();
    }

    if (TI_SYS_PM_MODE == POE_PM_ENERGYSAVE) {
        /* 看看是不是有端口要下电 */
        (void)ssa_poe_ti_bud_dec_set_port_power_down();
    }

    return POE_E_NONE;
}

/* 设置系统ups */
static int mom_event_set_ups_enable(int enable)
{
    int rv;
    bool ups_enable;
    
    POE_DRV_ZLOG_INFO("ups_en = %d\n", enable);

    ups_enable = enable > 0 ? true : false;

    rv = ssa_poe_ti_cpld_set_ups(ups_enable);
    if (rv < 0) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_cpld_set_ups failed, rv = %d\n", rv);
        return POE_E_FAIL;
    }
    TI_SYS_UPS_EN = ups_enable;

    return POE_E_NONE;

}

/* 设置系统功率管理模式 */
static int mom_event_set_pm_mode(int pm_mode)
{
    int rv;
    int lport;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_INFO("pm_mode = %d\n", pm_mode);

    if (pm_mode < POE_PM_AUTO || pm_mode > POE_PM_STATIC) {
        POE_DRV_ZLOG_ERROR("pm_mode[%d] is error.\n", pm_mode);
        return POE_E_PARAM;
    }
    
    rv = POE_E_NONE;
    if (TI_SYS_PM_MODE == pm_mode) {
        POE_DRV_ZLOG_ERROR("Current pm_mode = %d, don't need to change!\n", TI_SYS_PM_MODE);
        return POE_E_NONE;
    }

    TI_SYS_PM_MODE = pm_mode;
    
    /* 关闭所有端口 */
    rv = poe_ssa_all_port_power_off();
    if (rv != POE_E_NONE) {
       POE_DRV_ZLOG_ERROR("poe_ssa_all_port_power_off failed, rv = %d\n", rv);
       return rv;
    }

    /* 清空所有端口信息 */
    SSA_POE_FOR_EACH_LPORT_PRI_LOW_TO_HIGH(lport) {
        port_info = TI_LPORT_INFO(lport);
        ssa_poe_ti_clear_port_power_info(port_info);
        ssa_poe_ti_clear_port_detect_info(port_info);
        ssa_poe_ti_clear_port_i_count_info(port_info);
        ssa_poe_ti_set_port_state(port_info, TI_PORT_OFF, POE_NORMAL, true);
    }
    
    ssa_poe_ti_update_system_power();

    rv = poe_ssa_all_port_reopen();
    if (rv != POE_E_NONE) {
       POE_DRV_ZLOG_ERROR("poe_ssa_all_port_reopen failed, rv = %d\n", rv);
    }

    return 0;
}

/* 接收来自ssc的系统配置 */
int ssa_poe_sys_conf_proc(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj)
{
    int ret;

    POE_DRV_ZLOG_INFO("%s %d %d %d %d %p %p\n", __func__, db, type, flag, cmd, value, rgobj);

    SPoe__PoeSysConfInfo *conf_info;

    conf_info = (SPoe__PoeSysConfInfo *)value;
    POE_DRV_ZLOG_INFO("ssa_poe_sys_conf_proc, conf_info->op = %d\n", conf_info->op);

    switch (conf_info->op) {
    /* 系统信息配置 */    
    case S_POE__POE_OP_E__POE_SYS_PM_MODE:
        ret = mom_event_set_pm_mode(conf_info->value);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_event_set_pm_mode fail, ret:%d", ret);
            return POE_E_FAIL;
        }

        break;
    case S_POE__POE_OP_E__POE_SYS_UPS_ENABLE:
        ret = mom_event_set_ups_enable(conf_info->value);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_event_set_ups_enable fail, ret:%d", ret);
            return POE_E_FAIL;
        }

        break;
    case S_POE__POE_OP_E__POE_SYS_POWER_RESERVE:
        ret = mom_event_set_power_reserve(conf_info->value);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_event_set_power_reserve fail, ret:%d", ret);
            return POE_E_FAIL;
        }

        break;
    case S_POE__POE_OP_E__POE_SYS_LLDP_PDTYPE_EN:
        ret = mom_event_set_lldp_enable(conf_info->value);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_event_set_lldp_enable fail, ret:%d", ret);
            return POE_E_FAIL;
        }

        break;
    default :
        break;
        }
    return 0;
}

/* 设置端口pdtype */
static int mom_event_set_port_pdtype(int lport, int phyid, poe_lldp_pdtype_t lldptype)
{
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("lport = %d, lphyid = %d, lldptype = %d", lport, phyid, lldptype);

    port_info = TI_LPORT_INFO(lport);

    /* 如果硬件分级结果不是class4，说明PD有问题 */
    if (TI_PORT_DATA_CLS_STATUS(port_info) != PD_CLASS_4 
        && (lldptype == POE_LLDP_PDTYPE_1 || lldptype == POE_LLDP_PDTYPE_2)) {
        POE_DRV_ZLOG_ERROR("pd class = %d isn't class 4, pd_type = %d error.\n", 
            TI_PORT_DATA_CLS_STATUS(port_info), lldptype);
        return POE_E_FAIL;
    }

    TI_PORT_CTRL_LLDP_PDTYPE(port_info) = lldptype;
        
    return POE_E_NONE;
}

/* 设置端口分配功率 */
static int mom_event_set_port_alloc_power(int lport, int phyid, uint32_t alloc_power)
{
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("lport = %d, lphyid = %d, alloc_power = %d", lport, phyid, alloc_power);

    port_info = TI_LPORT_INFO(lport);
    TI_PORT_CONFIG_STATIC_POWER(port_info) = alloc_power;
    if (TI_SYS_PM_MODE == POE_PM_STATIC) {
        ssa_poe_ti_update_system_power();
    }

    return POE_E_NONE;
}

/* 设置端口最大功率 */
static int mom_event_set_port_maxpower(int lport, int phyid, int power_max)
{
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("lport = %d, lphyid = %d, prority = %d", lport, phyid, power_max);

    port_info = TI_LPORT_INFO(lport);
    TI_PORT_CONFIG_MAX_POWER(port_info) = power_max;

    return POE_E_NONE;
}

/* 设置端口非标使能 */
static int mom_even_set_port_legacy(int lport, int phyid, bool enable)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    TPS238x_Operating_Modes_t opmode;
    
    POE_DRV_ZLOG_WARN("lport = %d, lphyid = %d, enable = %d", lport, phyid, enable);

    port_info = TI_LPORT_INFO(lport);
    
    if (enable) {
        opmode = OPERATING_MODE_DIAGNOSTIC;
    } else {
        opmode = OPERATING_MODE_SEMI_AUTO;
    }

    rv =  poe_ssa_ti_write_port_operating_mode(lport, opmode);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_operating_mode failed, rv = %d\n", rv);
        return rv;
    }
    
    /* 关闭非标切到semiauto模式，要重新打开det/cls */
    if (opmode == OPERATING_MODE_SEMI_AUTO) {
        rv = poe_ssa_ti_write_port_detect_class_enable(lport, true);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_detect_class_enable fail , rv = %d", rv);
            return rv;
        }
    } 

    TI_PORT_CONFIG_LEGACY_EN(port_info) = enable;

    return POE_E_NONE;
}

/* 设置端口强制供电 */
static int mom_even_set_port_force_en(int lport, int phyid, bool enable)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("lport = %d, lphyid = %d, enable = %d", lport, phyid, enable);

    port_info = TI_LPORT_INFO(lport);

    /* 端口没开的话，强制使能无效? */
    if (!TI_PORT_CONFIG_POE_EN(port_info)) {
        POE_DRV_ZLOG_WARN("Current port enable = %d, can't force-on!\n", 
            TI_PORT_CONFIG_POE_EN(port_info));
        return POE_E_NONE;
    }

    if (enable) {
        rv = poe_ssa_ti_write_port_force_on(lport);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_force_on fail , rv = %d", rv);
            return rv;
        }
        TI_PORT_CONFIG_FORCEON_EN(port_info) = true;
        
        /* 这两个由软件置上 */
        TI_PORT_DATA_DET_STATUS(port_info) = DETECT_RESIST_VALID;
        TI_PORT_DATA_CLS_STATUS(port_info) = PD_CLASS_0;
        TI_PORT_CTRL_ICUT(port_info)       = TI_PCUT_90;
        /* XXX 强制供电需要分配功率? */
        TI_PORT_CTRL_ALLOC_POWER(port_info) = 60000;
        /* 强制供电的通告、led在PGC处理 */
        ssa_poe_ti_set_port_state(port_info, TI_PORT_ON, POE_NORMAL, false);
    } else {
        rv = poe_ssa_ti_write_port_force_off(lport);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_force_off fail , rv = %d", rv);
            return rv;
        }
        TI_PORT_CONFIG_FORCEON_EN(port_info) = false;
        TI_PORT_CTRL_ALLOC_POWER(port_info) = 0;
        /* 强制供电的通告、led在PGC处理 */
        ssa_poe_ti_set_port_state(port_info, TI_PORT_OFF, POE_NORMAL, false);
    }
    
    return POE_E_NONE;
}

/* 设置端口优先级 */
static int mom_event_set_port_priority(int lport, int phyid, SPoe__PoePriE priority)
{
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("lport = %d, lphyid = %d, prority = %d", lport, phyid, priority);

    port_info = TI_LPORT_INFO(lport);
    TI_PORT_CONFIG_PRIORITY(port_info) = priority;

    return POE_E_NONE;
}

/* 设置端口检测使能 */
static int mom_event_set_port_detect_enable(int lport, int phyid, int enable, int args)
{
    int ret;
    ssa_poe_ti_port_info_t *port_info;
    bool port_enable;
    bool pg;
    
    POE_DRV_ZLOG_WARN("lport = %d, enable = %d", lport, enable);
    printf("args = %d\n", args);
    port_enable = enable > 0 ? true : false;
    port_info = TI_LPORT_INFO(lport);
    
    if (port_enable) {
        /* 开启端口检测分级使能即可 */
        ret = poe_ssa_ti_write_port_detect_class_enable(lport, true);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_detect_class_enable fail , ret = %d", ret);
            return ret;
        }
        
        TI_PORT_CONFIG_POE_EN(port_info) = true;
    } else {
        /* 如果端口正在供电，需要把端口power off，然后在检测使能关闭 */
        ret = poe_ssa_ti_read_port_power_status(lport, &pg);
        if (ret != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("poe_ssa_ti_read_port_power_status failed, ret = %d\n", ret);
            pg = false;
        }

        /* PG为0，不能进行power off操作 */
        if (pg) {
            ret = poe_ssa_ti_write_port_power_enable(lport, false);
            if (ret != POE_E_NONE) {
                POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_power_enable failed, ret = %d\n", ret);
                return ret;
            }
        }
#if 0 /* 执行power off寄存器 det/cls会清零 */
        usleep(TI_COMMAND_DELAY_SHORT);
        ret = poe_ssa_ti_write_port_detect_class_enable(lport, false);
        if (ret!= POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_detect_class_enable failed, ret = %d\n", ret);
            return ret;
        }
#endif
        /* 关闭端口要同时关强制供电 */
        if (TI_PORT_CONFIG_FORCEON_EN(port_info)) {
            POE_DRV_ZLOG_WARN("Disable force on!\n");
            ret = poe_ssa_ti_write_port_disconnect_enable(lport, true);
            if (ret != POE_E_NONE) {
                POE_DRV_ZLOG_ERROR("ssa_poe_ti_write_port_disconnect_enable failed, ret = %d\n", ret);
                return ret;
            }
            TI_PORT_CONFIG_FORCEON_EN(port_info) = false;
        }

        TI_PORT_CONFIG_POE_EN(port_info) = false;

        /* overload是ce配置下来的，等下电再通告 */
        if (args == POE_OVERLOAD) {
            ssa_poe_ti_set_port_state(port_info, TI_PORT_OVL, POE_OVERLOAD, false);
        } else if (args == POE_NORMAL) {
            ssa_poe_ti_set_port_state(port_info, TI_PORT_OFF, POE_NORMAL, true);
        }
    }
    TI_PORT_DATA_PHTID(port_info) = phyid;
    
    return POE_E_NONE;
}

/* 接收来自ssc的配置处理 */
int ssa_poe_intf_conf_proc(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj)
{
    int lport;
    int ret;
    uint32_t phyid;

    POE_DRV_ZLOG_INFO("%s %d %d %d %d %p %p\n", __func__, db, type, flag, cmd, value, rgobj);

    SPoe__PoeIntfConfInfo *conf_info;

    conf_info = (SPoe__PoeIntfConfInfo *)value;
    POE_DRV_ZLOG_INFO("conf_info->op = %d\n", conf_info->op);

    phyid = conf_info->index->phyid;


    if (poe_ssa_is_local_port(phyid) == 0) {
        POE_DRV_ZLOG_DEBUG("poe_ssa_is_not_local_port");
        return 0;
    }

    ret = poe_ssa_get_lport_by_phyid(phyid, &lport);
    if (ret != 0) {
        POE_DRV_ZLOG_ERROR("poe_ssa_get_lport_by_phyid fail, ret:%d", ret);
        return POE_E_FAIL;
    }
	POE_DRV_ZLOG_INFO("####lport = %d\n", lport);

    switch (conf_info->op) {
    /* 端口信息配置 */
    case S_POE__POE_OP_E__POE_PORT_DETECT_ENABLE: 
        ret = mom_event_set_port_detect_enable(lport, phyid, conf_info->poe_en, conf_info->reason);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_event_set_port_detect_enable fail, ret:%d", ret);
            return POE_E_FAIL;
        }
        break;
    case S_POE__POE_OP_E__POE_PORT_POWER_ENABLE:
        /* do nothing */
        break;
    case S_POE__POE_OP_E__POE_PORT_PRIORITY:
        ret = mom_event_set_port_priority(lport, phyid, conf_info->port_pri);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_event_set_port_priority fail, ret:%d", ret);
            return POE_E_FAIL;
        }
        break;
    case S_POE__POE_OP_E__POE_PORT_FORCE_ON:
        ret = mom_even_set_port_force_en(lport, phyid, conf_info->force_en);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_even_set_port_force_en fail, ret:%d", ret);
            return POE_E_FAIL;
        }
        break;
    case S_POE__POE_OP_E__POE_PORT_LEGACY:
        ret = mom_even_set_port_legacy(lport, phyid, conf_info->legacy_en);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_even_set_port_legacy fail, ret:%d", ret);
            return POE_E_FAIL;
        }            
        break;
    case S_POE__POE_OP_E__POE_PORT_POWER_MAX:
        ret = mom_event_set_port_maxpower(lport, phyid, conf_info->power_max);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_event_set_port_maxpower fail, ret:%d", ret);
            return POE_E_FAIL;
        }            
        break;
    case S_POE__POE_OP_E__POE_PORT_POWER_ALLOC:
        ret = mom_event_set_port_alloc_power(lport, phyid, conf_info->power_alloc);
        if (ret != 0) {
            POE_DRV_ZLOG_ERROR("mom_event_set_port_alloc_power fail, ret:%d", ret);
            return POE_E_FAIL;
        }    
        break;
    case S_POE__POE_OP_E__POE_PORT_LLDP_PDTYPE:
        //ret = mom_event_set_port_pdtype(lport, phyid, conf_info->);
        break;
    default :
        break;
    }
    
    return 0;
}

/* 占坑用 */
static int ssa_poe_ti_none_event_handler(uint32_t chip, uint32_t chip_port)
{
    return POE_E_NONE;
}

/* 处理tsd下电事件 thermal shutdown */
static int ssa_poe_ti_tsd_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    
    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    TI_SYS_COOL_DOWN_TIME = TI_TEMP_COOL_DOWN_PERIOD;
    /* 后面会复位芯片，所以这里提前复位软件icut */
   // TI_PORT_CTRL_ICUT(port_info) = TI_ICUT_374;

    ssa_poe_ti_clear_port_detect_info(port_info);
    ssa_poe_ti_clear_port_power_info(port_info);
    /* 释放功率 */
    ssa_poe_ti_update_system_power();
    
    /* 原来处于TI_PORT_OFF的端口，要么是被disable了，要么是disconnect，不用通告 */
    if (TI_PORT_CTRL_STATE(port_info) != TI_PORT_OFF) {
        /* reset状态下可能读不到，所以手动清 */
        TI_PORT_CTRL_PWON(port_info) = false;
        TI_PORT_DATA_POWER_UP(port_info) = false;
        ssa_poe_ti_set_port_state(port_info, TI_PORT_TEMP, POE_OVER_TEMP, true);
    }

    /* 
     * TSD、VPUV、VDUV事件，都是一块芯片四个端口一起下电的，芯片相当于执行了reset
     * 芯片需要重新进行设置，设置一次就行
    */
    if (chip_port == 2) {
        rv = ssa_poe_ti_recover_chip_config(chip);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_chip_init failed, rv = %d\n", rv);
            return rv;
        }
    }

    return POE_E_NONE;
}

/*
 * 处理vpuv事件
 * 当VUVLOPW_F < VPWR < VPUV_F时，所有端口下电，相当于执行了PWOFF命令；
 * 当VPWR < VUVLOPW_F，所有端口下电，相当于执行了上电reset操作。
*/
static int ssa_poe_ti_vpuv_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    TI_SYS_COOL_DOWN_TIME = TI_UV_COOL_DOWN_PERIOD;
    //TI_PORT_CTRL_ICUT(port_info) = TI_ICUT_374;

    ssa_poe_ti_clear_port_detect_info(port_info);
    ssa_poe_ti_clear_port_power_info(port_info);
    /* 释放功率 */
    ssa_poe_ti_update_system_power();
    /* reset没有pgc? */
    if (TI_PORT_CTRL_STATE(port_info) != TI_PORT_OFF) {
        TI_PORT_CTRL_PWON(port_info) = false;
        TI_PORT_DATA_POWER_UP(port_info) = false;
        ssa_poe_ti_set_port_state(port_info, TI_PORT_VPUV, POE_OVERLOAD, true);
    }

    /* 
     * TSD、VPUV、VDUV事件，都是一块芯片四个端口一起下电的，芯片相当于执行了reset
     * 芯片需要重新进行设置，设置一次就行
    */
    if (chip_port == 2) {
        rv = ssa_poe_ti_recover_chip_config(chip);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_chip_init failed, rv = %d\n", rv);
            return rv;
        }
    }

    return POE_E_NONE;
}

/* 
 * 处理vduv事件
*/
static int ssa_poe_ti_vduv_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    TI_SYS_COOL_DOWN_TIME = TI_UV_COOL_DOWN_PERIOD;
   // TI_PORT_CTRL_ICUT(port_info) = TI_ICUT_374;

    ssa_poe_ti_clear_port_detect_info(port_info);
    ssa_poe_ti_clear_port_power_info(port_info);
    /* 释放功率 */
    ssa_poe_ti_update_system_power();
    
    /* reset没有pgc? */
    if (TI_PORT_CTRL_STATE(port_info) != TI_PORT_OFF) {
        TI_PORT_CTRL_PWON(port_info) = false;
        TI_PORT_DATA_POWER_UP(port_info) = false;
        ssa_poe_ti_set_port_state(port_info, TI_PORT_VDUV, POE_OVERLOAD, true);
    }

    /* 
     * TSD、VPUV、VDUV事件，都是一块芯片四个端口一起下电的，芯片相当于执行了reset
     * 芯片需要重新进行设置，设置一次就行
    */
    if (chip_port == 2) {
        rv = ssa_poe_ti_recover_chip_config(chip);
        if (rv != POE_E_NONE) {
            printf("ssa_poe_ti_chip_init failed, rv = %d\n", rv);
            return rv;
        }
    }

    return POE_E_NONE;
}

/* 处理ilim事件 */
static int ssa_poe_ti_ilim_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    /* 软件和芯片同步进行冷却 */
    TI_PORT_CTRL_COOL_DOWN_TIME(port_info) = TI_COOL_DOWN_PERIOD;
    TI_PORT_CTRL_ILIM_COUNT(port_info)++;
#if 0
    rv = ssa_poe_ti_recover_icut_ilim_def_config(chip, chip_port);
    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_recover_icut_ilim_def_config failed, rv = %d\n", rv);
    }
#endif
    ssa_poe_ti_clear_port_detect_info(port_info);
    ssa_poe_ti_clear_port_power_info(port_info);
    ssa_poe_ti_set_port_state(port_info, TI_PORT_ILIM, POE_SHORT_CIRCUIT, false);
    /* 释放功率 */
    ssa_poe_ti_update_system_power();

    return POE_E_NONE;
}

/* 处理icut事件 */
static int ssa_poe_ti_icut_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    
    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    /* 软件和芯片同步进行冷却 */
    TI_PORT_CTRL_COOL_DOWN_TIME(port_info) = TI_COOL_DOWN_PERIOD;
    TI_PORT_CTRL_ICUT_COUNT(port_info)++;
#if 0
    rv = ssa_poe_ti_recover_icut_ilim_def_config(chip, chip_port);
    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_recover_icut_ilim_def_config failed, rv = %d\n", rv);
    }
#endif
    ssa_poe_ti_clear_port_detect_info(port_info);
    ssa_poe_ti_clear_port_power_info(port_info);
    ssa_poe_ti_set_port_state(port_info, TI_PORT_ICUT, POE_SHORT_CIRCUIT, false);
    /* 释放功率 */
    ssa_poe_ti_update_system_power();

    return POE_E_NONE;
}

/* 
 * 处理istart事件
 * STRT事件在semiauto模式下有两个触发条件:
 * 1. 检测分级通过，在PWON过程发生Inrush fault
 * 2. 检测分级未通过执行PWON?
 * 在软件中第2种情况不至于发生
*/
static int ssa_poe_ti_istart_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    
    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    TI_PORT_CTRL_COOL_DOWN_TIME(port_info) = TI_COOL_DOWN_PERIOD;
    TI_PORT_CTRL_ISTRT_COUNT(port_info)++;
#if 0
    rv = ssa_poe_ti_recover_icut_ilim_def_config(chip, chip_port);
    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_recover_icut_ilim_def_config failed, rv = %d\n", rv);
    }
#endif
    ssa_poe_ti_clear_port_detect_info(port_info);
    ssa_poe_ti_clear_port_power_info(port_info);
    ssa_poe_ti_set_port_state(port_info, TI_PORT_STRTF, POE_OVER_CUT, false);
    /* 释放功率 */
    ssa_poe_ti_update_system_power();

    return POE_E_NONE;
}


/* 处理disc事件 */
static int ssa_poe_ti_disc_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
#if 0
    rv = ssa_poe_ti_recover_icut_ilim_def_config(chip, chip_port);
    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_recover_icut_ilim_def_config failed, rv = %d\n", rv);
    }
#endif   
    ssa_poe_ti_clear_port_i_count_info(port_info);
    ssa_poe_ti_clear_port_detect_info(port_info);
    ssa_poe_ti_clear_port_power_info(port_info);
    ssa_poe_ti_set_port_state(port_info, TI_PORT_OFF, POE_NORMAL, false);
    /* 释放功率 */
    ssa_poe_ti_update_system_power();

    return POE_E_NONE;
}

/* 处理pgc上电事件 */
static int ssa_poe_ti_pgc_up_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    uint32_t cur;
    uint32_t vol;

    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    /* pg为up，说明端口才真正开始稳定供电 */
    TI_PORT_DATA_POWER_UP(port_info) = true;
    /*
     * 更新电流电压，功率
    */
    rv = poe_ssa_ti_get_port_measurements(TI_PORT_CTRL_LPORT(port_info), &cur, &vol);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_read_port_current failed, rv = %d\n", rv);
        return rv;
    }

    TI_PORT_DATA_CURRENT(port_info) = cur;
    TI_PORT_DATA_VOLTAGE(port_info) = vol;
    TI_PORT_DATA_CONSUME_POWER(port_info) = cur * vol / 1000;
    /* 节能模式下，分配功率 = 消耗功率 */
    if (TI_SYS_PM_MODE == POE_PM_ENERGYSAVE) {
        TI_PORT_CTRL_ALLOC_POWER(port_info) = TI_PORT_DATA_CONSUME_POWER(port_info);
    }
    /* 实时更新系统功率 */
    ssa_poe_ti_update_system_power();
    /* 上电前面应该已经通告了，这里是更新电流 */
    //ssa_poe_push(POE_PORT_SINGLE_INFO, TI_PORT_CTRL_LPORT(port_info));

    return POE_E_NONE;
}

/* 
 * 处理pgc下电事件.
 * 主动下电(执行了PWOFF、reset、fastshut): 可能是用户关闭、PM、过载等情况
*/
static int ssa_poe_ti_pgc_down_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    
    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);
    
    port_info = TI_PORT_INFO(chip, chip_port);
    /* pg为down，说明真真下电 */
    TI_PORT_DATA_POWER_UP(port_info) = false;

    /* xxx */
    if (TI_PORT_CTRL_STATE(port_info) == TI_PORT_ON) {
        POE_DRV_ZLOG_WARN("Port state turn off\n");
        ssa_poe_ti_set_port_state(port_info, TI_PORT_OFF, POE_NORMAL, false);
    }
#if 0
    rv = ssa_poe_ti_recover_icut_ilim_def_config(chip, chip_port);
    if (rv != POE_E_NONE) {
        DBG_TI_ERR("ssa_poe_ti_recover_icut_ilim_def_config failed, rv = %d\n", rv);
    }
#endif
    /* 数据再清一下 */
    ssa_poe_ti_clear_port_detect_info(port_info);
    ssa_poe_ti_clear_port_power_info(port_info);
    /* 释放功率 */
    ssa_poe_ti_update_system_power();
    /* 下电通告 */
    // ssa_poe_push(POE_PORT_SINGLE_INFO, TI_PORT_CTRL_LPORT(port_info));
    /* led关灯 */
    //ssa_poe_ti_led_update(chip, chip_port);

    return POE_E_NONE;
}

/* 处理pec上电事件 */
static int ssa_poe_ti_pec_on_event_handler(uint32_t chip, uint32_t chip_port)
{
    ssa_poe_ti_port_info_t *port_info;
    
    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    TI_PORT_CTRL_PWON(port_info) = true;

    return POE_E_NONE;
}

/* 处理pec下电事件 */
static int ssa_poe_ti_pec_off_event_handler(uint32_t chip, uint32_t chip_port)
{
    ssa_poe_ti_port_info_t *port_info;
    
    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);

    port_info = TI_PORT_INFO(chip, chip_port);
    TI_PORT_CTRL_PWON(port_info) = false;

    return POE_E_NONE;
}

/* 处理clsc事件 */
static int ssa_poe_ti_clsc_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    poe_pd_class_t class_status;
    
    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);
    port_info = TI_PORT_INFO(chip, chip_port);

    /* 上电要经历两个检测周期，第二次的过滤掉 */
    if (TI_PORT_CTRL_STATE(port_info) == TI_PORT_ON) {
        POE_DRV_ZLOG_WARN("Ignord the second clsc event, return!\n");
        return POE_E_NONE;
    }
    
    rv = poe_ssa_ti_read_port_class_status(TI_PORT_CTRL_LPORT(port_info), &class_status);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_read_port_detect_class_status failed, rv = %d\n", rv);
    } else {
        TI_PORT_DATA_CLS_STATUS(port_info) = class_status;
    }
    
    POE_DRV_ZLOG_WARN("Port detect = %d, class = %d\n", 
        TI_PORT_DATA_DET_STATUS(port_info), TI_PORT_DATA_CLS_STATUS(port_info));

    /* 正常PD */
    if (TI_PORT_DATA_DET_STATUS(port_info) == DETECT_RESIST_VALID) {
        /* do nothing */
    /* 非标PD */
    } else if (TI_PORT_CONFIG_LEGACY_EN(port_info) 
        && (TI_PORT_DATA_DET_STATUS(port_info) == DETECT_RESIST_LOW 
        || TI_PORT_DATA_DET_STATUS(port_info) == DETECT_RESIST_HIGH)) {
        TI_PORT_DATA_CLS_STATUS(port_info) = PD_CLASS_0;
    /* return others */
    } else {
        POE_DRV_ZLOG_ERROR("Invalid!!!!!!\n");
        return POE_E_NONE;
    }

    switch (TI_PORT_DATA_CLS_STATUS(port_info)) {
    case PD_CLASS_0:
    case PD_CLASS_1:
    case PD_CLASS_2:
    case PD_CLASS_3:
    case PD_CLASS_4:
    case PD_CLASS_5:
    case PD_CLASS_6:
    case PD_CLASS_7:
    case PD_CLASS_8:
    case PD_CLASS_4PLUS_TYPE1:
    case PD_CLASS4_4P_DUAL:
        rv = ssa_poe_ti_port_power_up(chip, chip_port);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_port_power_up failed, rv = %d\n", rv);
            return rv;
        }
        break;
    default:
        /* 走到这里说明可能发生了start事件，这里不管 */
        break;
    }

    return POE_E_NONE;
}

/* 处理detect事件 */
static int ssa_poe_ti_detc_event_handler(uint32_t chip, uint32_t chip_port)
{
    int rv;
    ssa_poe_ti_port_info_t *port_info;
    TPS238x_Detection_Status_t detect_status;

    POE_DRV_ZLOG_WARN("Chip-port[%d, %d]\n", chip, chip_port);
    
    port_info = TI_PORT_INFO(chip, chip_port);
    rv = poe_ssa_ti_read_port_detect_status(TI_PORT_CTRL_LPORT(port_info), &detect_status);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_read_port_detect_class_status failed, rv = %d\n", rv);
        return rv;
    }
    POE_DRV_ZLOG_WARN("Read detect = %d\n", detect_status);

    TI_PORT_DATA_DET_STATUS(port_info) = detect_status;
    
    /* 拔掉PD后恢复正常状态 */
    if (detect_status == DETECT_OPEN_CIRCUIT && TI_PORT_CTRL_STATE(port_info) != TI_PORT_OFF) {
        ssa_poe_ti_clear_port_power_info(port_info);
        ssa_poe_ti_update_system_power();
        ssa_poe_ti_set_port_state(port_info, TI_PORT_OFF, POE_NORMAL, true);
    }

    return POE_E_NONE;
}

static int ssa_poe_ti_pgc_down_event_dispatch(ti_chip_irq_info_t ti_irq_tmp_info[], uint64_t pgc_down_bmp)
{
    int rv;
    uint32_t chip;
    uint32_t chip_port;
    int lport;
    uint32_t event_idx;
    ti_port_irq_info_t *port_irq_info;

    POE_DRV_ZLOG_WARN("Enter!\n");

    rv = POE_E_NONE;
    /* 静态模式下，不管优先级 */
    if (TI_SYS_PM_MODE == POE_PM_STATIC) {
        for (lport = 24; lport >= 0; lport--) {
            if (POE_PBMP_PORT_TEST(pgc_down_bmp, lport) == 0) {
                continue;
            }

            chip = TI_GET_CHIPID(lport);
            chip_port = TI_GET_CHIP_PORT(lport);
            port_irq_info = &(ti_irq_tmp_info[chip].port_irq_info[chip_port]);
            for (event_idx = 0; event_idx < port_irq_info->port_event_num; event_idx++) {
                if (port_irq_info->port_event[event_idx] == TI_PGC_NO_GOOD) {
                    break;
                }
            }
            
            rv += ssa_poe_ti_event_handler_func[TI_PGC_NO_GOOD](chip, chip_port);
            if (rv != POE_E_NONE) {
                POE_DRV_ZLOG_ERROR("ssa_poe_ti_handler_event_func[%d] failed, rv = %d\n", TI_PGC_NO_GOOD, rv);
                continue;
            }
        }

        return rv;
    }
    
    /* 非静态模式下，分级好的端口按照优先级顺序加入处理队列，按优先级上电 */
    SSA_POE_FOR_EACH_LPORT_PRI_LOW_TO_HIGH(lport) {
        if (POE_PBMP_PORT_TEST(pgc_down_bmp, lport) == 0) {
            continue;
        }
        // printf("lport = %d\n", lport);

        chip = TI_GET_CHIPID(lport);
        chip_port = TI_GET_CHIP_PORT(lport);
        port_irq_info = &(ti_irq_tmp_info[chip].port_irq_info[chip_port]);
        for (event_idx = 0; event_idx < port_irq_info->port_event_num; event_idx++) {
            if (port_irq_info->port_event[event_idx] == TI_PGC_NO_GOOD) {
                break;
            }
        }

        rv += ssa_poe_ti_event_handler_func[TI_PGC_NO_GOOD](chip, chip_port);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_handler_event_func[%d] failed, rv = %d\n", TI_PGC_NO_GOOD, rv);
            continue;
        }

    }

    return rv;
}

static int ssa_poe_ti_detect_event_dispatch(ti_chip_irq_info_t ti_irq_tmp_info[], uint64_t pwr_up_bmp)
{
    int rv;
    uint32_t chip;
    uint32_t chip_port;
    uint32_t lport;
    uint32_t event_idx;
    ti_port_irq_info_t *port_irq_info;

    POE_DRV_ZLOG_WARN("Enter!\n");

    rv = POE_E_NONE;
    /* 静态模式下，不管优先级 */
    if (TI_SYS_PM_MODE == POE_PM_STATIC) {
        SSA_POE_FOR_EACH_LPORT(lport) {
            if (POE_PBMP_PORT_TEST(pwr_up_bmp, lport) == 0) {
                continue;
            }

            chip = TI_GET_CHIPID(lport);
            chip_port = TI_GET_CHIP_PORT(lport);
            port_irq_info = &(ti_irq_tmp_info[chip].port_irq_info[chip_port]);
            for (event_idx = 0; event_idx < port_irq_info->port_event_num; event_idx++) {
                if (port_irq_info->port_event[event_idx] == TI_DETECT_CLSC) {
                    break;
                }
            }
            
            rv += ssa_poe_ti_event_handler_func[TI_DETECT_CLSC](chip, chip_port);
            if (rv != POE_E_NONE) {
                POE_DRV_ZLOG_ERROR("ssa_poe_ti_handler_event_func[%d] failed, rv = %d\n", TI_DETECT_CLSC, rv);
                continue;
            }
        }

        return rv;
    }
    
    /* 非静态模式下，分级好的端口按照优先级顺序加入处理队列，按优先级上电 */
    SSA_POE_FOR_EACH_LPORT_PRI_HIGH_TO_LOW(lport) {
        if (POE_PBMP_PORT_TEST(pwr_up_bmp, lport) == 0) {
            continue;
        }
        POE_DRV_ZLOG_WARN("lport = %d\n", lport);

        chip = TI_GET_CHIPID(lport);
        chip_port = TI_GET_CHIP_PORT(lport);
        port_irq_info = &(ti_irq_tmp_info[chip].port_irq_info[chip_port]);
        for (event_idx = 0; event_idx < port_irq_info->port_event_num; event_idx++) {
            if (port_irq_info->port_event[event_idx] == TI_DETECT_CLSC) {
                break;
            }
        }

        rv += ssa_poe_ti_event_handler_func[TI_DETECT_CLSC](chip, chip_port);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_handler_event_func[%d] failed, rv = %d\n", TI_DETECT_CLSC, rv);
            continue;
        }
    }

    return rv;
}

/* 把缓存中的中断事件解析出来进行处理 */
static int poe_ssa_ti_irq_event_handle(ti_chip_irq_info_t ti_irq_tmp_info[])
{
    int rv;
    uint32_t chip;
    uint32_t chip_port;
    uint32_t lport;
    ti_port_irq_info_t *port_irq_info;
    ssa_poe_ti_port_info_t *port_info;
    ti_irq_event_type_t port_event;
    int event_idx;
    uint64_t pwr_up_bmp;
    uint64_t pgc_down_bmp;
    
    POE_DRV_ZLOG_WARN("Enter!\n");
    POE_PBMP_PORT_CLEAR(pwr_up_bmp);
    POE_PBMP_PORT_CLEAR(pgc_down_bmp);
    rv = POE_E_NONE;

    SSA_POE_FOR_EACH_CHIP_AND_CHIP_PORT(chip, chip_port) {
        port_info = TI_PORT_INFO(chip, chip_port);
        lport = TI_PORT_CTRL_LPORT(port_info);
        port_irq_info = &(ti_irq_tmp_info[chip].port_irq_info[chip_port]);
        for (event_idx = 0; event_idx < port_irq_info->port_event_num; event_idx++) {
            port_event = port_irq_info->port_event[event_idx];
            switch (port_event) {
            case TI_IRQ_EVENT_NONE:
                break;
            /* 按端口优先级排序后再加队列,放到后面 */
            case TI_DETECT_CLSC:
                POE_PBMP_PORT_ADD(pwr_up_bmp, lport);
                break;
            
            case TI_PGC_NO_GOOD:
                POE_PBMP_PORT_ADD(pgc_down_bmp, lport);
                break;
            case TI_PGC_GOOD:
            case TI_IFAULT_ILIM:
            case TI_IFAULT_PCUT:
            case TI_DISF:
            case TI_SUPF_TSD:
            case TI_STARTF:
            case TI_SUPF_VPUV:
            case TI_SUPF_VDUV:
            case TI_PEC_ENABLE:
            case TI_PEC_NO_ENABLE:
            case TI_DETECT_DET:
                rv = ssa_poe_ti_event_handler_func[port_event](chip, chip_port);
                if (rv != POE_E_NONE) {
                    POE_DRV_ZLOG_ERROR("ssa_poe_ti_handler_event_func[%d] failed, rv = %d\n", port_event, rv);
                    return rv;
                }
                break;
            default:
                break;
            }
        }
    }

    if (POE_PBMP_PORT_NOT_NULL(pgc_down_bmp)) {
        rv = ssa_poe_ti_pgc_down_event_dispatch(ti_irq_tmp_info, pgc_down_bmp);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_pgc_down_event_dispatch failed, rv = %d\n", rv);
        }
    }

    /* 准备上电的端口按优先级顺序加队列等待上电 */
    if (POE_PBMP_PORT_NOT_NULL(pwr_up_bmp)) {
        rv += ssa_poe_ti_detect_event_dispatch(ti_irq_tmp_info, pwr_up_bmp);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_clsc_event_dispatch failed, rv = %d\n", rv);
        }
    }
   
    return rv;
}

int poe_ssa_irq_handle(void)
{
    int rv;
    rv = poe_ssa_ti_irq_event_handle(irq_info_cache);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_clsc_event_dispatch failed, rv = %d\n", rv);
        return rv;
    }

    return POE_E_NONE;
}

void power_up_test(void)
{
    power_up_port[0] = 4;
    power_up_port[1] = 3;
    power_up_port[2] = 2;
    return;
}
/* 给上电动作 */
static int ssa_poe_ti_power_up(void)
{
    int rv;
    int i;
    
    for (i = 0; i < POWER_UP_PORT_NUM; i++) {
        if (power_up_port[i] != 0) {
           rv = poe_ssa_ti_write_port_power_enable(power_up_port[i], true);
           printf("power up port is : %d\n", power_up_port[i]);
           if (rv != POE_E_NONE) {
                POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_power_enable failed, rv = %d\n", rv);
                continue;
           }
           /* 给power up 动作后，恢复默认值 */
           power_up_port[i] = 0;
        }
    }

    return 0;
}

static int ssa_poe_ti_update_led_mode(void)
{
    int rv;
    uint8_t value;
    
    POE_DRV_ZLOG_WARN("Enter!\n");

    rv = poe_cpld_device_read(SSA_I2C_CPLD_PATH, CPLD_POE_MODE_ADDR, &value, sizeof(uint8_t));
    if (rv < 0) {
        POE_DRV_ZLOG_ERROR("dfd_cpld_read failed, rv = %d\n", rv);
        return POE_E_FAIL;
    }
    POE_DRV_ZLOG_WARN("Read cpld PoeModeReg = 0x%x\n", value);

    /* 1代表switch模式，0代表poe模式 */
    if ((value & 0x1) && (g_poe_ti_led_mode == TI_LED_MODE_POE)) {
        g_poe_ti_led_mode = TI_LED_MODE_SWITCH;
        //ssa_port_set_poe_mode(false);
        (void)ssa_poe_ti_cpld_set_mode_led(TI_LED_MODE_SWITCH);
        POE_DRV_ZLOG_WARN("Set switch mode!\n");
    } else if (!(value & 0x1) && (g_poe_ti_led_mode == TI_LED_MODE_SWITCH)) {
        g_poe_ti_led_mode = TI_LED_MODE_POE;
        //ssa_port_set_poe_mode(true);
        (void)ssa_poe_ti_cpld_set_mode_led(TI_LED_MODE_POE);
        //ssa_poe_ti_set_all_color();
        POE_DRV_ZLOG_WARN("Set PoE mode!\n");
    } else {
        /* nothing */
    }

    return 0;
}

/* 更新端口电流电压功率 */
static int ssa_poe_ti_update_port_data(void)
{
    int rv;
    bool need_push;
    uint32_t chip;
    uint32_t chip_port;
    uint32_t cur;
    uint32_t vol;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("Enter!\n");
    need_push = false;

    SSA_POE_FOR_EACH_CHIP_AND_CHIP_PORT(chip, chip_port) {
        port_info = TI_PORT_INFO(chip, chip_port);
        if (!TI_PORT_DATA_POWER_UP(port_info)) {
            continue;
        }
        
        rv = poe_ssa_ti_get_port_measurements(TI_PORT_CTRL_LPORT(port_info), &cur, &vol);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_WARN("ssa_poe_ti_read_port_current failed, rv = %d\n", rv);
            /* 获取失败，则用原来的值 */
            cur = TI_PORT_DATA_CURRENT(port_info);
            vol = TI_PORT_DATA_VOLTAGE(port_info);
        }
        
        need_push = true;
        TI_PORT_DATA_CURRENT(port_info) = cur;
        TI_PORT_DATA_VOLTAGE(port_info) = vol;
        TI_PORT_DATA_CONSUME_POWER(port_info) = cur * vol / 1000;
        /* 节能模式下，分配功率 = 消耗功率 */
        if (TI_SYS_PM_MODE == POE_PM_ENERGYSAVE) {
            TI_PORT_CTRL_ALLOC_POWER(port_info) = TI_PORT_DATA_CONSUME_POWER(port_info);
        }

        POE_DRV_ZLOG_WARN("Update chip-port[%d, %d] cur = %u, vol = %u\n", 
            chip, chip_port, cur, vol);
    }

    /* 如果有上电端口，才需要通告 */
    if (need_push) {
        /* 再更新系统功率 */
        ssa_poe_ti_update_system_power();
    }
   // ssa_poe_push(POE_PORT_I_CUR_ALL, 0);
    POE_DRV_ZLOG_WARN("Push all ports current\n");

    return POE_E_NONE;
}

/* 更新端口冷却时间，解除异常状态 */
static int ssa_poe_ti_update_port_cool_down(void)
{
    uint32_t chip;
    uint32_t chip_port;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_WARN("Enter!\n");

    if (TI_SYS_COOL_DOWN_TIME > 0) {
        if (--TI_SYS_COOL_DOWN_TIME == 0) {
            SSA_POE_FOR_EACH_CHIP_AND_CHIP_PORT(chip, chip_port) {
                port_info = TI_PORT_INFO(chip, chip_port);
                TI_PORT_CTRL_ALLOC_POWER(port_info) = 0;
                ssa_poe_ti_set_port_state(port_info, TI_PORT_OFF, POE_NORMAL, true);
            }
        }
    }

    SSA_POE_FOR_EACH_CHIP_AND_CHIP_PORT(chip, chip_port) {
        port_info = TI_PORT_INFO(chip, chip_port);
        if (TI_PORT_CTRL_COOL_DOWN_TIME(port_info) == 0) {
            continue;
        }
        
        POE_DRV_ZLOG_WARN("Chip-port[%d, %d] remains cool-down time: %ds\n", 
            chip, chip_port, TI_PORT_CTRL_COOL_DOWN_TIME(port_info));
        if (--TI_PORT_CTRL_COOL_DOWN_TIME(port_info) == 0) {
            /* 重新进入上电逻辑 */
            TI_PORT_CTRL_ALLOC_POWER(port_info) = 0;
            ssa_poe_ti_set_port_state(port_info, TI_PORT_OFF, POE_NORMAL, true);
        }
    }

    return POE_E_NONE;
}

/* 需要在轮询线程中手动使能检测-分级的端口检查 */
static bool ssa_poe_ti_restart_port_check(ssa_poe_ti_port_info_t *port_info)
{
    /* 测试restart对semiauto端口不起作用，用于非标端口(manual模式)即可 */
    if (!TI_PORT_CONFIG_LEGACY_EN(port_info)) {
        return false;
    }

    /* 端口关闭了不使能 */
    if (!TI_PORT_CONFIG_POE_EN(port_info)) {
        return false;
    }
    /* 还在冷却不使能 */
    if (TI_PORT_CTRL_COOL_DOWN_TIME(port_info) > 0) {
        return false;
    }
    /* 已经上电了不使能 */
    if (TI_PORT_CTRL_STATE(port_info) == TI_PORT_ON) {
        return false;
    }

    return true;
}

/* restart 检测分级 */
static int ssa_poe_ti_restart_port(void)
{
    int rv;
    uint8_t chip;
    uint8_t chip_port;
    uint8_t reg_val;
    ssa_poe_ti_port_info_t *port_info;

    POE_DRV_ZLOG_DEBUG("Enter!\n");

    POE_SSA_FOR_EACH_CHIP(chip) {
    reg_val = 0;
    for (chip_port = 1; chip_port <= 2; chip_port++) {
        port_info = TI_PORT_INFO(chip, chip_port);
        if (ssa_poe_ti_restart_port_check(port_info)) {
            /* rcl4-bit7, rcl3-bit6, rcl2-bit5, rcl1-bit4 */
            /* rdet4-bit3, rdet3-bit2, rdet2-bit1, rdet1-bit0 */
            reg_val = reg_val | (0x33 << 2 * (chip_port - 1));
        }
    }

    if (reg_val != 0) {
        POE_DRV_ZLOG_DEBUG("Restart chip = %d, reg_val = 0x%x\n", chip, reg_val);
            rv = tps_ReadI2CReg(TI_GET_I2C(chip), TPS238X_DETECT_CLASS_RESTART_COMMAND, &reg_val);
            if (rv != POE_E_NONE) {
                POE_DRV_ZLOG_ERROR("tps_ReadI2CReg failed, rv = %d\n", rv);
                continue;
            }
        }
    }

    return 0;
}

/* 解析检测分级事件 */
static int ssa_poe_ti_parse_detcls(void)
{
    int rv;
    uint8_t chip, chip_port;
    ti_chip_irq_info_t *chip_irq_info;
    pipe_obj_t *p_pipe;
    int pipe_num;
    
    memset(irq_info_cache, 0, sizeof(irq_info_cache));

    rv = poe_ssa_ti_parse_irq(irq_info_cache);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_irq_detc failed, rv = %d\n", rv);
    }

    int i;
    for (chip = 1; chip <= 12; chip++) {
        for (chip_port = 1; chip_port <= 2; chip_port++) {
            chip_irq_info = &irq_info_cache[chip];
            POE_DRV_ZLOG_INFO("chip num : %d, chip_port : %d\n", chip, chip_port);
            POE_DRV_ZLOG_INFO("port_event_num = %d \n",chip_irq_info->port_irq_info[chip_port].port_event_num);
            for (i = 0; i < TI_IRQ_EVENT_MAX; i++) {
                POE_DRV_ZLOG_INFO("port_event = %d \n",chip_irq_info->port_irq_info[chip_port].port_event[i]);            
            }
        }
    }

    /* 解析完之后，写pipe */
    p_pipe = poe_ssa_get_app_pipe();
    if (p_pipe == NULL) {
        POE_DRV_ZLOG_ERROR("poe_ssa_get_app_pipe fail");
        return 0;
    }
    pipe_num = 1;
    rv = pipe_write(p_pipe, &pipe_num, sizeof(int));
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("pipe_write fail, rv = %d", rv);
        return 0;
    }
    
#if 0
    POE_SSA_FOR_EACH_CHIP(chip) {
        chip_irq_info = &irq_info_cache[chip];
        rv = ssa_poe_ti_parse_irq_detc(chip, chip_irq_info);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_irq_detc failed, rv = %d\n", rv);
        }
        rv = ssa_poe_ti_parse_irq_clasc(chip, chip_irq_info);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_irq_clasc failed, rv = %d\n", rv);
        }
    }
#endif
    return 0;
}
#if 0
/* 解析下电事件 */
static int ssa_poe_ti_parse_power_down(void)
{
    int rv;
    uint8_t chip;
    ti_chip_irq_info_t *chip_irq_info;
    memset(irq_info_cache, 0, sizeof(irq_info_cache));

    POE_SSA_FOR_EACH_CHIP(chip) {
        chip_irq_info = &irq_info_cache[chip];
        rv = ssa_poe_ti_parse_irq_pgc(chip, chip_irq_info);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_irq_pgc failed, rv = %d\n", rv);
        }
    }
    
    return 0;
}
#endif
/* 轮询主线程 */
void *poe_ti_main_thread(void *arg)
{
    uint32_t period_cnt[TI_TASK_MAX];
    ti_thread_task_t task_idx;
    struct timeval tv1;
    unsigned long long start_utime;
    unsigned long long end_utime;

    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, "ssa_poeti_main");

    /* 判断是否配置是否下发完，初始化完 */
    POE_DRV_ZLOG_INFO("Waiting...\n");
    while (!g_ti_sys_inited) {
        sleep(1);
    }
    POE_DRV_ZLOG_INFO("Start!\n");

    memset(period_cnt, 0, sizeof(period_cnt));
    start_utime = 0;
    end_utime = 0;

    /* 走到这里说明已经热重启完了，要通告一下上电端口信息 */
    //ssa_poe_ti_ups_push();
    //usleep(sleep_partical);

    while (1) {
        usleep(TI_POLL_PRTICLE_PERIOD);
        gettimeofday(&tv1, NULL);
        POE_DRV_ZLOG_INFO("*** begin[%ld.%ld].\n", (long)tv1.tv_sec, (long)tv1.tv_usec);
        for (task_idx = 0; task_idx < TI_TASK_MAX; task_idx++) {
            /* 根据不同的task_id，执行不同的任务 */
            if (period_cnt[task_idx]++ == ti_task[task_idx].task_period) {
                gettimeofday(&tv1, NULL);
                start_utime = tv1.tv_sec * 1000000 + tv1.tv_usec;
                POE_DRV_ZLOG_INFO("Task[%d] begin[%ld.%ld]. Interval time = %lluus\n",
                   task_idx, (long)tv1.tv_sec, (long)tv1.tv_usec, start_utime - end_utime);
                POE_DRV_ZLOG_INFO("period_cnt = %d\n",period_cnt[task_idx]);
                (void)ti_task[task_idx].task_func();

                gettimeofday(&tv1, NULL);
                end_utime = tv1.tv_sec * 1000000 + tv1.tv_usec;
                POE_DRV_ZLOG_INFO("Task[%d] end[%ld.%ld]. Runtime = %lluus\n", 
                    task_idx, (long)tv1.tv_sec, (long)tv1.tv_usec, end_utime - start_utime);
            }
            /* 一个轮询周期之后统一清零所有cnt */
            if (period_cnt[TI_RESTART_DET_CLS] == TI_DET_CLS_PERIOD) {
                memset(period_cnt, 0, sizeof(period_cnt));
            }
        }
    }

    return NULL;
}

