/* 
 * Copyright(C) 2018 Ruijie Network. All rights reserved. 
 */

#ifndef _POE_SSA_DEBUG_H_
#define _POE_SSA_DEBUG_H_

/* rg_at */
#include <rg_at/ssat_intf.h>
/* zlog */
#include "poe_db.h"
#include "poe_pub.h"

#define POE_CHK_RET_RETURN_VOID(ret, fmt, args...) \
do {\
    if ((ret) != 0) {\
        POE_DRV_ZLOG_ERROR(fmt, ##args);\
        return ret;\
    }\
} while(0)

extern rgdf_zlog_level_t g_poe_drv_zlog_level;
extern zlog_category_t *g_poe_drv_zlog_category;

int ssa_poe_ut_init(rg_global_t *rg_global);

int poe_zlog_init(void);

#endif    /* _POE_SSA_DEBUG_H_ */

