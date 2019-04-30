/*
 * Description: POE TPS CPLD layer header (MAIN)
 * Copyright: (C)2001-2009 锐捷网络.  All rights reserved.
 */

/*
 *  poe_tps_cpld.c
 *  Original Author:  zhangzhaobing@ruijie.com.cn, 2018-12
 *  
 *  
 *  History:
 *
 */

#include "poe_tps_cpld.h"

/**
 * poe_cpld_device_read - 读cpld寄存器
 * @path: i2c cpld path
 * @addr：cpld寄存器地址
 * @val：读出的值
 * @size：读出的长度
 *
 * 返回值:
 *     成功返回0,失败返回负数
 */
int poe_cpld_device_read(char *path, int addr, unsigned char *val, int size)
{
    int fd;
    int rv;

    if (val == NULL || path == NULL) {
        return -1;
    }

    rv = -1;
    fd = open(path, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        goto fail;
    }

    rv = lseek(fd, addr, 0);
    if (rv < 0) {
        goto exit;
    }

    rv = read(fd, val, size);
    if (rv < 0) {
        goto exit;
    }

exit:
    close(fd);
fail:

    return rv;
}

/**
 * poe_cpld_device_write - 写cpld寄存器 
 * @path: i2c cpld path
 * @addr：cpld寄存器地址
 * @val：写入的值
 * @size：写入的长度
 *
 * 返回值:
 *     成功返回0,失败返回负数
 */
int poe_cpld_device_write(char *path, int addr, unsigned char *val, int size)
{
    int fd;
    int rv;

    if (val == NULL || path == NULL) {
        return -1;
    }

    fd = open(path, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        goto fail;
    }

    rv = lseek(fd, addr, 0);
    if (rv < 0) {
        goto exit;
    }

    rv = write(fd, val, size);
    if (rv < 0) {
        goto exit;
    }
    rv = 0;

exit:
    close(fd);
fail:
    return rv;
}

/**
 * ssa_poe_ti_cpld_check_pse_reset - 检查cpld的reset寄存器是否被使能
 *
 * note: 值为1，代表输出低电平，把芯片reset住了
 * 值为0，代表输出高电平，代表已经释放reset了
 *
 * return: 是返回ture,否返回false
*/
bool ssa_poe_ti_cpld_check_pse_reset(void)
{
    int rv;
    uint8_t value;

    rv = poe_cpld_device_read(SSA_I2C_CPLD_PATH, TI_CPLD_RESET_ADDR, &value, sizeof(uint8_t));
    if (rv < 0) {
        printf("dfd_cpld_write failed, rv = %d\n", rv);
        value = 0;
    }
    printf("Read cpld ResetReg = 0x%x\n", value);
    if (value & (0x1 << TI_CPLD_RESET_BIT_OFFSET)) {
        return true;
    }

    return false;
}

/**
 * ssa_poe_ti_cpld_check_ups - cpld检查cpld的ups寄存器是否开了热启不下电
 *
 * return: 是返回ture,否返回false
*/
bool ssa_poe_ti_cpld_check_ups(void)
{
    int rv;
    uint8_t value;

    rv = poe_cpld_device_read(SSA_I2C_CPLD_PATH, TI_CPLD_UPS_ADDR, &value, sizeof(uint8_t));
    if (rv < 0) {
        printf("dfd_cpld_write failed, rv = %d\n", rv);
        value = 0;
    }
    printf("Read cpld UpsReg = 0x%x\n", value);
    if (value & (0x1 << TI_CPLD_UPS_BIT_OFFSET)) {
        return true;
    }

    return false;
}

/**
 * ssa_poe_ti_cpld_reset_pse - cpld复位pse芯片
 * @enable: true使能复位，false释放复位
 *
 * return: 成功返回SS_E_NONE,失败返回负数
 */
int ssa_poe_ti_cpld_reset_pse(bool enable)
{
    int rv;
    uint8_t value;

    if (enable) {
        value = 0x1 << TI_CPLD_RESET_BIT_OFFSET;
    } else {
        value = 0;
    }
    printf("Write cpld ResetReg = 0x%x\n", value);
    rv = poe_cpld_device_write(SSA_I2C_CPLD_PATH, TI_CPLD_RESET_ADDR, &value, sizeof(uint8_t));
    if (rv < 0) {
        printf("dfd_cpld_write failed, rv = %d\n", rv);
    }

    return rv;
}

/**
 * ssa_poe_ti_cpld_set_ups - CPLD设置热启动不间断供电功能
 * @enable: 是否开启热启动不间断供电
 *
 * 返回值:
 *     成功返回SS_E_NONE,失败返回负数
 */
int ssa_poe_ti_cpld_set_ups(bool enable)
{
    int rv;
    uint8_t value;

    rv = poe_cpld_device_read(SSA_I2C_CPLD_PATH, TI_CPLD_UPS_ADDR, &value, sizeof(uint8_t));
    if (rv < 0) {
        printf("poe_cpld_device_read failed, rv = %d\n", rv);
    }

    if (enable) {
        value = value | (0x1 << TI_CPLD_UPS_BIT_OFFSET);
    } else {
        value = value & ~(0x1 << TI_CPLD_UPS_BIT_OFFSET);
    }
    
    printf("Write cpld UpsReg = 0x%x\n", value);
    rv = poe_cpld_device_write(SSA_I2C_CPLD_PATH, TI_CPLD_UPS_ADDR, &value, sizeof(uint8_t));
    if (rv < 0) {
        printf("poe_cpld_device_write failed, rv = %d\n", rv);
    }

    return rv;
}

/* 判断系统是冷启动还是热启动 断狗不好使？ */
bool ssa_poe_ti_cpld_check_cool_start(void)
{
   int rv;
   uint8_t value;

   rv = poe_cpld_device_read(SSA_I2C_CPLD_PATH, TI_CPLD_BROKEN_DOG, &value, sizeof(uint8_t));
   if (rv < 0) {
       printf("dfd_cpld_write failed, rv = %d\n", rv);
       value = 0;
   }
   printf("Read cpld UpsReg = 0x%x\n", value);
   /* 冷启断狗会有一次计数 */
   if (value != 0x1) {
       return false;
   }

   return true;
}

/**
 * ssa_poe_ti_cpld_set_mode_led - 设置led灯模式
 * @led_mode: led灯模式
 *
 * return: 成功返回SS_E_NONE,失败返回负数
 */
int ssa_poe_ti_cpld_set_mode_led(ti_led_mode_t led_mode)
{
    int rv;
    uint8_t value;

    rv = poe_cpld_device_read(SSA_I2C_CPLD_PATH, TI_CPLD_UPS_ADDR, &value, sizeof(uint8_t));
    if (rv < 0) {
        printf("poe_cpld_device_read failed, rv = %d\n", rv);
    }
    
    /* bit3:绿色灯，bit2:红色灯，bit3-2：00表示黄色灯，01 表示绿色灯 */
    if (led_mode) {
        value = value & ~(0x1 << TI_CPLD_LED_BIT_OFFSET);
    } else {
        value = value | (0x1 << TI_CPLD_LED_BIT_OFFSET);
    }
    
    POE_DRV_ZLOG_DEBUG("Write cpld ModeLedReg = 0x%x\n", value);
    rv = poe_cpld_device_write(SSA_I2C_CPLD_PATH, CPLD_POE_MODE_LED_ADDR, &value, sizeof(uint8_t));
    if (rv != 0) {
        POE_DRV_ZLOG_ERROR("poe_cpld_device_write failed, rv = %d\n", rv);
    }

    return rv;
}

