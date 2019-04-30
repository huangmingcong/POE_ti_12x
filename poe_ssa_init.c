#include "tps238x.h"
#include "poe_ssa_debug.h"
#include "poe_tps_drv.h"
#include "I2C_functions_ruijie.h"
#include "sys_init.h"
#include "poe_tps_cfg.h"
#include "poe_db.h"
#include "ssa_poe_mom.h"
#include "poe_ssa_init.h"
#include <sw_intf/ddm/libddm.h>

ssa_poe_ti_sys_info_t ssa_ti_sys_info;
bool g_ti_sys_inited = false;

/* ddm客户端初始化 */
int poe_ssa_ddm_init(rg_global_t *global, char *name)
{
    int rv;

    rv = libddm_init(global, name);
    if (rv != 0) {
        printf("libddm_init failed, rv = %d", rv);
        return -1;
    }

    return 0;
}

/* 依赖mom初始化 */
int poe_ssa_dep_mom_init(void)
{
    int ret;
    
    ret = poe_port_status_handler_init();
	if (ret != 0) {
		printf("poe_port_status_handler_init fail\n");
		return -1;
	}

    return 0;
}

/**
 * ssa_poe_ti_port_mapping - 设置port到device的映射关系
 *
 * return: 成功 - 0, 其他 - 失败
 */
static int ssa_poe_ti_port_mapping(void)
{
    poe_ti_port_map_t *port_map;
    uint32_t lport;
    
    POE_DRV_ZLOG_DEBUG("\n");

    port_map = poe_ti_get_port_map();
    if (port_map == NULL) {
        printf("ssa_poe_ti_get_port_map NULL\n");
        return POE_E_FAIL;
    } 

    TI_SYS_PORT_MAP = port_map;

    return POE_E_NONE;
}

/* 端口数据初始化 */
static int ssa_poe_ti_lport_info_init(void)
{
    int lport;
    ssa_poe_ti_port_info_t *port_info;

    SSA_POE_FOR_EACH_LPORT(lport) {
        port_info = TI_LPORT_INFO(lport);
        if (port_info == NULL) {
            POE_DRV_ZLOG_ERROR("port info is null %p\n", port_info);
        }

        TI_PORT_CONFIG_POE_EN(port_info)     = true;
        TI_PORT_CONFIG_FORCEON_EN(port_info) = false;
        TI_PORT_CONFIG_LEGACY_EN(port_info)  = false;
        TI_PORT_CONFIG_PRIORITY(port_info)   = POE_PORT_PRI_LOW;
        TI_PORT_CONFIG_MAX_POWER(port_info)  = SSA_POE_MAXPOWER_DEF;
//        TI_PORT_CTRL_LLDP_PDTYPE(port_info)  = POE_LLDP_PDTYPE_CLASS;
        TI_PORT_CTRL_LPORT(port_info)        = lport;
        TI_PORT_CTRL_SUPPORT_ICUT(port_info) = POE_SUPPORT_ICUT_DEFAULT;
    }

    return POE_E_NONE;
}

/* ssa poe 系统信息初始化 */
int ssa_poe_ti_sys_info_init(void)
{
    int rv;

    POE_DRV_ZLOG_INFO("ssa_poe_ti_sys_info_init enter\n");
    memset(&SSA_TI_SYS_INFO, 0, sizeof(SSA_TI_SYS_INFO));

    /* 系统配置信息初始化 */
    TI_SYS_GLB_POE_EN  = true;
    TI_SYS_POWER_EXIST = false;
    TI_SYS_LLDP_EN     = true;
    TI_SYS_UPS_EN      = false;
    TI_SYS_PM_MODE     = POE_PM_ENERGYSAVE;
    TI_SYS_DISC_MODE   = POE_DISCONNECT_DC;
    TI_SYS_POE_TOTAL_POWER = 370000; //先写死
    TI_SYS_PORT_MAP    = NULL;

    /* 端口映射 */
    rv = ssa_poe_ti_port_mapping();
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_port_mapping failed\n");
        return rv;
    }

    /* 端口探测 */
    rv = poe_ti_drv_probe();
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("poe_ti_drv_probe error\n");
        return -1;
    }

    /* 端口信息初始化，要在端口映射完后 */
    (void)ssa_poe_ti_lport_info_init();
    POE_DRV_ZLOG_INFO("ssa_poe_ti_sys_info_init end\n");

    return POE_E_NONE;
}

/* poe ssa 初始化 */
int poe_ssa_init(void)
{
    pthread_t thread_id;
    int ret;

	ret = poe_zlog_init();
	if (ret != 0) {
		printf("poe_zlog_init fail\n");
		return -1;
	}

    ret = poe_ssa_ti_init();
	if (ret != 0) {
		printf("poe_ssa_ti_init fail\n");
		return -1;
	}

    ret = pthread_create(&thread_id, NULL, poe_ti_main_thread, NULL);
    if (ret != 0) {
        printf("pthread_create failed!\n");
        return -1;
    }
    /* 驱动初始化完成标志 */
    g_ti_sys_inited = true;

    return POE_E_NONE;
}    
