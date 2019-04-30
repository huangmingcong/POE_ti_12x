/*
 * Description: POE TPS CPLD layer header (MAIN)
 * Copyright: (C)2001-2009 锐捷网络.  All rights reserved.
 */

/*
 *  poe_tps_cpld.h
 *  Original Author:  zhangzhaobing@ruijie.com.cn, 2018-12
 *  
 *  
 *  History:
 *
 */
 
#ifndef  _POE_TPS_CPLD_H_
#define  _POE_TPS_CPLD_H_

#include <stdio.h>
#include "poe_ssa_debug.h"

#define TI_CPLD_RESET_ADDR           (0x2a)
#define TI_CPLD_UPS_ADDR             (0xcb)
#define TI_CPLD_BROKEN_DOG           (0x0a)
#define TI_CPLD_IRQ_MASK_ADDR        (0x56)
#define TI_CPLD_FAST_SHTDWN_ADDR     (0xb2)
#define CPLD_POE_MODE_LED_ADDR       (0xcb)
#define CPLD_POE_MODE_ADDR           (0xce)

#define TI_CPLD_RESET_BIT_OFFSET     (3)
#define TI_CPLD_POWER_BIT_OFFSET     (0)
#define TI_CPLD_UPS_BIT_OFFSET       (0)
#define TI_CPLD_LED_BIT_OFFSET       (2)
#define TI_CPLD_IRQ_MASK_BIT_OFFSET  (7)
#define TI_CPLD_FASTSHUT_BIT_OFFSET  (0)

#define SSA_I2C_CPLD_PATH         "/dev/cpld0"   /* CPLD设备地址 */

/* LED灯模式，0-交换模式，1-POE模式 */
typedef enum {
    TI_LED_MODE_SWITCH = 0,
    TI_LED_MODE_POE    = 1
} ti_led_mode_t;

/* CPLD读接口 */
int poe_cpld_device_read(char *path, int addr, unsigned char *val, int size);

/* CPLD写接口 */
int poe_cpld_device_write(char *path, int addr, unsigned char *val, int size);

/* 检查cpld的reset寄存器是否被使能 */
bool ssa_poe_ti_cpld_check_pse_reset(void);

/* cpld检查cpld的ups寄存器是否开了热启不下电 */
bool ssa_poe_ti_cpld_check_ups(void);

/* cpld复位pse芯片 */
int ssa_poe_ti_cpld_reset_pse(bool enable);

/* CPLD设置热启动不间断供电功能 */
int ssa_poe_ti_cpld_set_ups(bool enable);

/* 判断系统是冷启动还是热启动 */
bool ssa_poe_ti_cpld_check_cool_start(void);

/* 设置led灯模式 */
int ssa_poe_ti_cpld_set_mode_led(ti_led_mode_t led_mode);

#endif