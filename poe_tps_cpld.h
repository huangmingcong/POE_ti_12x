/*
 * Description: POE TPS CPLD layer header (MAIN)
 * Copyright: (C)2001-2009 �������.  All rights reserved.
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

#define SSA_I2C_CPLD_PATH         "/dev/cpld0"   /* CPLD�豸��ַ */

/* LED��ģʽ��0-����ģʽ��1-POEģʽ */
typedef enum {
    TI_LED_MODE_SWITCH = 0,
    TI_LED_MODE_POE    = 1
} ti_led_mode_t;

/* CPLD���ӿ� */
int poe_cpld_device_read(char *path, int addr, unsigned char *val, int size);

/* CPLDд�ӿ� */
int poe_cpld_device_write(char *path, int addr, unsigned char *val, int size);

/* ���cpld��reset�Ĵ����Ƿ�ʹ�� */
bool ssa_poe_ti_cpld_check_pse_reset(void);

/* cpld���cpld��ups�Ĵ����Ƿ����������µ� */
bool ssa_poe_ti_cpld_check_ups(void);

/* cpld��λpseоƬ */
int ssa_poe_ti_cpld_reset_pse(bool enable);

/* CPLD��������������Ϲ��繦�� */
int ssa_poe_ti_cpld_set_ups(bool enable);

/* �ж�ϵͳ������������������ */
bool ssa_poe_ti_cpld_check_cool_start(void);

/* ����led��ģʽ */
int ssa_poe_ti_cpld_set_mode_led(ti_led_mode_t led_mode);

#endif