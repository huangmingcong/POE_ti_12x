/*
 * Description: POE TPS CPLD layer header (MAIN)
 * Copyright: (C)2001-2009 �������.  All rights reserved.
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
 * poe_cpld_device_read - ��cpld�Ĵ���
 * @path: i2c cpld path
 * @addr��cpld�Ĵ�����ַ
 * @val��������ֵ
 * @size�������ĳ���
 *
 * ����ֵ:
 *     �ɹ�����0,ʧ�ܷ��ظ���
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
 * poe_cpld_device_write - дcpld�Ĵ��� 
 * @path: i2c cpld path
 * @addr��cpld�Ĵ�����ַ
 * @val��д���ֵ
 * @size��д��ĳ���
 *
 * ����ֵ:
 *     �ɹ�����0,ʧ�ܷ��ظ���
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
 * ssa_poe_ti_cpld_check_pse_reset - ���cpld��reset�Ĵ����Ƿ�ʹ��
 *
 * note: ֵΪ1����������͵�ƽ����оƬresetס��
 * ֵΪ0����������ߵ�ƽ�������Ѿ��ͷ�reset��
 *
 * return: �Ƿ���ture,�񷵻�false
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
 * ssa_poe_ti_cpld_check_ups - cpld���cpld��ups�Ĵ����Ƿ����������µ�
 *
 * return: �Ƿ���ture,�񷵻�false
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
 * ssa_poe_ti_cpld_reset_pse - cpld��λpseоƬ
 * @enable: trueʹ�ܸ�λ��false�ͷŸ�λ
 *
 * return: �ɹ�����SS_E_NONE,ʧ�ܷ��ظ���
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
 * ssa_poe_ti_cpld_set_ups - CPLD��������������Ϲ��繦��
 * @enable: �Ƿ�������������Ϲ���
 *
 * ����ֵ:
 *     �ɹ�����SS_E_NONE,ʧ�ܷ��ظ���
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

/* �ж�ϵͳ������������������ �Ϲ�����ʹ�� */
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
   /* �����Ϲ�����һ�μ��� */
   if (value != 0x1) {
       return false;
   }

   return true;
}

/**
 * ssa_poe_ti_cpld_set_mode_led - ����led��ģʽ
 * @led_mode: led��ģʽ
 *
 * return: �ɹ�����SS_E_NONE,ʧ�ܷ��ظ���
 */
int ssa_poe_ti_cpld_set_mode_led(ti_led_mode_t led_mode)
{
    int rv;
    uint8_t value;

    rv = poe_cpld_device_read(SSA_I2C_CPLD_PATH, TI_CPLD_UPS_ADDR, &value, sizeof(uint8_t));
    if (rv < 0) {
        printf("poe_cpld_device_read failed, rv = %d\n", rv);
    }
    
    /* bit3:��ɫ�ƣ�bit2:��ɫ�ƣ�bit3-2��00��ʾ��ɫ�ƣ�01 ��ʾ��ɫ�� */
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

