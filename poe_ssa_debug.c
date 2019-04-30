
#include "poe_ssa_debug.h"
#include "ssa_poe_mom.h"
#include "poe_tps_cfg.h"
#include "poe_tps_drv.h"

zlog_category_t *g_poe_drv_zlog_category = NULL;
rgdf_zlog_level_t g_poe_drv_zlog_level = RGDF_ZLOG_LV_DEBUG;

static void set_debug_level(int dbg_lvl)
{
	if (dbg_lvl < 0 || dbg_lvl > RGDF_ZLOG_LV_MAX) {
		return;
	}
	g_poe_drv_zlog_level = dbg_lvl;

    return;
}

static void set_port_disconnect(int lport, bool enable)
{
    int rv;
    
    rv = poe_ssa_ti_write_port_disconnect_enable(lport, enable);
    if (rv != 0) {
        printf("poe_ssa_ti_write_port_disconnect_enable fail, rv:%d", rv);
        return;
    }

    return;
}

static void set_port_operate_mode(int lport, int mode)
{
    int rv;

    rv = poe_ssa_ti_write_port_operating_mode(lport, mode);
    if (rv != 0) {
        printf("poe_ssa_ti_write_port_operating_mode fail, rv:%d", rv);
        return;
    }

    return;
}

static void set_port_detcls_enable(int lport, bool enable)
{
    int rv;
    
    rv = poe_ssa_ti_write_port_detect_class_enable(lport, enable);
    if (rv != 0) {
        printf("poe_ssa_ti_write_port_detect_class_enable fail, rv:%d", rv);
        return;
    }

    return;
}

static void set_port_reset(int lport)
{
    int rv;
    
    rv = poe_ssa_ti_write_lport_reset(lport);
    if (rv != 0) {
        printf("poe_ssa_ti_write_lport_reset fail, rv:%d", rv);
        return;
    }

    return;
}

static void set_port_power_enable(int lport, int enable)
{
    int rv;
    
    rv = poe_ssa_ti_write_port_power_enable(lport, enable);
    if (rv != 0) {
        printf("poe_ssa_ti_write_port_power_enable fail, rv:%d", rv);
        return;
    }

    return;
}

static void get_port_power_state(int lport)
{
    int rv;
    bool state;
    
    rv = poe_ssa_ti_read_port_power_status(lport, &state);
    if (rv != 0) {
        printf("poe_ssa_ti_read_port_power_status fail, rv:%d", rv);
        return;
    }

    printf("port power state : %d\n", state);

    return;
}

static void get_port_detect(int lport)
{
    int rv;
    TPS238x_Detection_Status_t detect;
    
    rv = poe_ssa_ti_read_port_detect_status(lport, &detect);
    if (rv != 0) {
        printf("poe_ssa_ti_read_port_detect_status fail, rv:%d", rv);
        return;
    }

    printf("port detect : %d\n", detect);

    return;

}

static void get_port_class(int lport)
{
    int rv;
    int port_class;
    
    rv = poe_ssa_ti_read_port_class_status(lport, &port_class);
    if (rv != 0) {
        printf("poe_ssa_ti_read_port_class_status fail, rv:%d", rv);
        return;
    }

    printf("port class : %d\n", port_class);

    return;
}

static void get_port_resistence(int lport)
{
    int rv;
    uint32_t resistance;
    
    rv = poe_ssa_ti_read_port_resistance(lport, &resistance);
    if (rv != 0) {
        printf("poe_ssa_ti_read_port_resistance fail, rv:%d", rv);
        return;
    }

    printf("port resistance : %d k¦¸\n", resistance);

    return;
}

static void get_port_temperature(int lport)
{
    int rv;
    uint32_t temp;
    
    rv = poe_ssa_ti_read_device_temperature(lport, &temp);
    if (rv != 0) {
        printf("poe_ssa_ti_read_device_temperature fail, rv:%d", rv);
        return;
    }

    printf("port temperature : %d ¡æ\n", temp);

    return;
}

static void get_port_status_from_mom(int lport)
{
    int ret, error = 0;
    uint32_t phyid;
    SSsapoe__SsaPoeIntfSta *rg;
    SPoe__PoeIntfIndex poe_intf_index;
    SSsapoe__SsaPoeIntfSta ssa_poe_intf_sta;
    SSsapoe__SsaPoePortSta sta_info = S_SSAPOE__SSA_POE_PORT_STA__INIT;
    app_info_t *app_info;
    
    s_poe__poe_intf_index__init(&poe_intf_index);
    s_ssapoe__ssa_poe_intf_sta__init(&ssa_poe_intf_sta);
    /* lport to phyid */
    ret = poe_ssa_get_phyid_by_lport(lport, &phyid);
    if (ret != 0) {
        POE_DRV_ZLOG_ERROR("poe_ssa_get_phyid_by_lport fail, ret:%d", ret);
        return;
    }

    poe_intf_index.phyid = phyid;
    ssa_poe_intf_sta.index = &poe_intf_index;
    ssa_poe_intf_sta.sta = &sta_info;
    app_info = poe_ssa_get_app_info();

    ret = rg_mom_exists_sync(app_info->rg_global, RG_MOM_ASIC_DB, (const rg_obj*)&ssa_poe_intf_sta);
	if (ret != 0) {
		POE_DRV_ZLOG_ERROR("rg_mom_exists_sync, ret:%d", ret);
	}
    
    rg = (SSsapoe__SsaPoeIntfSta *)rg_mom_get_sync(app_info->rg_global, RG_MOM_ASIC_DB, (const rg_obj*)&ssa_poe_intf_sta, &error);
    if (rg != NULL && ret == 0) {
        printf("power_up :%d\n", rg->sta->power_up);
        printf("class :%d\n", rg->sta->class_);
        printf("detect :%d\n", rg->sta->detect);
        printf("vol :%d mV\n", rg->sta->vol);
        printf("cur :%d mA\n", rg->sta->icut);
        printf("resist :%d k¦¸\n", rg->sta->resis_det);
        printf("power :%d mW\n", rg->sta->power_cons);
        printf("off_reason :%d\n", rg->sta->reason);
    }
    rg_mom_rg_obj_free((rg_obj*)rg);
    
    return;
}

static void set_power_reserve_to_mom(int reserve_power)
{
    int ret;
    app_mom_t *mom;

    SPoe__PoeSysConfInfo sys_conf;
    SPoe__PoeSysIndex sys_index;
    
    s_poe__poe_sys_index__init(&sys_index);
    s_poe__poe_sys_conf_info__init(&sys_conf);

    rg_mom_set_op_t op = { 3 };

    sys_index.op = 1;
    sys_conf.index = &sys_index;
    sys_conf.op = S_POE__POE_OP_E__POE_SYS_POWER_RESERVE;
    sys_conf.value = reserve_power;

    mom = poe_ssa_get_app_mom();
    ret = rg_mom_set_ext_sync(mom->rg_global, RG_MOM_ASIC_DB, &op, (const rg_obj*)&sys_conf); 
    if (ret != 0) {
         POE_DRV_ZLOG_ERROR("rg_mom_set_ext_sync fail, ret:%d", ret);
         return;
    }

    POE_DRV_ZLOG_DEBUG("set_sys_ups_to_mom end, reserve_power=%d\n", reserve_power);

    return;

}
static void set_sys_ups_to_mom(int enable)
{
    int ret;
    app_mom_t *mom;

    SPoe__PoeSysConfInfo sys_conf;
    SPoe__PoeSysIndex sys_index;
    
    s_poe__poe_sys_index__init(&sys_index);
    s_poe__poe_sys_conf_info__init(&sys_conf);

    rg_mom_set_op_t op = { 3 };

    sys_index.op = 1;
    sys_conf.index = &sys_index;
    sys_conf.op = S_POE__POE_OP_E__POE_SYS_UPS_ENABLE;
    sys_conf.value = enable;

    mom = poe_ssa_get_app_mom();
    ret = rg_mom_set_ext_sync(mom->rg_global, RG_MOM_ASIC_DB, &op, (const rg_obj*)&sys_conf); 
    if (ret != 0) {
         POE_DRV_ZLOG_ERROR("rg_mom_set_ext_sync fail, ret:%d", ret);
         return;
    }

    POE_DRV_ZLOG_DEBUG("set_sys_ups_to_mom end, enable=%d\n", enable);

    return;

}
static void set_pm_mode_to_mom(int pm)
{
    int ret;
    app_mom_t *mom;

    SPoe__PoeSysConfInfo sys_conf;
    SPoe__PoeSysIndex sys_index;
    
    s_poe__poe_sys_index__init(&sys_index);
    s_poe__poe_sys_conf_info__init(&sys_conf);

    rg_mom_set_op_t op = { 3 };

    sys_index.op = 1;
    sys_conf.index = &sys_index;
    sys_conf.op = S_POE__POE_OP_E__POE_SYS_PM_MODE;
    sys_conf.value = pm;

    mom = poe_ssa_get_app_mom();
    ret = rg_mom_set_ext_sync(mom->rg_global, RG_MOM_ASIC_DB, &op, (const rg_obj*)&sys_conf); 
    if (ret != 0) {
         POE_DRV_ZLOG_ERROR("rg_mom_set_ext_sync fail, ret:%d", ret);
         return;
    }

    POE_DRV_ZLOG_DEBUG("set_port_enable end, pm_mode=%d\n", pm);

    return;
}

static void set_port_maxpower_to_mom(int lport, int maxpower)
{
    int ret;
    uint32_t phyid;
    app_mom_t *mom;

    if (POE_LPORT_INVALID(lport)) {
      printf("lport %d is error.\n", lport);
      return;
    }

    SPoe__PoeIntfIndex poe_intf_index;
    SPoe__PoeIntfConfInfo poe_intf_conf_info;

    s_poe__poe_intf_index__init(&poe_intf_index);
    s_poe__poe_intf_conf_info__init(&poe_intf_conf_info);
    rg_mom_set_op_t op = { 3 };
    /* lport to phyid */
    ret = poe_ssa_get_phyid_by_lport(lport, &phyid);
    if (ret != 0) {
      POE_DRV_ZLOG_ERROR("poe_ssa_get_phyid_by_lport fail, ret:%d", ret);
      return;
    }

    poe_intf_conf_info.index = &poe_intf_index;
    poe_intf_index.phyid = phyid;
    poe_intf_conf_info.op = S_POE__POE_OP_E__POE_PORT_POWER_MAX;
    poe_intf_conf_info.power_max = maxpower;

    mom = poe_ssa_get_app_mom();
    ret = rg_mom_set_ext_sync(mom->rg_global, RG_MOM_ASIC_DB, &op, (const rg_obj*)&poe_intf_conf_info); 
    if (ret != 0) {
      POE_DRV_ZLOG_ERROR("rg_mom_set_ext_sync fail, ret:%d", ret);
      return;
    }

    POE_DRV_ZLOG_DEBUG("set_port_enable end, lport=%d, maxpower=%d\n", lport, maxpower);

    return;
}

static void set_port_legacy_to_mom(int lport, int enable)
{
    int ret;
    uint32_t phyid;
    app_mom_t *mom;

    if (POE_LPORT_INVALID(lport)) {
       printf("lport %d is error.\n", lport);
       return;
    }

    SPoe__PoeIntfIndex poe_intf_index;
    SPoe__PoeIntfConfInfo poe_intf_conf_info;

    s_poe__poe_intf_index__init(&poe_intf_index);
    s_poe__poe_intf_conf_info__init(&poe_intf_conf_info);
    rg_mom_set_op_t op = { 3 };
    /* lport to phyid */
    ret = poe_ssa_get_phyid_by_lport(lport, &phyid);
    if (ret != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_get_phyid_by_lport fail, ret:%d", ret);
       return;
    }

    poe_intf_conf_info.index = &poe_intf_index;
    poe_intf_index.phyid = phyid;
    poe_intf_conf_info.op = S_POE__POE_OP_E__POE_PORT_LEGACY;
    poe_intf_conf_info.legacy_en = enable;

    mom = poe_ssa_get_app_mom();
    ret = rg_mom_set_ext_sync(mom->rg_global, RG_MOM_ASIC_DB, &op, (const rg_obj*)&poe_intf_conf_info); 
    if (ret != 0) {
       POE_DRV_ZLOG_ERROR("rg_mom_set_ext_sync fail, ret:%d", ret);
       return;
    }

    POE_DRV_ZLOG_DEBUG("set_port_enable end, lport=%d, enable=%d\n", lport, enable);

    return;

}
static void set_port_priority_to_mom(int lport, int prio)
{
    int ret;
    uint32_t phyid;
    app_mom_t *mom;

    if (POE_LPORT_INVALID(lport)) {
       printf("lport %d is error.\n", lport);
       return;
    }

    SPoe__PoeIntfIndex poe_intf_index;
    SPoe__PoeIntfConfInfo poe_intf_conf_info;

    s_poe__poe_intf_index__init(&poe_intf_index);
    s_poe__poe_intf_conf_info__init(&poe_intf_conf_info);
    rg_mom_set_op_t op = { 3 };
    /* lport to phyid */
    ret = poe_ssa_get_phyid_by_lport(lport, &phyid);
    if (ret != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_get_phyid_by_lport fail, ret:%d", ret);
       return;
    }

    poe_intf_conf_info.index = &poe_intf_index;
    poe_intf_index.phyid = phyid;
    poe_intf_conf_info.op = S_POE__POE_OP_E__POE_PORT_PRIORITY;
    poe_intf_conf_info.port_pri = prio;

    mom = poe_ssa_get_app_mom();
    ret = rg_mom_set_ext_sync(mom->rg_global, RG_MOM_ASIC_DB, &op, (const rg_obj*)&poe_intf_conf_info); 
    if (ret != 0) {
       POE_DRV_ZLOG_ERROR("rg_mom_set_ext_sync fail, ret:%d", ret);
       return;
    }

    POE_DRV_ZLOG_DEBUG("set_port_enable end, lport=%d, prio=%d\n", lport, prio);

    return;  
}
static void set_port_force_to_mom(int lport, int enable)
{
    int ret;
    uint32_t phyid;
    app_mom_t *mom;

    if (POE_LPORT_INVALID(lport)) {
       printf("lport %d is error.\n", lport);
       return;
    }

    SPoe__PoeIntfIndex poe_intf_index;
    SPoe__PoeIntfConfInfo poe_intf_conf_info;

    s_poe__poe_intf_index__init(&poe_intf_index);
    s_poe__poe_intf_conf_info__init(&poe_intf_conf_info);
    rg_mom_set_op_t op = { 3 };
    /* lport to phyid */
    ret = poe_ssa_get_phyid_by_lport(lport, &phyid);
    if (ret != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_get_phyid_by_lport fail, ret:%d", ret);
       return;
    }

    poe_intf_conf_info.index = &poe_intf_index;
    poe_intf_index.phyid = phyid;
    poe_intf_conf_info.op = S_POE__POE_OP_E__POE_PORT_FORCE_ON;
    poe_intf_conf_info.force_en = enable;

    mom = poe_ssa_get_app_mom();
    ret = rg_mom_set_ext_sync(mom->rg_global, RG_MOM_ASIC_DB, &op, (const rg_obj*)&poe_intf_conf_info); 
    if (ret != 0) {
       POE_DRV_ZLOG_ERROR("rg_mom_set_ext_sync fail, ret:%d", ret);
       return;
    }

    POE_DRV_ZLOG_DEBUG("set_port_enable end, lport=%d, enable=%d\n", lport, enable);

    return;

}

static void set_port_enable_to_mom(int lport, int enable)
{
    int ret;
    uint32_t phyid;
    app_mom_t *mom;

    if (POE_LPORT_INVALID(lport)) {
        printf("lport %d is error.\n", lport);
        return;
    }

    SPoe__PoeIntfIndex poe_intf_index;
    SPoe__PoeIntfConfInfo poe_intf_conf_info;

    s_poe__poe_intf_index__init(&poe_intf_index);
    s_poe__poe_intf_conf_info__init(&poe_intf_conf_info);
    rg_mom_set_op_t op = { 3 };
    /* lport to phyid */
    ret = poe_ssa_get_phyid_by_lport(lport, &phyid);
    if (ret != 0) {
        POE_DRV_ZLOG_ERROR("poe_ssa_get_phyid_by_lport fail, ret:%d", ret);
        return;
    }

    poe_intf_conf_info.index = &poe_intf_index;
    poe_intf_index.phyid = phyid;
    poe_intf_conf_info.op = S_POE__POE_OP_E__POE_PORT_DETECT_ENABLE;
    poe_intf_conf_info.poe_en = enable;

    mom = poe_ssa_get_app_mom();
    ret = rg_mom_set_ext_sync(mom->rg_global, RG_MOM_ASIC_DB, &op, (const rg_obj*)&poe_intf_conf_info); 
	if (ret != 0) {
		POE_DRV_ZLOG_ERROR("rg_mom_set_ext_sync fail, ret:%d", ret);
		return;
	}

    POE_DRV_ZLOG_DEBUG("set_port_enable end, lport=%d, enable=%d\n", lport, enable);

    return;
}

static void poe_dump_port_info(int lport)
{
    ssa_poe_ti_port_info_t *port_info;
       
   if (POE_LPORT_INVALID(lport)) {
       printf("lport %d is error.\n", lport);
       return;
   }
   
   port_info = TI_LPORT_INFO(lport);

   printf("\n===============Port %d information===============\n", lport);
   printf("Chip:%d  ChipPort:%d\n", TI_GET_CHIPID(lport), TI_GET_CHIP_PORT(lport));
   printf("Configuration        Control             Data    \n");
   printf("--------------------------------------------------------------\n");
   printf("PoE_EN     : %-6s  PWON         : %-3s  Power_up      : %s\n", 
       TI_PORT_CONFIG_POE_EN(port_info) ? "Y" : "N", TI_PORT_CTRL_PWON(port_info) ? "Y" : "N", 
       TI_PORT_DATA_POWER_UP(port_info) ? "Y" : "N");
   printf("ForceOn_EN : %-6s  ICUT_Config  : %-3d  Detect_status : %d\n", 
       TI_PORT_CONFIG_FORCEON_EN(port_info) ? "Y" : "N", TI_PORT_CTRL_ICUT(port_info), 
       TI_PORT_DATA_DET_STATUS(port_info));
   printf("Legacy_EN  : %-6s  Off_reason   : %-3d  Class_status  : %d\n", 
       TI_PORT_CONFIG_LEGACY_EN(port_info) ? "Y" : "N", TI_PORT_CTRL_OFF_REASON(port_info), 
       TI_PORT_DATA_CLS_STATUS(port_info));
   printf("Priority   : %-6d  Port_state   : %-3d  Current       : %umA\n", 
       TI_PORT_CONFIG_PRIORITY(port_info), TI_PORT_CTRL_STATE(port_info), TI_PORT_DATA_CURRENT(port_info));
   printf("Stat_power : %-6d  CLDN_period  : %-3d  Voltage       : %umV\n", 
       TI_PORT_CONFIG_STATIC_POWER(port_info), TI_PORT_CTRL_COOL_DOWN_TIME(port_info), 
       TI_PORT_DATA_VOLTAGE(port_info));
   printf("Max_power  : %-6d  Icut_times   : %-3d  Consume_power : %umW\n", 
       TI_PORT_CONFIG_MAX_POWER(port_info), TI_PORT_CTRL_ICUT_COUNT(port_info), 
       TI_PORT_DATA_CONSUME_POWER(port_info));
   printf("                     Ilim_times   : %-3d  Alloc_power   : %umW\n", 
       TI_PORT_CTRL_ILIM_COUNT(port_info), TI_PORT_CTRL_ALLOC_POWER(port_info));
   printf("                     Istart_times : %-3d  Resistance    : %u\n", 
       TI_PORT_CTRL_ISTRT_COUNT(port_info), TI_PORT_DATA_DET_RESISTANCE(port_info));
   printf("                     LLDP_PDTYPE  : %-3d\n", TI_PORT_CTRL_LLDP_PDTYPE(port_info));
   
   return;
}

static void poe_dump_system_info(void)
{
    printf("\n==========PoE system information==========\n");
    printf("Global_PoE_EN : %s\n", TI_SYS_GLB_POE_EN ? "Y" : "N");
    printf("PoE_power     : %s\n", TI_SYS_POWER_EXIST ? "Y" : "N");
    printf("LLDP_EN       : %s\n", TI_SYS_LLDP_EN ? "Y" : "N");
    printf("UPS_EN        : %s\n", TI_SYS_UPS_EN ? "Y" : "N");
    printf("PM_MODE       : %d\n", TI_SYS_PM_MODE);
    printf("DISC_MODE     : %d\n", TI_SYS_DISC_MODE);
    printf("PoE_total_power        : %umW\n", TI_SYS_POE_TOTAL_POWER);
    printf("PoE_reserve_power_rate : %u%%\n", TI_SYS_RESERVE_PWR_RATE);
    printf("PoE_reserve_power      : %umW\n", TI_SYS_RESERVE_POWER);
    printf("PoE_consume_power      : %umW\n", TI_SYS_CONSUME_POWER);
    printf("PoE_alloc_power        : %umW\n", TI_SYS_ALLOC_POWER);
    printf("PoE_remain_power       : %umW\n", TI_SYS_REMAIN_POWER);

    return;
}
static void poe_dump_ports_status(void)
{
    int i;
    ssa_poe_ti_port_info_t *port_info;
    char pd_class[50];
    char current[50];
    char voltage[50];
    char power[50];
    int totoal_power;

    totoal_power = 0;
    printf("\n=================Ports status information=================\n");
    printf("%-10s%-10s%-10s%-12s%-12s%-15s\n", "port", "status", "class", "current", "voltage", "cons_power");
    for (i = 1; i <= POE_MAX_PORT; i++) {
        port_info = TI_LPORT_INFO(i);
        if (TI_PORT_DATA_CLS_STATUS(port_info) == PD_CLASS_UNKNOWN) {
            sprintf(pd_class, "%s", "N/A");
        } else {
            sprintf(pd_class, "%d", TI_PORT_DATA_CLS_STATUS(port_info));
        }
        sprintf(current, "%d mA", TI_PORT_DATA_CURRENT(port_info));
        sprintf(voltage, "%d mV", TI_PORT_DATA_VOLTAGE(port_info));
        sprintf(power, "%d mW", TI_PORT_DATA_CONSUME_POWER(port_info));

        printf("Te 0/%-5d", i);
        //printf("%-8s", port_info->switch_flag == 0 ? "disable" : "enable");
        printf("%-10s", TI_PORT_DATA_POWER_UP(port_info) ? "ON" : "OFF");
    	printf("%-10s", pd_class);
    	printf("%-12s", current);
    	printf("%-12s", voltage);
    	printf("%-15s\n", power);

        totoal_power += TI_PORT_DATA_CONSUME_POWER(port_info);
    }
    printf("\n------------------------------------\n");
    printf("PSE total power consumption\t: %d mW\n", totoal_power);
 
    return;
}

/* ut debug command init */
int ssa_poe_ut_init(rg_global_t *rg_global) 
{
    int ret;
    char *name = "poe_ssa";
    
    ret = ssat_lib_init(rg_global, name);
    if (!ret) {
        SSAT_FUN_REG(name, get_port_resistence, "lport=%d");
        SSAT_FUN_REG(name, get_port_temperature, "lport=%d");
        SSAT_FUN_REG(name, get_port_class, "lport=%d");
        SSAT_FUN_REG(name, get_port_detect, "lport=%d");
        SSAT_FUN_REG(name, get_port_power_state, "lport=%d");
        SSAT_FUN_REG(name, set_port_disconnect, "lport=%d, enable=%d");
        SSAT_FUN_REG(name, set_port_operate_mode, "lport=%d, mode=%d");
        SSAT_FUN_REG(name, set_port_detcls_enable, "lport=%d, enable=%d");
        SSAT_FUN_REG(name, set_port_reset, "lport=%d");
        SSAT_FUN_REG(name, set_port_power_enable, "lport=%d, enable=%d");
		SSAT_FUN_REG(name, set_debug_level, "dbg_lvl=%d");
        SSAT_FUN_REG(name, poe_dump_ports_status, "void");
        SSAT_FUN_REG(name, poe_dump_system_info, "void");
        SSAT_FUN_REG(name, poe_dump_port_info, "lport=%d");
        SSAT_FUN_REG(name, get_port_status_from_mom, "lport=%d");
        SSAT_FUN_REG(name, set_port_enable_to_mom, "lport=%d, enable=%d");
        SSAT_FUN_REG(name, set_port_force_to_mom, "lport=%d, enable=%d");
        SSAT_FUN_REG(name, set_port_priority_to_mom, "lport=%d, prio=%d");
        SSAT_FUN_REG(name, set_port_legacy_to_mom, "lport=%d, enable=%d");
        SSAT_FUN_REG(name, set_port_maxpower_to_mom, "lport=%d, maxpower=%d");
        SSAT_FUN_REG(name, set_pm_mode_to_mom, "pm=%d");
        SSAT_FUN_REG(name, set_sys_ups_to_mom, "enable=%d");
        SSAT_FUN_REG(name, set_power_reserve_to_mom, "reserve_power=%d");
        SSAT_FUN_REG(name, power_up_test, "void");
    }

    return ret;
}

int poe_zlog_init(void)
{
    g_poe_drv_zlog_category = zlog_init_indep("poe_drv");
    if (!g_poe_drv_zlog_category) {
        printf("zlog_init_indep fail\n");
		return -1;
    }

    return 0;
}

