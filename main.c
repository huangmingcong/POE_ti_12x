#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>

#include "app_proc.h"
#include "app_mom.h"
#include "obj_udp_poe.h"
#include "obj_intf_poe.h"
#include "obj_sys_poe.h"
#include "poe_ssa_init.h"
#include "poe_ssa_debug.h"
#include "app_pipe.h"
#include "ssa_poe_mom.h"
#include "poe_pub.h"
//dddddd
#define SSA_POE_LOG(fmt, arg...)   printf("   SSA   "fmt, ##arg)
#define PIPE_SIZE (sizeof(int))

app_info_t *g_app_info;
app_mom_t *g_app_mom;
pipe_obj_t *g_app_pipe;
int pipe_bit = 1;

app_info_t *poe_ssa_get_app_info(void)
{
    return g_app_info;
}

app_mom_t *poe_ssa_get_app_mom(void)
{
    return g_app_mom;
}

pipe_obj_t *poe_ssa_get_app_pipe(void)
{
    return g_app_pipe;
}

/* poe 流程 */
static int m_demo_db = RG_MOM_ASIC_DB;

static int poe_obj_write_test(void *mom, int db)
{
    int rv;
    
    SPoe__PoeIntfIndex poe_intf_index;
    SSsapoe__SsaPoeIntfSta ssa_poe_intf_sta;
    SSsapoe__SsaPoePortSta sta_info = S_SSAPOE__SSA_POE_PORT_STA__INIT;
    
    s_poe__poe_intf_index__init(&poe_intf_index);
    s_ssapoe__ssa_poe_intf_sta__init(&ssa_poe_intf_sta);

    ssa_poe_intf_sta.index = &poe_intf_index;
    ssa_poe_intf_sta.sta = &sta_info;
    
    sta_info.power_cons = 100;
    
    rv = app_mom_db_set(mom, db, &ssa_poe_intf_sta);
    POE_DRV_ZLOG_INFO("set the data for test rv : %d\n", rv);
	
	return 0;
}

/* 订阅处理函数 */
static int app_mom_pubsub_proc(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj)
{
    int rv = 0;
    rg_obj *obj;
    int msg_size = 0;
    
    obj = (rg_obj *)rgobj;
    msg_size = obj->descriptor->sizeof_message;
    
    POE_DRV_ZLOG_INFO("%s %d %d %d %d %p %p\n", __func__, db, type, flag, cmd, value, rgobj);
    switch(cmd) {
    case RG_MOM_SUBSCRIBE:		
		poe_obj_write_test(mom, db);
        break;
        
    case RG_MOM_SET: 
         if (msg_size == POE_SYS_CONF_MSG_SIZE) {
            POE_DRV_ZLOG_INFO("get sys conf msg from cli, begin to proc\n");
            ssa_poe_sys_conf_proc(mom, db, type, flag, cmd, value, rgobj);
        } else if (msg_size == POE_INTF_CONF_MSG_SIZE) {
            POE_DRV_ZLOG_INFO("get intf conf msg from cli, begin to proc\n");
            ssa_poe_intf_conf_proc(mom, db, type, flag, cmd, value, rgobj);
        } else {
            POE_DRV_ZLOG_INFO("not valid msg, do nothing\n");
        }
        break;
    case RG_MOM_DEL: 
        if (msg_size == POE_SYS_CONF_MSG_SIZE) {
            POE_DRV_ZLOG_INFO("get sys conf msg from cli, begin to proc\n");
        } else if (msg_size == POE_INTF_CONF_MSG_SIZE) {
            POE_DRV_ZLOG_INFO("get intf conf msg from cli, begin to proc\n");
            ssa_poe_intf_conf_proc(mom, db, type, flag, cmd, value, rgobj);
        } else {
            POE_DRV_ZLOG_INFO("not valid msg, do nothing\n");
        }
        break;
    }
    
    return rv;
    
}

/* 订阅数据 */
static int poe_ssa_all_info_sub(app_mom_t *mom, int db)
{
    int rv;
    void *obj;

    rv = POE_E_NONE;

    /* 根据数据定阅不同的消息 */
	if (db == RG_MOM_ASIC_DB) {
        /* 基于全局配置信息 */
        obj = rgobj_poe_sys_conf_info_entry();
        rv = app_mom_db_sub(mom, db, obj);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("sub sys conf error, rv=%d\n", rv);
        }
        /* 基于端口配置信息 */
		obj = rgobj_poe_intf_conf_info_entry();
		rv = app_mom_db_sub(mom, db, obj);
        if (rv != 0) {
		    POE_DRV_ZLOG_ERROR("proxy sub db %d obj %p rgobj_ssapoe_intf_sta_entry\n", db, obj);
        }
    }

    return rv;
}

/* 数据一致性校验 */
static int poe_ssa_all_info_scan(app_mom_t *mom, int db)
{
    int rv;
    void *obj;

    rv = POE_E_NONE;

    /* 根据数据定阅不同的消息 */
	if (db == RG_MOM_ASIC_DB) {
        /* 基于全局配置信息 */
        obj = rgobj_poe_sys_conf_info_entry();
        rv = app_mom_db_scan(mom, db, obj);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("scan sys conf error, rv=%d\n", rv);
        }
        /* 基于端口配置信息 */
		obj = rgobj_poe_intf_conf_info_entry();
		rv = app_mom_db_scan(mom, db, obj);
        if (rv != 0) {
		    POE_DRV_ZLOG_ERROR("scan sub db %d obj %p rgobj_ssapoe_intf_sta_entry\n", db, obj);
        }
    }

    return rv;

}

/* 事件连接函数 */
static int app_mom_conn_proc(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj)
{
    int rv;
    
    POE_DRV_ZLOG_INFO("%s %d %d %d %d %p\n", __func__, db, type, flag, cmd, value);
    rv = POE_E_NONE;
    
    rv = poe_ssa_all_info_sub(mom, db);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("poe_ssa_all_info_sub fail, rv = %d\n", rv);
        return rv;
    }

    rv = poe_ssa_all_info_scan(mom, db);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("poe_ssa_all_info_scan fail, rv = %d\n", rv);
        return rv;
    }
    
    return rv;
}

/* 数据回调函数 */
static int app_mom_event_cback(void *mom, int db, int type, int flag, int cmd, void *value, void *rgobj)
{
    POE_DRV_ZLOG_INFO("\n");
    POE_DRV_ZLOG_INFO("%s %d %d %d %d %p %p\n", __func__, db, type, flag, cmd, value, rgobj);
    int rv = 0;
    
    switch (type) {
        case MOM_CBACK_CONN: {
            POE_DRV_ZLOG_INFO("    db connect cback\n");
            rv = app_mom_conn_proc(mom, db, type, flag, cmd, value, rgobj);
        } break;
 
        case MOM_CBACK_DISCON: {
            POE_DRV_ZLOG_INFO("    db disconnect cback\n");
        } break;

        case MOM_CBACK_PUBSUB: {
            POE_DRV_ZLOG_INFO("    db pubsub cback\n");
            rv = app_mom_pubsub_proc(mom, db, type, flag, cmd, value, rgobj);
        } break;
            
        default: {
            POE_DRV_ZLOG_INFO("    unknown type cback\n");
        } break;
    }
    
    return rv;
}

/* 伪线程socket读fd的回调函数 */
static int poe_rg_thread_read_proc(struct rg_thread *t)
{
    int rv;
    int read_bit;
    struct rg_thread *g_read;
    app_info_t *p_app;
    pipe_obj_t *p_pipe;

    p_pipe = poe_ssa_get_app_pipe();
    p_app = poe_ssa_get_app_info();
    
    rv = pipe_read(p_pipe, &read_bit, PIPE_SIZE);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("pipe_read fail\n");
    }
    
    if (read_bit == pipe_bit) {
        rv = poe_ssa_irq_handle();
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("pipe_read fail\n");
        }
    }
    g_read = rg_thread_add_read_high_withname(p_app->rg_master, 
        poe_rg_thread_read_proc, NULL, p_pipe->readfd, p_pipe->name);
    if (!g_read) {
        printf("g_read is NULL");
        return POE_E_FAIL;
    }
    
    return POE_E_NONE;
}

static int poe_add_read_timer(struct rg_thread_master *g_master, char *name)
{
    struct rg_thread *g_read;
    struct timeval 	timer = {5, 0};

    g_read = NULL;
    g_read = rg_thread_add_timer_timeval_withname(g_master, 
        poe_rg_thread_read_proc, NULL, timer, name);
    if (!g_read) {
        printf("rg_thread_add_timer_withname failed");
        return POE_E_FAIL;
    }

    return POE_E_NONE;
}

/* 主函数 */
int main(int argc, char **argv)
{	
	int rv;
	app_info_t app_info;
	app_info_t *p_app;
    
	app_mom_t mom;
	app_mom_t *p_mom;

    pipe_obj_t pipe;
    pipe_obj_t *p_pipe;
    struct rg_thread *g_read;
    /* pipe 初始化 */
    p_pipe = pipe_init(&pipe, "poe_ssa", PIPE_SIZE, "poe_pipe");
    if (p_pipe == NULL) {
		POE_DRV_ZLOG_ERROR("pipe_init error\n");
		return 0;
	}
    g_app_pipe = p_pipe;

    /* poe ssa 初始化 */
    rv = poe_ssa_init();
    if (rv != 0) {
        printf("poe_ssa_init fail.\n");
        return 0;
    }

	if (argc > 1) {
		m_demo_db = atoi(argv[1]);
		POE_DRV_ZLOG_INFO("set thd db is %d\n", m_demo_db);
	}
	
	p_app = app_proc_initex(&app_info, "poe_ssa");
	if (p_app == NULL) {
		POE_DRV_ZLOG_ERROR("app_proc_initex error\n");
		return 0;
	}

    g_app_info = p_app;
    
    /* 初始化ddm */
    rv = poe_ssa_ddm_init(p_app->rg_global, p_app->name);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("poe_ssa_ddm_init fail");
        return 0;
    }

    /* 初始化ut */
    rv = ssa_poe_ut_init(p_app->rg_global);
	if (rv != 0) {
		POE_DRV_ZLOG_ERROR("poe_fac_ut_init fail, ret:%d", rv);
		return 0;
	}

	p_mom = app_mom_init(&mom, p_app->rg_global, p_app, app_mom_event_cback);
    if (p_mom == NULL) {
        POE_DRV_ZLOG_ERROR("app_mom_init error\n");
		return 0;
    }

    g_app_mom = p_mom;

    /* 依赖mom 初始化 */
    rv = poe_ssa_dep_mom_init();
   if (rv != 0) {
		POE_DRV_ZLOG_ERROR("poe_ssa_dep_mom_init fail, ret:%d", rv);
		return 0;
	}

    /* connect the mom db */
	rv = app_mom_db_conn(p_mom, m_demo_db);
    if (rv != 0) {
		POE_DRV_ZLOG_ERROR("app_mom_db_conn fail, ret:%d", rv);
		return 0;
	}

    /* pipe 读处理 */
    g_read = rg_thread_add_read_high_withname(p_app->rg_master, poe_rg_thread_read_proc, 
        NULL, p_pipe->readfd, p_pipe->name);
    if (!g_read) {
        /* socket读添加失败时，起一个定时器重复添加 */
        printf("!!!!!!rg_thread_add_read_high_withname failed!!!!!!!");
        rv = poe_add_read_timer(p_app->rg_master, p_pipe->name);
        if (rv != POE_E_NONE) {
            close(p_pipe->readfd);
            close(p_pipe->writefd);
            return 0;
        }
    }

	POE_DRV_ZLOG_INFO("app_proc_loop loop the data\n");
	app_proc_loop(p_app);
	
	return 0;
}
