#ifndef _POE_SSA_INIT_
#define _POE_SSA_INIT_

#define SSA_POE_FOR_EACH_LPORT(lport)   for ((lport) = 1; (lport) <= 24; (lport)++)


int poe_ssa_ddm_init(rg_global_t *global, char *name);

int poe_ssa_init(void);

int poe_ssa_dep_mom_init(void);

/* ssa poe 系统信息初始化 */
int ssa_poe_ti_sys_info_init(void);

#endif

