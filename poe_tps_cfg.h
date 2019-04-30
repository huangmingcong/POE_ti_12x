#ifndef __POE_TPS_CFG_H__
#define __POE_TPS_CFG_H__

#include <stdio.h>
#include <tps238x.h>
#include "I2C_functions_ruijie.h"
#include "sys_init.h"

typedef struct poe_ti_port_map_e {
    uint8_t i2c_address;
    uint8_t channel[2];
    uint8_t chip_port;   /* �������һ��chip_port, ����channel��Ӧһ��chip_port */
    uint8_t chipid;
} poe_ti_port_map_t;

//#define TI_PORT_MAP port_map

/* 
 * ����lport���i2c address��chip_port
*/
#define TI_GET_I2C_ADDRESS(_lport)    ti_drv_get_i2c_addr(_lport)
#define TI_GET_CAHHEL_1(_lport)		 ti_drv_get_i2c_ch1(_lport)
#define TI_GET_CAHHEL_2(_lport)		 ti_drv_get_i2c_ch2(_lport)
#define TI_GET_CAHHEL_CFG(_lport)		 ti_drv_get_i2c_ch_cfg(_lport)

/* tps channel �Ͷ˿ڵ�ӳ�� */
poe_ti_port_map_t *poe_ti_get_port_map(void);
int ti_drv_get_i2c_ch1(int lpor);
int ti_drv_get_i2c_ch2(int lpor);
int poe_ti_drv_probe(void); 
int ti_drv_get_i2c_addr(int lport);
int ti_drv_get_i2c_ch_cfg(int lport);
int poe_drv_set_chan(int ch);

/* �Ƿ�Ϊ����phyid �� ����1 ���� ����0 */
int poe_ssa_is_local_port(uint32_t phyid);
int poe_ssa_get_phyid_by_lport(int32_t lport, uint32_t *phyid);
int poe_ssa_get_lport_by_phyid(uint32_t phyid, int32_t *lport);

#endif