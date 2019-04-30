#include "poe_tps_cfg.h"
#include <sw_intf/ddm/libddm.h>
#include "poe_db.h"
#include "poe_ssa_debug.h"

/* i2c地址 */
uint8_t tps2388x_i2cAddList[NUM_OF_TPS2388x * NUM_OF_QUARD] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b};

poe_ti_port_map_t s6120_24mxs_port_map[] = {
    /* i2c_address, channel[2], chip_port */
       {0,      {255,  255}, 0, 0},   /* invalid */
       {0x20,   {3,    4}, 2, 1},     /* lport 1 */
       {0x20,   {1,    2}, 1, 1},     /* lport 2 */
       {0x21,   {3,    4}, 2, 2},     /* lport 3 */
       {0x21,   {1,    2}, 1, 2},     /* lport 4 */
       {0x22,   {3,    4}, 2, 3},     /* lport 5 */
       {0x22,   {1,    2}, 1, 3},     /* lport 6 */
       {0x23,   {3,    4}, 2, 4},     /* lport 7 */
       {0x23,   {1,    2}, 1, 4},     /* lport 8 */
       {0x24,   {3,    4}, 2, 5},     /* lport 9 */
       {0x24,   {1,    2}, 1, 5},     /* lport 10 */
       {0x25,   {3,    4}, 2, 6},     /* lport 11 */
       {0x25,   {1,    2}, 1, 6},     /* lport 12 */
       {0x26,   {3,    4}, 2, 7},     /* lport 13 */
       {0x26,   {1,    2}, 1, 7},     /* lport 14 */
       {0x27,   {3,    4}, 2, 8},     /* lport 15 */
       {0x27,   {1,    2}, 1, 8},     /* lport 16 */
       {0x28,   {3,    4}, 2, 9},     /* lport 17 */
       {0x28,   {1,    2}, 1, 9},     /* lport 18 */
       {0x29,   {3,    4}, 2, 10},     /* lport 19 */
       {0x29,   {1,    2}, 1, 10},     /* lport 20 */
       {0x2a,   {3,    4}, 2, 11},     /* lport 21 */
       {0x2a,   {1,    2}, 1, 11},     /* lport 22 */
       {0x2b,   {3,    4}, 2, 12},     /* lport 23 */
       {0x2b,   {1,    2}, 1, 12},     /* lport 24 */
};

poe_ti_port_map_t *poe_ti_get_port_map(void)
{
    return s6120_24mxs_port_map;
}

static poe_ti_port_map_t *m_port_map = NULL;
static int m_get_chan = 0;

int poe_ti_drv_probe(void) 
{
    m_port_map = s6120_24mxs_port_map;

	return 0;
}

int poe_drv_set_chan(int ch)
{
    m_get_chan = ch;
    
    return 0;
}

int ti_drv_get_i2c_ch_cfg(int lport)
{
    if (m_port_map == NULL) {
        return -1;
    }

    return m_port_map[lport].channel[m_get_chan];
}

int ti_drv_get_i2c_addr(int lport)
{
    if (m_port_map == NULL) {
        return -1;
    }
    
    return m_port_map[lport].i2c_address;
}

int ti_drv_get_i2c_ch1(int lport)
{
    if (m_port_map == NULL) {
        return -1;
    }
    
    return m_port_map[lport].channel[0];
}


int ti_drv_get_i2c_ch2(int lport)
{
    if (m_port_map == NULL) {
        return -1;
    }
    
    return m_port_map[lport].channel[1];
}

int poe_ssa_is_local_port(uint32_t phyid)
{
    bool ret;

    /* 判断是否为本地phyid */
    ret = libddm_is_local_port_ext(phyid);
    if (!ret) {
        return 0;
    } else {
        return 1;
    }

    return 0;
}
int poe_ssa_get_lport_by_phyid(uint32_t phyid, int32_t *lport)
{
    int rv;
    int32_t dev;
    int32_t slot;
    int32_t ddm_lport;
 
    rv = libddm_get_dev_slot_lport_by_phyid_ext(phyid, &dev, &slot, &ddm_lport);
    if (rv != 0) {
        return -1;
    }

    *lport = ddm_lport;
    POE_DRV_ZLOG_WARN("poe_ssa_get_lport_by_phyid, lport = %d\n", ddm_lport);

    return 0;
}

int poe_ssa_get_phyid_by_lport(int32_t lport, uint32_t *phyid)
{
    int32_t unit;
    int32_t port;
    uint32_t lphyid, gphyid;
    int rv;

    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param error, lport = %d", lport);
        return -1;
    }

    rv = libddm_get_unit_port_by_lport_ext(lport, &unit, &port); 
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("libddm_get_unit_port_by_lport_ext error\n");
        return -1;
    }

    POE_DRV_ZLOG_WARN("libddm_get_unit_port_by_lport_ext, lport = %d, unit = %d, port = %d\n", lport, unit, port);
    
    rv = libddm_get_lphyid_by_unit_port_ext(unit, port, &lphyid);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("libddm_get_lphyid_by_unit_port_ext error\n");
        return -1;
    }

    rv = libddm_get_gphyid_by_lphyid_ext(lphyid, &gphyid);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("libddm_get_gphyid_by_lphyid_ext error\n");
        return -1;
    }

    *phyid = gphyid;
    POE_DRV_ZLOG_WARN("poe_ssa_get_phyid_by_lport, phyid = %d\n", gphyid);

    return 0;
}

