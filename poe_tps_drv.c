
#include "poe_tps_drv.h"
#include "poe_ssa_debug.h"
#include <tps238x.h>
#include "poe_tps_cfg.h"

#define DRV_CALL(__FUNC, __ARG) \
        (poe_drv_set_chan(0), __FUNC __ARG); \
        (poe_drv_set_chan(1), __FUNC __ARG)

#define DRV_CALL_2CH(_expr) (poe_drv_set_chan(0), _expr) | (poe_drv_set_chan(1), _expr)

typedef int (*parse_func_t)(uint8_t i2c_addr, ti_chip_irq_info_t *chip_irq_info);

static int ssa_poe_ti_parse_irq_pec(uint8_t chip, ti_chip_irq_info_t *chip_irq_info);
static int ssa_poe_ti_parse_irq_pgc(uint8_t chip, ti_chip_irq_info_t *chip_irq_info);
static int ssa_poe_ti_parse_irq_disf(uint8_t chip, ti_chip_irq_info_t *chip_irq_info);
static int ssa_poe_ti_parse_irq_detc(uint8_t chip, ti_chip_irq_info_t *chip_irq_info);
static int ssa_poe_ti_parse_irq_clasc(uint8_t chip, ti_chip_irq_info_t *chip_irq_info);
static int ssa_poe_ti_parse_irq_ifault(uint8_t chip, ti_chip_irq_info_t *chip_irq_info);
static int ssa_poe_ti_parse_irq_startf(uint8_t chip, ti_chip_irq_info_t *chip_irq_info);
static int ssa_poe_ti_parse_irq_supf(uint8_t chip, ti_chip_irq_info_t *chip_irq_info);

/* 根据中断类型，解析出在哪个端口发生了哪个事件 */
static parse_func_t ssa_poe_ti_parse_irq_func[TI_IRQ_MAX] = {
    ssa_poe_ti_parse_irq_pec,
    ssa_poe_ti_parse_irq_pgc,
    ssa_poe_ti_parse_irq_disf,
    ssa_poe_ti_parse_irq_detc,
    ssa_poe_ti_parse_irq_clasc,
    ssa_poe_ti_parse_irq_ifault,
    ssa_poe_ti_parse_irq_startf,
    ssa_poe_ti_parse_irq_supf,
};

/* 解析power事件寄存器 */
static int ssa_poe_ti_parse_power_event(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    int chip_port;
    uint8_t pwr_evt_reg_val;
    uint8_t pwr_reg_val;
    uint8_t port_offset_bit;
    uint32_t event_idx;
    
    POE_DRV_ZLOG_DEBUG("chip = %x.\n", chip);

    rv = tps_ReadI2CReg(TI_GET_I2C(chip), TPS238X_POWER_EVENT_CLEAR_COMMAND, &pwr_evt_reg_val);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("tps_ReadI2CReg failed, rv = %d\n", rv);
        return rv;
    }

    /* 已经处理过了 */
    if (pwr_evt_reg_val == 0) {
        return POE_E_NONE;
    }

    rv = tps_ReadI2CReg(TI_GET_I2C(chip), TPS238X_POWER_STATUS_COMMAND, &pwr_reg_val);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("tps_ReadI2CReg failed, rv = %d\n", rv);
        return rv;
    }
    POE_DRV_ZLOG_DEBUG("Read PwrEvtReg = 0x%x, PwrReg = 0x%x.\n", pwr_evt_reg_val, pwr_reg_val);

    for (chip_port= 1; chip_port <= 2; chip_port++) {
        /* bit3-pec4, bit2-pec3, bit1-pec2, bit0-pec1 */
        port_offset_bit = 0x3 << 2 * (chip_port - 1);
        /* 先判断是否发生PEC事件 */
        if (pwr_evt_reg_val & port_offset_bit) {
            event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
            /* 如果是，则再判断是上电事件还是下电事件 */
            if (pwr_reg_val & port_offset_bit) {
                chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_PEC_ENABLE;
                POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs PEC-ON[%d] event, CurEventNum = %d.\n", 
                    chip, chip_port, TI_PEC_ENABLE,
                    chip_irq_info->port_irq_info[chip_port].port_event_num);
            } else {
                chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_PEC_NO_ENABLE;
                POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs PEC-OFF[%d] event, CurEventNum = %d.\n", 
                    chip, chip_port, TI_PEC_NO_ENABLE,
                    chip_irq_info->port_irq_info[chip_port].port_event_num);
            }
        }

        /* bit7-pgc4, bit6-pgc3, bit5-pec1, bit4-pgc1 */
        port_offset_bit = 0x3 << (4 + 2 * (chip_port - 1));
        /* 判断是否发生PGC事件 */
        if (pwr_evt_reg_val & port_offset_bit) {
            event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
            /* 判断是上电事件还是下电事件 */
            if (pwr_reg_val & port_offset_bit) {
                chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_PGC_GOOD;
                POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs PGC-UP[%d] event, CurEventNum = %d.\n", 
                    chip, chip_port, TI_PGC_GOOD,
                    chip_irq_info->port_irq_info[chip_port].port_event_num);
            } else {
                chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_PGC_NO_GOOD;
                POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs PGC-DOWN[%d] event, CurEventNum = %d.\n", 
                    chip, chip_port, TI_PGC_NO_GOOD,
                    chip_irq_info->port_irq_info[chip_port].port_event_num);
            }
        }
    }

    return POE_E_NONE;
}

/* 解析fault事件寄存器 */
static int ssa_poe_ti_parse_fault_event(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    int chip_port;
    uint8_t reg_val;
    uint8_t port_offset_bit;
    uint32_t event_idx;

    POE_DRV_ZLOG_DEBUG("chip = %x.\n", chip);

    rv = tps_ReadI2CReg(TI_GET_I2C(chip), TPS238X_FAULT_EVENT_CLEAR_COMMAND, &reg_val);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_drv_read_i2c failed, rv = %d\n", rv);
        return rv;
    }
    POE_DRV_ZLOG_DEBUG("Read FaultEvtReg = 0x%x.\n", reg_val);
    
    if (reg_val == 0) {
        return POE_E_NONE;
    }

    for (chip_port = 1; chip_port <= 2; chip_port++) {
        #if 0 /* pcut 4 pair 不在这个寄存器判断 */
        /* bit3-icut4, bit2-icut3, bit1-icut2, bit0-icut1 */
        port_offset_bit = 0x1 << (chip_port - 1);
        if (reg_val & port_offset_bit) {
            event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
            chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_IFAULT_ICUT;
            POE_DRV_ZLOG_DEBUG("Chip-port[%d, %d] occurs ICUT event, CurEventNum = %d.\n", 
                i2c_addr, chip_port, chip_irq_info->port_irq_info[chip_port].port_event_num);
        }
        #endif
        /* bit7-disf4, bit6-disf3, bit5-disf2, bit4-disf1 */
        port_offset_bit = 0x3 << (4 + 2 * (chip_port - 1));
        /* 判断是否发生disf事件 */
        if (reg_val & port_offset_bit) {
            event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
            chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_DISF;
            POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs DISF[%d] event, CurEventNum = %d.\n", 
                chip, chip_port, TI_DISF, 
                chip_irq_info->port_irq_info[chip_port].port_event_num);
        }
    }

    return POE_E_NONE;
}

/* 解析detect事件寄存器 */
static int ssa_poe_ti_parse_detect_event(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    uint32_t chip_port;
    uint32_t event_idx;
    uint8_t reg_val;
    uint8_t port_offset_bit;
    
    POE_DRV_ZLOG_DEBUG("chip = %x.\n", chip);

    rv = tps_ReadI2CReg(TI_GET_I2C(chip), TPS238X_DETECTION_EVENT_CLEAR_COMMAND, &reg_val);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_drv_read_i2c failed, rv = %d\n", rv);
        return rv;
    }
    POE_DRV_ZLOG_DEBUG("Read DetEventReg = 0x%x.\n", reg_val);

    if (reg_val == 0) {
        return POE_E_NONE;
    }

    for (chip_port= 1; chip_port <= 2; chip_port++) {
        /* 检测事件 */
        port_offset_bit = 0x3 << 2 * (chip_port - 1);
        if (reg_val & port_offset_bit) {
            event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
            chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_DETECT_DET;
            POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs DET[%d] event, CurEventNum = %d.\n", 
                chip, chip_port, TI_DETECT_DET, 
                chip_irq_info->port_irq_info[chip_port].port_event_num);
        }

        /* 分级事件 */
        port_offset_bit = 0x3 << (4 + 2 * (chip_port - 1));
        if (reg_val & port_offset_bit) {
            event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
            chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_DETECT_CLSC;
            POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs CLSC[%d] event, CurEventNum = %d.\n", 
                chip, chip_port, TI_DETECT_CLSC, 
                chip_irq_info->port_irq_info[chip_port].port_event_num);
        }
    }

    return POE_E_NONE;
}

/* 解析过流事件寄存器 */
static int ssa_poe_ti_parse_start_ilim_event(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    int chip_port;
    uint8_t reg_val;
    uint8_t port_offset_bit;
    uint32_t event_idx;
    
    POE_DRV_ZLOG_DEBUG("chip = %x.\n", chip);
    
    rv = tps_ReadI2CReg(TI_GET_I2C(chip), TPS238X_START_LIMIT_EVENT_CLEAR_COMMAND, &reg_val);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("tps_ReadI2CReg failed, rv = %d\n", rv);
        return rv;
    }
    POE_DRV_ZLOG_DEBUG("Read StatIlimReg = 0x%x.\n", reg_val);
    
    if (reg_val == 0) {
        return POE_E_NONE;
    }

    for (chip_port = 1; chip_port <= 2; chip_port++) {
        /* bit7-ilim4, bit6-ilim3, bit5-ilim2, bit4-ilim1 */
        port_offset_bit = 0x3 << (4 + 2 * (chip_port - 1));
        /* 判断是否发生Ilim事件 */
        if (reg_val & port_offset_bit) {
            event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
            chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_IFAULT_ILIM;
            POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs ILIM[%d] event, CurEventNum = %d.\n", 
                chip, chip_port, TI_IFAULT_ILIM, 
                chip_irq_info->port_irq_info[chip_port].port_event_num);
        }

        /* bit3-strt4, bit2-strt3, bit1-strt2, bit0-strt1 */
        port_offset_bit = 0x3 << 2 * (chip_port - 1);
        /* 判断是否发生startf事件 */
        if (reg_val & port_offset_bit) {
            event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
            chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = TI_STARTF;
            POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs STARTF[%d] event, CurEventNum = %d.\n", 
                chip, chip_port, TI_STARTF, chip_irq_info->port_irq_info[chip_port].port_event_num);
        }
    }
    
    return POE_E_NONE;
}

/* 解析pec中断 */
static int ssa_poe_ti_parse_irq_pec(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    
    POE_DRV_ZLOG_DEBUG("chip = %d.\n", chip);

    rv = ssa_poe_ti_parse_power_event(chip, chip_irq_info);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_power_event failed, rv = %d\n", rv);
        return rv;
    }

    return POE_E_NONE;
}

/* 解析pgc中断 */
static int ssa_poe_ti_parse_irq_pgc(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    
    POE_DRV_ZLOG_DEBUG("chip = %d.\n", chip);

    rv = ssa_poe_ti_parse_power_event(chip, chip_irq_info);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_power_event failed, rv = %d\n", rv);
        return rv;
    }

    return POE_E_NONE;
}

/* 解析disf中断 */
static int ssa_poe_ti_parse_irq_disf(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("chip = %d.\n", chip);

    rv = ssa_poe_ti_parse_fault_event(chip, chip_irq_info);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_fault_event failed, rv = %d\n", rv);
        return rv;
    }

    return POE_E_NONE;
}

/* 解析detc中断。这个中断状态位不会产生中断，是在轮询线程中给非标端口用的 */
static int ssa_poe_ti_parse_irq_detc(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    
    POE_DRV_ZLOG_DEBUG("chip = %d.\n", chip);
    
    rv = ssa_poe_ti_parse_detect_event(chip, chip_irq_info);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_detect_event failed, rv = %d\n", rv);
        return rv;
    }

    return POE_E_NONE;
}

/* 解析clasc中断 */
static int ssa_poe_ti_parse_irq_clasc(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    
    POE_DRV_ZLOG_DEBUG("chip = %d.\n", chip);

    rv = ssa_poe_ti_parse_detect_event(chip, chip_irq_info);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_detect_event failed, rv = %d\n", rv);
        return rv;
    }

    return POE_E_NONE;
}

/* 解析ifault中断 */
static int ssa_poe_ti_parse_irq_ifault(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    
    POE_DRV_ZLOG_DEBUG("chip = %x.\n", chip);
    
    rv = ssa_poe_ti_parse_start_ilim_event(chip, chip_irq_info);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_start_ilim_event failed, rv = %d\n", rv);
        //return rv;
    }
    
    rv += ssa_poe_ti_parse_fault_event(chip, chip_irq_info);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_fault_event failed, rv = %d\n", rv);
        return rv;
    }
    
    return POE_E_NONE;
}

/* 解析startf中断 */
static int ssa_poe_ti_parse_irq_startf(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("chip = %x.\n", chip);
    
    rv = ssa_poe_ti_parse_start_ilim_event(chip, chip_irq_info);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_start_ilim_event failed, rv = %d\n", rv);
        return rv;
    }
    
    return POE_E_NONE;
}

/* 解析supf中断 */
static int ssa_poe_ti_parse_irq_supf(uint8_t chip, ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    int chip_port;
    uint8_t reg_val;
    uint32_t event_idx;
    ti_irq_event_type_t supf_event;

    POE_DRV_ZLOG_DEBUG("chip = %x.\n", chip);
    
    rv = tps_ReadI2CReg(TI_GET_I2C(chip), TPS238X_SUPPLY_EVENT_CLEAR_COMMAND, &reg_val);
    if (rv != POE_E_NONE) {
        POE_DRV_ZLOG_ERROR("ssa_poe_drv_read_i2c failed, rv = %d\n", rv);
        return rv;
    }
    POE_DRV_ZLOG_DEBUG("Read SupplyReg = 0x%x.\n", reg_val);

    /* TSD-bit7 */
    if (reg_val & (0x1 << 7)) {
        supf_event = TI_SUPF_TSD;
    /* VDUV-bit6 */
    } else if (reg_val & (0x1 << 6)) {
        supf_event = TI_SUPF_VDUV;
    /* VPUV-bit4 */
    } else if (reg_val & (0x1 << 4)) {
        supf_event = TI_SUPF_VPUV;
    /* OSSE-bit1 */
    } else if (reg_val & (0x1 << 1)) {
        supf_event = TI_SUPF_OSSE;
    /* RAMFLT-bit0 */
    } else if (reg_val & 0x1) {
        supf_event = TI_SUPF_RAMFLT;
    } else {
        supf_event = TI_IRQ_EVENT_NONE;
    }

    for (chip_port = 1; chip_port <= 2; chip_port++) {
        event_idx = chip_irq_info->port_irq_info[chip_port].port_event_num++;
        chip_irq_info->port_irq_info[chip_port].port_event[event_idx] = supf_event;
        POE_DRV_ZLOG_DEBUG("chip-port[%d, %d] occurs supf[%d] event, CurEventNum = %d.\n", 
            chip, chip_port, supf_event, chip_irq_info->port_irq_info[chip_port].port_event_num);
    }
    
    return POE_E_NONE;
}

static int ssa_poe_ti_parse_chip_irq(uint8_t chip, uint8_t irq_register, 
    ti_chip_irq_info_t *chip_irq_info)
{
    int rv;
    int irq_type;

    POE_DRV_ZLOG_DEBUG("chip = %d, irq_reg = 0x%x\n", chip, irq_register);

    rv = POE_E_NONE;
    for (irq_type = TI_IRQ_MAX - 1; irq_type >= 0; irq_type--) {
        if (irq_register & (0x1 << irq_type)) {
            rv += ssa_poe_ti_parse_irq_func[irq_type](chip, chip_irq_info);
            if (rv != POE_E_NONE) {
                POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_irq_func[%d] failed, rv = %d\n", irq_type, rv);
                continue;
            }
        }
    }

    return rv;
}

/* 中断来了以后，调用这个函数把中断对应的事件解析出来，缓存起来 */
int poe_ssa_ti_parse_irq(ti_chip_irq_info_t ti_irq_tmp_info[])
{
    int rv;
    uint8_t reg_val;
    uint8_t chip;
    ti_chip_irq_info_t *chip_irq_info;
    
    POE_DRV_ZLOG_DEBUG("Enter!\n");

    rv = POE_E_NONE;
    POE_SSA_FOR_EACH_CHIP(chip) {
        rv += tps_ReadI2CReg(TI_GET_I2C(chip), TPS238X_INTERRUPT_COMMAND, &reg_val);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("tps_ReadI2CReg failed, rv = %d\n", rv);
            continue;
        }
        POE_DRV_ZLOG_DEBUG("Read chip[%d] IrqReg = 0x%x\n", chip, reg_val);

        if (reg_val == 0) {
            continue;
        }

        chip_irq_info = &ti_irq_tmp_info[chip];
        rv += ssa_poe_ti_parse_chip_irq(chip, reg_val, chip_irq_info);
        if (rv != POE_E_NONE) {
            POE_DRV_ZLOG_ERROR("ssa_poe_ti_parse_chip_event failed, rv = %d\n", rv);
        }
    }

    return rv;
}

/**
 * poe_ssa_ti_write_port_disconnect_enable - 设置端口断接侦测使能
 * IN:
 * @lport: 逻辑端口号
 * @enable: true为使能false为失能
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_disconnect_enable(uint32_t lport, bool enable)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("lport = %d, enable = %d", lport, enable);

	//判读输入端口是否是合法端口
    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    if (enable) {
        poe_drv_set_chan(CHANNEL_1);		//设置通道
        rv = tps_SetPortDisconnectEnable((uint8_t)lport, TPS_ON);
        poe_drv_set_chan(CHANNEL_2);
        rv += tps_SetPortDisconnectEnable((uint8_t)lport, TPS_ON);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortDisconnectEnable fail, rv:%d", rv);
            return POE_E_FAIL;
        }
    } else {
        poe_drv_set_chan(CHANNEL_1);
        rv = tps_SetPortDisconnectEnable((uint8_t)lport, TPS_OFF);
        poe_drv_set_chan(CHANNEL_2);
        rv += tps_SetPortDisconnectEnable((uint8_t)lport, TPS_OFF);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortDisconnectEnable fail, rv:%d", rv);
            return POE_E_FAIL;
        }
    }

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_write_port_operating_mode - 设置端口操作模式
 * IN:
 * @lport: 逻辑端口号
 * @opmode: 要设置的操作模式
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_operating_mode(uint32_t lport, TPS238x_Operating_Modes_t opmode)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("lport = %d, opmode = %d", lport, opmode);

    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    poe_drv_set_chan(CHANNEL_1);
    rv = tps_SetPortOperatingMode(lport, opmode);
    poe_drv_set_chan(CHANNEL_2);
    rv += tps_SetPortOperatingMode(lport, opmode);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_SetPortOperatingMode fail, rv:%d", rv);
        return POE_E_FAIL;
    }

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_write_port_detect_class_enable - 使能端口检测
 * IN:
 * @lport: 逻辑端口号
 * @enable: ture为打开false为关闭
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_detect_class_enable(uint32_t lport, bool enable)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("lport = %d, enable = %d", lport, enable);

    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    if (enable) {
        poe_drv_set_chan(CHANNEL_1);
        rv = tps_SetPortDetectClassEnable(lport, TPS_ON, TPS_ON);
        poe_drv_set_chan(CHANNEL_2);
        rv += tps_SetPortDetectClassEnable(lport, TPS_ON, TPS_ON);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortDetectClassEnable fail, rv:%d", rv);
            return POE_E_FAIL;
        }
    } else {
        poe_drv_set_chan(CHANNEL_1);
        rv = tps_SetPortDetectClassEnable(lport, TPS_OFF, TPS_OFF);
        poe_drv_set_chan(CHANNEL_2);
        rv += tps_SetPortDetectClassEnable(lport, TPS_OFF, TPS_OFF);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortDetectClassEnable fail, rv:%d", rv);
            return POE_E_FAIL;
        }
    }

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_write_lport_reset - 复位功能
 * IN:
 * @lport: 逻辑端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_lport_reset(uint32_t lport)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("lport = %d", lport);

    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    poe_drv_set_chan(CHANNEL_1);
    rv = tps_ResetPort(lport);
    poe_drv_set_chan(CHANNEL_2);
    rv += tps_ResetPort(lport);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_ResetPort fail, rv:%d", rv);
        return POE_E_FAIL;
    }

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_write_port_power_enable - 使能端口上电下点 0x19
 * IN:
 * @lport: 逻辑端口号
 * @pwon: ture为上电false为下电
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_power_enable(uint32_t lport, bool pwon)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("lport = %d, pwon = %d", lport, pwon);

    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    if (pwon) {
        poe_drv_set_chan(CHANNEL_1);
        rv = tps_SetPortPower((uint8_t)lport, TPS_ON);
        poe_drv_set_chan(CHANNEL_2);
        rv += tps_SetPortPower((uint8_t)lport, TPS_ON);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortPower fail, rv:%d", rv);
            return POE_E_FAIL;
        }
    } else {
        poe_drv_set_chan(CHANNEL_1);
        rv = tps_SetPortPower((uint8_t)lport, TPS_OFF);
        poe_drv_set_chan(CHANNEL_2);
        rv += tps_SetPortPower((uint8_t)lport, TPS_OFF);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortPower fail, rv:%d", rv);
            return POE_E_FAIL;
        }
    }
    
    return POE_E_NONE;
}

/**
 * poe_ssa_ti_write_port_pcut_config - 设置端口Pcut
 * IN:
 * @lport: 逻辑端口号
 * @icut: 要设置的icut枚举
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_pcut_config(uint32_t lport, ti_pcut_ctrl_t pcut)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("lport = %d, pcut = %d", lport, pcut);

    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    /* 设置icut值 */
    rv = tps_SetPort4PPolicing((uint8_t)lport, pcut); 
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("tps_SetPort4PPolicing error!, rv:%d", rv);
       return POE_E_FAIL;
    }

    return POE_E_NONE;
}

/**
 * ssa_poe_ti_read_port_pcut_config - 设置端口Pcut
 * IN:
 * @lport: 逻辑端口号
 * @icut: 要设置的icut枚举
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_pcut_config(uint32_t lport, ti_pcut_ctrl_t *pcut)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("lport = %d, pcut = %d", lport, pcut);

    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    /* 设置icut值 */
    rv = tps_GetPort4PPolicing((uint8_t)lport, pcut); 
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("tps_GetPort4PPolicing error!, rv:%d", rv);
       return POE_E_FAIL;
    }

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_port_pcut_enable - 使能端口pcut
 * IN:
 * @lport: 逻辑端口号
 * @enable: ture为打开false为关闭
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_port_pcut_enable(uint32_t lport, bool enable)
{
    int rv;
    uint8_t value;
    uint8_t port_bit;

    POE_DRV_ZLOG_DEBUG("lport = %d, enable = %d", lport, enable);

    if (POE_LPORT_INVALID(lport)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }
    
    /* 4PCUT34:bit3, 4PCUT12:bit2, 奇数端口对应chan3和chan4 */
    if (lport % 2) {
        port_bit = 3;
    } else {
        port_bit = 2;
    }

    if (enable) {
        rv = tps_ReadI2CReg (tps_GetDeviceI2CAddress(lport), TPS238X_4P_DISCONNECT_PCUT_ILIM_CONFIG_COMMAND, &value);
        value = value | (0x1 << port_bit);
        rv += tps_WriteI2CReg (tps_GetDeviceI2CAddress(lport), TPS238X_4P_DISCONNECT_PCUT_ILIM_CONFIG_COMMAND, value);
        if (rv != 0) {
           POE_DRV_ZLOG_ERROR("tps_read write I2CReg error!, rv:%d", rv);
           return POE_E_FAIL;
        }
    } else {
        rv = tps_ReadI2CReg (tps_GetDeviceI2CAddress(lport), TPS238X_4P_DISCONNECT_PCUT_ILIM_CONFIG_COMMAND, &value);
        value = value | (0x0 << port_bit);
        rv += tps_WriteI2CReg (tps_GetDeviceI2CAddress(lport), TPS238X_4P_DISCONNECT_PCUT_ILIM_CONFIG_COMMAND, value);
        if (rv != 0) {
           POE_DRV_ZLOG_ERROR("tps_read write I2CReg error!, rv:%d", rv);
           return POE_E_FAIL;
        }    
    } 

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_write_port_force_on - 设置端口强制供电
 * IN:
 * @lport: 逻辑端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_force_on(uint32_t lport)
{
    int rv;
    bool state;
    
    POE_DRV_ZLOG_DEBUG("lport = %d", lport);

    /* ilim 设置为最大 */
    poe_drv_set_chan(CHANNEL_1);
    rv = tps_SetPortILIM(lport, _2X_ILIM_FOLDBACK_CURVE);
    poe_drv_set_chan(CHANNEL_2);
    rv += tps_SetPortILIM(lport, _2X_ILIM_FOLDBACK_CURVE);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("tps_SetPortILIM error!, rv:%d", rv);
       return POE_E_FAIL;
    }
    
    /* 要manual模式才能强制上电 */
    rv = poe_ssa_ti_write_port_operating_mode(lport, OPERATING_MODE_DIAGNOSTIC);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_operating_mode error!, rv:%d", rv);
       return POE_E_FAIL;
    }

    /* 关闭pcut */
    rv = poe_ssa_ti_port_pcut_enable(lport, false);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_ti_port_pcut_enable error!, rv:%d", rv);
       return POE_E_FAIL;
    }

    /* 关闭disconnect */
    rv = poe_ssa_ti_write_port_disconnect_enable(lport, false);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_disconnect_enable error!, rv:%d", rv);
       return POE_E_FAIL;
    }

    /* check端口是否已经上电 */
    rv = poe_ssa_ti_read_port_power_status(lport, &state);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_ti_read_port_power_status error!, rv:%d", rv);
       return POE_E_FAIL;
    }

    if (!state) {
        /* 给端口上电 */
        usleep(TI_COMMAND_DELAY_SHORT);
        rv = poe_ssa_ti_write_port_power_enable(lport, true);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_power_enable error!, rv:%d", rv);
            return POE_E_FAIL;
        }
    }

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_write_port_force_off - 关闭端口强制供电
 * IN:
 * @lport: 逻辑端口号
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_write_port_force_off(uint32_t lport)
{
    int rv;
    bool state;
    
    POE_DRV_ZLOG_DEBUG("lport = %d", lport);
    /* xxx ilimit 是否要手动设置？*/
    /* check端口是否已经上电 */
    rv = poe_ssa_ti_read_port_power_status(lport, &state);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_ti_read_port_power_status error!, rv:%d", rv);
       return POE_E_FAIL;
    }

    if (state) {
        /* 给端口下电 */
        usleep(TI_COMMAND_DELAY_SHORT);
        rv = poe_ssa_ti_write_port_power_enable(lport, false);
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_power_enable error!, rv:%d", rv);
            return POE_E_FAIL;
        }
    }
    
    /* DC检测使能 */
    rv = poe_ssa_ti_write_port_disconnect_enable(lport, true);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_disconnect_enable error!, rv:%d", rv);
       return POE_E_FAIL;
    }
    
    /* 设置会semiauto模式 */
    rv = poe_ssa_ti_write_port_operating_mode(lport, OPERATING_MODE_SEMI_AUTO);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_operating_mode error!, rv:%d", rv);
       return POE_E_FAIL;
    }
    
    /* 检测使能 */
    rv = poe_ssa_ti_write_port_detect_class_enable(lport, true);
    if (rv != 0) {
       POE_DRV_ZLOG_ERROR("poe_ssa_ti_write_port_operating_mode error!, rv:%d", rv);
       return POE_E_FAIL;
    }

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_read_port_power_status - 读芯片端口供电状态
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @state: 端口供电状态,false为不供电,true为供电
 *
 * 读端口供电状态 POWER_STATUS_REGISTER 10
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_power_status(uint32_t lport, bool *state)
{
    int rv;
    uint32_t status1, status2;
    TPS238x_Ports_t pg;
    TPS238x_Ports_t pe;
    uint8_t chan1, chan2;
    uint8_t i2c_address;

    POE_DRV_ZLOG_DEBUG("lport = %d, pcut = %p", lport, state);

    if (POE_LPORT_INVALID(lport) || state == NULL) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }
    
    i2c_address = TI_GET_I2C_ADDRESS(lport);
    
    rv = tps_GetDevicePowerStatus(i2c_address, &pe, &pg);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_GetDevicePowerStatus error!, rv:%d", rv);
        return -1;
    }
    POE_DRV_ZLOG_DEBUG("poe_ssa_ti_read_port_power_status lport = %d, pg = %d\n", lport, pg);

    /* port in chip */
    chan1 = TI_GET_CAHHEL_1(lport);
    chan2 = TI_GET_CAHHEL_2(lport);

    /* 从PGn 中获取端口供电状态 */
    /* bit1:pg1, bit2:pg2, bit3:pg3,bit4:pg4 */
    status1 = (uint32_t)(pg >> (chan1 - 1) & 0x1);
    status2 = (uint32_t)(pg >> (chan2 - 1) & 0x1);
    /* 两个chan都没供电才算未接pd */
    if (!status1 && !status2) {
        *state = false;
    } else {
        *state = true;
    }
    
    POE_DRV_ZLOG_DEBUG("poe_ssa_ti_read_port_power_status lport = %d, state = %d\n", lport, *state);

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_read_port_resistance - 读端口电阻，kΩ
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @resistance: 芯片端口resistance,不可填NULL
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_resistance(uint32_t lport, uint32_t *resistance)
{
    int rv;
    uint32_t status1, status2;
    uint8_t resist;
    TPS238x_Ports_t pg;
    TPS238x_Ports_t pe;
    uint8_t chip_port_num1, chip_port_num2;
    uint8_t i2c_address;

    i2c_address = TI_GET_I2C_ADDRESS(lport);

    rv = tps_GetDevicePowerStatus(i2c_address, &pe, &pg);
    POE_CHK_RET_RETURN_VOID(rv, "tps_GetDevicePowerStatus error!, rv:%d", rv);

    /* port in chip */
    chip_port_num1 = TI_GET_CAHHEL_1(lport);
    chip_port_num2 = TI_GET_CAHHEL_2(lport);

    /* 从PGn 中获取端口供电状态 */
    status1 = (uint32_t)(pg >> (chip_port_num1 - 1) & 0x1);
    status2 = (uint32_t)(pg >> (chip_port_num2 - 1) & 0x1);
    if (!status1 && !status2) {
        *resistance = 0;
        return 0;
    }

    if (!status1) {
        poe_drv_set_chan(1);
    } else if (!status2) {
        poe_drv_set_chan(0);
    } else {
        /* do nothing */
    }
    
    rv = tps_GetPortDetectResistance(lport, &resist);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_GetPortDetectResistance error!, rv:%d", rv);
    }

    /* 4线对供电时 电阻不需要两个channel相加 k欧姆 */
    *resistance = resist * 1923525 / 10000 / 1000;

    POE_DRV_ZLOG_DEBUG("poe_ssa_ti_read_port_resistance lport = %d, resistance = %d\n", lport, *resistance);

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_read_device_temperature - 读芯片温度, ℃
 * IN:
 * @chip: 芯片号
 * OUT:
 * @temp: 芯片温度,不可填NULL
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_device_temperature(uint32_t i2c_address, uint32_t *temp)
{
    int rv;
    uint8_t temperature;
    POE_DRV_ZLOG_DEBUG("i2c_address = %d, temp = %p", i2c_address, temp);

    if (POE_I2C_ADDRESS_INVALID(i2c_address) || temp == NULL) {
        POE_DRV_ZLOG_ERROR("input param is invalid, i2c_address:%x", i2c_address);
        return POE_E_FAIL;
    }

    rv = tps_GetDeviceTemperature((uint8_t)i2c_address, &temperature);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_GetDeviceTemperature error!, rv:%d", rv);
        return POE_E_FAIL;
    }
    /* 温度寄存器每位代表0.652℃ */
    *temp = temperature * 652 / 1000;

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_read_port_detect_status - 读端口detect状态
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @detect_status: 芯片端口detect状态
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_detect_status(uint32_t lport, TPS238x_Detection_Status_t *detect_status)
{
    int rv;

    if (POE_LPORT_INVALID(lport) || detect_status == NULL) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    rv = tps_GetPortDetectionStatus((uint8_t)lport, detect_status);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_GetPortDetectionStatus error!, rv:%d", rv);
        return -1;
    }

    return POE_E_NONE;
}

int poe_ti_fac_set_port_onoff(int lport, bool mode)
{
    int rv;

    if (mode) {
        rv = DRV_CALL(tps_SetPortOperatingMode, (lport, OPERATING_MODE_SEMI_AUTO));
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortOperatingMode error!, rv:%d", rv);
            return -1;
        }
        /* OFF 模式切换为 AUTO模式 需要使能detect和class一次
         * OFF模式切到semiauto或manual模式，然后接detect/class enable命令要延时1.2ms
         */
        usleep(3000);
        rv = DRV_CALL(tps_SetPortDetectClassEnable, (lport, TPS_ON, TPS_ON));
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortOperatingMode error!, rv:%d", rv);
            return -1;
        }
    } else {
        rv = DRV_CALL(tps_SetPortOperatingMode, (lport, OPERATING_MODE_OFF));
        if (rv != 0) {
            POE_DRV_ZLOG_ERROR("tps_SetPortOperatingMode error!, rv:%d", rv);
            return -1;
        }
    }

    POE_DRV_ZLOG_DEBUG("poe_ti_fac_set_port_onoff lport = %d, mode = %d\n", lport, mode);

    return 0;
}

int poe_ti_fac_check_port_overload(int lport, bool *is_overload)
{
    int rv;
    uint8_t reg_val;
    uint8_t port_offset_bit;
    uint8_t chip_port_num;
    uint8_t i2c_address;

    i2c_address = TI_GET_I2C_ADDRESS(lport);
    /* 延时60ms, 芯片默认60ms过载 */
    usleep(60000);

    rv = tps_GetDevicePCUTFaultEvent(i2c_address, TPS_OFF, &reg_val);
    POE_CHK_RET_RETURN_VOID(rv, "tps_GetDevicePCUTFaultEvent error!, rv:%d", rv);

    /* port in chip */
    chip_port_num = TI_GET_CAHHEL_1(lport);
    port_offset_bit = 0x1 << (chip_port_num - 1);
    if (reg_val & port_offset_bit) {
        *is_overload = true;
    } else {
        *is_overload = false;
    }

    POE_DRV_ZLOG_DEBUG("poe_ti_fac_check_port_overload lport = %d, is_overload = %d\n", lport, *is_overload);

    return 0;
}

int poe_ti_fac_set_port_max_power(int lport, int maxpower)
{
    int rv;
    uint8_t icut;

    if (maxpower == 10000) {
        icut = 8; /* 0000 1000 */
    } else if (maxpower == 30000) {
        icut = 14; /* 0000 1110 */
    } else if (maxpower == 90000) {
        icut = 60; /* 0011 1100 */
    } else {
        POE_DRV_ZLOG_ERROR("only support 10000 or 30000 or 90000 mW\n");
        return -1;
    }

    /* 设置icut值 */
    rv = tps_SetPort4PPolicing(lport, icut); 
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_SetPort4PPolicing error!, rv:%d", rv);
        return -1;
    }

    POE_DRV_ZLOG_DEBUG("poe_ti_fac_set_port_max_power lport = %d, maxpower = %d\n", lport, maxpower);

    return 0;
}

/**
 * poe_ssa_ti_get_port_measurements - 读端口电流值、电压值, mA mV
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @current: 芯片端口current
 * @voltage: 芯片端口current
 *
 * Note: 在POWER_STATUS_REGISTER对应的PGn为1时，才能保证读取到的数值有效
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_get_port_measurements(uint32_t lport, uint32_t *current, uint32_t *voltage)
{
    int rv;
    uint32_t status1, status2;
    uint16_t cur1, cur2;
    uint16_t vol1, vol2;
    TPS238x_Ports_t pg;
    TPS238x_Ports_t pe;
    uint8_t chip_port_num1, chip_port_num2;
    uint8_t i2c_address;

    i2c_address = TI_GET_I2C_ADDRESS(lport);

    rv = tps_GetDevicePowerStatus(i2c_address, &pe, &pg);
    POE_CHK_RET_RETURN_VOID(rv, "tps_GetDevicePowerStatus error!, rv:%d", rv);

    /* port in chip */
    chip_port_num1 = TI_GET_CAHHEL_1(lport);
    chip_port_num2 = TI_GET_CAHHEL_2(lport);

    /* 从PGn 中获取端口供电状态 */
    status1 = (uint32_t)(pg >> (chip_port_num1 - 1) & 0x1);
    status2 = (uint32_t)(pg >> (chip_port_num2 - 1) & 0x1);
    if (!status1 && !status2) {
        *current = 0;
        *voltage = 0;
        return 0;
    }
    poe_drv_set_chan(0);
    rv = tps_GetPortMeasurements(lport, &vol1, &cur1);
    if (rv !=0 ) {
        POE_DRV_ZLOG_ERROR("tps_GetPortMeasurements error!, rv:%d", rv);
    }
    vol1 = vol1 * 3662 / 1000;  /* mV */
    cur1 = cur1 * 70190 / 1000000; /* mA */
    POE_DRV_ZLOG_DEBUG("tps_GetPortMeasurements lport = %d, cur1 = %d, vol1 = %d\n", lport, cur1, vol1);

    poe_drv_set_chan(1);
    rv = tps_GetPortMeasurements(lport, &vol2, &cur2);
    if (rv !=0 ) {
        POE_DRV_ZLOG_ERROR("tps_GetPortMeasurements error!, rv:%d", rv);
    }
    vol2 = vol2 * 3662 / 1000;  /* mV */
    cur2 = cur2 * 70190 / 1000000; /* mA */
    POE_DRV_ZLOG_DEBUG("tps_GetPortMeasurements lport = %d, cur2 = %d, vol2 = %d\n", lport, cur2, vol2);

    /* 4线对供电时 电流两个channel需要相加，电压不需要 */
    *current = cur1 + cur2;
    *voltage = vol1;

    POE_DRV_ZLOG_ERROR("poe_ssa_ti_get_port_measurements lport = %d, current = %d, voltage = %d\n", lport, *current, *voltage);

    return 0;
}

/**
 * poe_ssa_ti_get_port_power - 读端口消耗功率 mW
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @power: 芯片端口power
 *
 * Note: 在POWER_STATUS_REGISTER对应的PGn为1时，才能保证读取到的数值有效
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_get_port_power(int lport, int *power)
{
    int rv;
    uint32_t status1, status2;
    uint16_t cur1, cur2;
    uint16_t vol1, vol2;
    TPS238x_Ports_t pg;
    TPS238x_Ports_t pe;
    uint8_t chip_port_num1, chip_port_num2;
    uint8_t i2c_address;

    i2c_address = TI_GET_I2C_ADDRESS(lport);

    rv = tps_GetDevicePowerStatus(i2c_address, &pe, &pg);
    POE_CHK_RET_RETURN_VOID(rv, "tps_GetDevicePowerStatus error!, rv:%d", rv);

    /* port in chip */
    chip_port_num1 = TI_GET_CAHHEL_1(lport);
    chip_port_num2 = TI_GET_CAHHEL_2(lport);

    /* 从PGn 中获取端口供电状态 */
    status1 = (uint32_t)(pg >> (chip_port_num1 - 1) & 0x1);
    status2 = (uint32_t)(pg >> (chip_port_num2 - 1) & 0x1);
    if (!status1 && !status2) {
        *power = 0;
        //printf("lport[%d] is power off\n", lport);
        return 0;
    }
    poe_drv_set_chan(0);
    rv = tps_GetPortMeasurements(lport, &vol1, &cur1);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_GetPortMeasurements error!, rv:%d", rv);
    }
    vol1 = vol1 * 3662 / 1000;  /* mV */
    cur1 = cur1 * 70190 / 1000000; /* mA */
    POE_DRV_ZLOG_DEBUG("tps_GetPortMeasurements lport = %d, cur1 = %d, vol1 = %d\n", lport, cur1, vol1);

    poe_drv_set_chan(1);
    rv = tps_GetPortMeasurements(lport, &vol2, &cur2);
    if (rv !=0 ) {
        POE_DRV_ZLOG_ERROR("tps_GetPortMeasurements error!, rv:%d", rv);
    }
    vol2 = vol2 * 3662 / 1000;  /* mV */
    cur2 = cur2 * 70190 / 1000000; /* mA */
    POE_DRV_ZLOG_DEBUG("tps_GetPortMeasurements lport = %d, cur2 = %d, vol2 = %d\n", lport, cur2, vol2);

    /* 4线对供电时 电流两个channel需要相加，电压不需要 */
    *power = ((cur1 +cur2) * vol1) / 1000; /* mW */

    POE_DRV_ZLOG_DEBUG("poe_ssa_ti_get_port_power lport = %d, power = %d\n", lport, *power);

    return 0;
}

static int32_t poe_ti_class_chip_to_soft(TPS238x_Classification_Status_t ti_class)
{
    poe_pd_class_t ss_class;

    switch (ti_class) {
    case CLASS_0:
        ss_class = PD_CLASS_0;
        break;
    case CLASS_1:
        ss_class = PD_CLASS_1;
        break;
    case CLASS_2:
        ss_class = PD_CLASS_2;
        break;
    case CLASS_3:
        ss_class = PD_CLASS_3;
        break;
    case CLASS_4:
        ss_class = PD_CLASS_4;
        break;
     case CLASS_5_4P_SINGLE:
        ss_class = PD_CLASS_5;
        break;
    case CLASS_6_4P_SINGLE:
        ss_class = PD_CLASS_6;
        break;
    case CLASS_7_4P_SINGLE:
        ss_class = PD_CLASS_7;
        break;
    case CLASS_8_4P_SINGLE:
        ss_class = PD_CLASS_8;
        break;
    case CLASS_4PLUS_TYPE1: 
        ss_class = PD_CLASS_4PLUS_TYPE1;
        break;
    case CLASS4_4P_DUAL:
        ss_class = PD_CLASS4_4P_DUAL;
        break;
    default:
        ss_class = PD_CLASS_UNKNOWN;
        break;
    }

    return ss_class;
}

/**
 * poe_ssa_ti_read_port_class_status - 读端口class状态
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @port_class: 芯片端口class
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_read_port_class_status(uint32_t lport, int *port_class)
{
    int rv;
    TPS238x_Classification_Status_t class_status;
    
    if (POE_LPORT_INVALID(lport) || port_class == NULL) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }

    rv = tps_GetPortRequestedClassificationStatus(lport, &class_status);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_GetPortRequestedClassificationStatus error!, rv:%d", rv);
        return -1;
    }

    *port_class = poe_ti_class_chip_to_soft(class_status);

    POE_DRV_ZLOG_DEBUG("poe_ssa_ti_read_port_class_status lport = %d, class = %d\n", lport, class_status);

    return 0;
}

/**
 * poe_ssa_ti_check_port_detect_class_enable - 检查端口是否det/cls使能
 * IN:
 * @lport: 逻辑端口号
 * OUT:
 * @enable: ture为打开false为关闭
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - 负数
 */
int poe_ssa_ti_check_port_detect_class_enable(uint32_t lport, bool *enable)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("lport = %d, enable = %p", lport, enable);

    if (POE_LPORT_INVALID(lport) || enable == NULL) {
        POE_DRV_ZLOG_ERROR("input param is invalid, lport:%d", lport);
        return POE_E_FAIL;
    }
    
    poe_drv_set_chan(0);
    rv = tps_GetPortDetectionEnable(lport);
    rv += tps_GetPortClassificationEnable(lport);
    poe_drv_set_chan(1);
    rv += tps_GetPortDetectionEnable(lport);
    rv += tps_GetPortClassificationEnable(lport);
    /* 使能返回TPS_ON 未使能返回TPS_OFF */
    if (rv == 4*TPS_ON) {
        *enable = true;
    } else {
        *enable = false;
    }

    return POE_E_NONE;
}

/**
 * poe_ssa_ti_chip_is_work_normal - 判断TI芯片是否正常工作
 *
 * 判断TI芯片是否正常工作 用于热启动不间断供电
 * 任意读取一块TI芯片的 TPS238X_INTERRUPT_MASK_COMMAND 寄存器值
 * 如果为 , 说明芯片从热启动不间断供电中正常恢复，不需要复位PSE芯片
 *
 * 返回值:
 *     TI芯片正常工作返回TRUE，否则返回FALSE
 */
bool poe_ssa_ti_chip_is_work_normal(uint8_t i2c_addr)
{
    int rv;
    TPS238X_Interrupt_Mask_Register_t reg_val;

    if (POE_I2C_ADDRESS_INVALID(i2c_addr)) {
        POE_DRV_ZLOG_ERROR("input param is invalid, i2c_address:%x", i2c_addr);
        return POE_E_FAIL;
    }

    rv = tps_GetDeviceInterruptMask(i2c_addr, &reg_val);
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("tps_GetDeviceInterruptMask error!, rv:%d", rv);
        return false;
    }

    if (reg_val.SUMSK_Supply_Event_Fault_Unmask == 1 && reg_val.INMSK_Inrush_Fault_Unmask == 1
        && reg_val.IFMSK_IFAULT_Unmask == 1 && reg_val.PEMSK_Power_Enable_Unmask == 1 
        && reg_val.PGMSK_Power_Good_Unmask == 1 && reg_val.CLMSK_Classificiation_Cycle_Unmask == 1
        && reg_val.DEMSK_Detection_Cycle_Unmask == 0 && reg_val.DIMSK_Disconnect_Unmask == 1) {
        return true;
    }

    return false;
}

/* pm变化关闭所有端口上电 */
int poe_ssa_all_port_power_off(void)
{
    uint8_t i2c_addr;
    uint8_t value;
    int rv;

    /* bit4-bit7 off set */
    value = 0xF0;
    POE_SSA_FOR_EACH_I2C(i2c_addr) {
       rv = tps_WriteI2CReg (i2c_addr, TPS238X_POWER_ENABLE_COMMAND, value);
       if (rv != 0) {
           POE_DRV_ZLOG_ERROR("tps_WriteI2CReg fail");
           return rv;
       }
    }

    return 0;
}

/* pm变化重新打开所有端口检测 */
int poe_ssa_all_port_reopen(void)
{
    uint8_t i2c_addr;
    uint8_t value;
    int rv;

    /* bit0-bit7 dect/cls set */
    value = 0xFF;
    POE_SSA_FOR_EACH_I2C(i2c_addr) {
       rv = tps_WriteI2CReg (i2c_addr, TPS238X_DETECT_CLASS_ENABLE_COMMAND, value);
       if (rv != 0) {
           POE_DRV_ZLOG_ERROR("tps_WriteI2CReg fail");
           return rv;
       }
    }

    return 0;
}

/**
 * ssa_poe_ti_recover_chip_config - TI芯片恢复配置
 * @chip  芯片号
 *
 * TI芯片初始化
 *
 * 返回值:
 *     成功 POE_E_NONE, 失败 - SS_E_FAIL
*/
int ssa_poe_ti_recover_chip_config(uint32_t chip)
{
    return 0;
}

/* 芯片初始化 */
static void tps23880_init(TPS238x_Operating_Modes_t opMode)
{
    POE_DRV_ZLOG_DEBUG("tps23880_init enter \n");

    int i;
	int rv;
    TPS238X_Interrupt_Mask_Register_t intMask;

	rv = sramcode_init();
	if (rv != 0) {
        POE_DRV_ZLOG_ERROR("sramcode_init fail");
        return;
	}

    intMask.CLMSK_Classificiation_Cycle_Unmask = 1;
    intMask.DEMSK_Detection_Cycle_Unmask = 0;
    intMask.DIMSK_Disconnect_Unmask = 1;
    intMask.PGMSK_Power_Good_Unmask = 1;
    intMask.PEMSK_Power_Enable_Unmask = 1;
    intMask.IFMSK_IFAULT_Unmask = 1;
    intMask.INMSK_Inrush_Fault_Unmask = 1;
    intMask.SUMSK_Supply_Event_Fault_Unmask = 1;

    //Configure device's interrupt
    for (i = 0; i < NUM_OF_TPS2388x * NUM_OF_QUARD; i++) {
        tps_SetDeviceInterruptMask(tps2388x_i2cAddList[i], intMask);
    }

    //Set all channels in auto mode
    for (i = 0; i < NUM_OF_TPS2388x * NUM_OF_QUARD; i++) {
        tps_SetDeviceOperatingMode(tps2388x_i2cAddList[i], opMode, opMode,
            opMode, opMode);
    }

    //Enable all channel's DC disconnect
    for (i = 0; i < NUM_OF_TPS2388x * NUM_OF_QUARD; i++) {
        tps_SetDeviceDisconnectEnable(tps2388x_i2cAddList[i], TPS238X_ALL_PORTS);
    }

    //Set  4pair ports in 4 pair 90W mode
    for (i = 0; i < NUM_OF_TPS2388x * NUM_OF_QUARD; i++) {
        tps_SetDevice4PPowerAllocation(tps2388x_i2cAddList[i],_4P_90W,_4P_90W);
    }

    // Power off all ports in case we are re-running this application without physically shutting down ports from previous run
    for (i = 0; i < NUM_OF_TPS2388x * NUM_OF_QUARD; i++) {
        tps_SetDevicePowerOff (tps2388x_i2cAddList[i], TPS238X_ALL_PORTS);
    }

    //Enable all channels' detection and classification
    for (i = 0; i < NUM_OF_TPS2388x * NUM_OF_QUARD; i++) {
        tps_SetDeviceDetectClassEnable(tps2388x_i2cAddList[i],TPS238X_ALL_PORTS,TPS238X_ALL_PORTS);
    }
    POE_DRV_ZLOG_DEBUG("tps23880_init end \n");

    return;
}

int poe_ssa_ti_init(void)
{
    int rv;

    POE_DRV_ZLOG_DEBUG("enter poe_ti_fac_init\n ");

    /* ssa poe 系统信息初始化 */
    rv = ssa_poe_ti_sys_info_init();
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("ssa_poe_ti_sys_info_init error\n");
        return -1;
    }
    
    TI_SYS_UPS_EN = ssa_poe_ti_cpld_check_ups();         /* 是否热启不下电 */

    if (!ssa_poe_ti_cpld_check_cool_start()) {
        POE_DRV_ZLOG_INFO("System Warm start!\n");
        if (TI_SYS_UPS_EN && poe_ssa_ti_chip_is_work_normal(0x25)) { /* 热启不下电,且任意某芯片工作正常 */
            /* 恢复数据 */
            POE_DRV_ZLOG_INFO("****poe ups ****\n");
            rv = poe_ssa_ti_ups_recover_data();
            if (rv != POE_E_NONE) {
                POE_DRV_ZLOG_INFO("poe_ssa_ti_ups_recover_data failed, rv = %d\n", rv);
                return rv;
            }
        /* 热启不下电恢复完数据后直接返回，不配置芯片 */
        return POE_E_NONE;
        }
    } else {
        POE_DRV_ZLOG_INFO("System Cool start!\n");  
    }
    /* 芯片初始化，支持修改芯片供电模式 */
    tps23880_init(OPERATING_MODE_SEMI_AUTO);

    return 0;
}

