/*
 * Description: I2C FUNCTIONS RUIJIE layer header (MAIN)
 * Copyright: (C)2001-2009 锐捷网络.  All rights reserved.
 */

/*
 *  I2C_functions_ruijie.c
 *  Original Author:  zhangzhaobing@ruijie.com.cn, 2018-12
 *  
 *  
 *  History:
 *
 */
 
#include <I2C_functions_ruijie.h>
#include <string.h>
#include <semaphore.h>                      /* 信号量头文件 */
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/prctl.h>
#include <errno.h>
#include "poe_ssa_debug.h"

static uint32_t poe_i2c_error_num = 0;

#define POE_I2C_NAME                      "/dev/i2c-7"   /* I2C的字符名 */

/* i2c出错处理 */
static void poedrv_add_i2c_error(void)
{
    poe_i2c_error_num++;

    return;
}

/* i2c读测�?*/
static int32_t poe_i2c_ioctl_read(int32_t fd, uint8_t *data, uint32_t i2c, uint16_t reg,
    uint32_t size)
{
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msgs[I2C_RD_MSG_NUM];
    uint8_t buf[POEi2C_IOMSGLEN];
    uint8_t recv_buf[POEi2C_IOMSGLEN];
    int32_t i;
    int32_t rv;

    memset(&ioctl_data, 0, sizeof(ioctl_data));
    memset(msgs, 0, sizeof(msgs));
    memset(buf, 0, sizeof(buf));
    memset(recv_buf, 0, sizeof(recv_buf));

    buf[0] = (uint8_t)(reg);
    msgs[0].len = sizeof(uint8_t);
    msgs[0].buf = buf;
    msgs[0].addr = i2c;

    msgs[1].buf = recv_buf;
    msgs[1].len = size;
    msgs[1].addr = i2c;
    msgs[1].flags |= I2C_M_RD;
    ioctl_data.msgs = &(msgs[0]);
    ioctl_data.nmsgs = I2C_RD_MSG_NUM;

    for (i = 0; i < SSA_POE_OP_CNT; i++) {
        rv= ioctl(fd, I2C_RDWR, (unsigned long)&ioctl_data);
        if(rv < 0) {
            poedrv_add_i2c_error();
            usleep(POE_I2C_SLEEP_TIME);
            continue;
        }
        break;
    }

    if (rv < 0) {
        POE_DRV_ZLOG_DEBUG("ioctl fail(ret:%d, errno:%s)!\n", rv, strerror(errno));
        return -1;
    }

    memcpy(data, recv_buf, size);

    return SSA_POE_E_NONE;
}

/* i2c写测�?*/
static int32_t poe_i2c_ioctl_write(int32_t fd, uint8_t *data, uint32_t i2c, uint16_t reg,
                uint32_t size)
{
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msgs[1];
    uint8_t buf[3];
    int32_t i;
    int32_t rv;
    uint32_t addrlen;

    memset(&ioctl_data, 0, sizeof(ioctl_data));
    memset(msgs, 0, sizeof(msgs));

    buf[0] = (uint8_t)(reg);
    addrlen = 1;
    memcpy(&(buf[addrlen]), data, size);
    msgs[0].buf = buf;
    msgs[0].len = addrlen + size;
    msgs[0].addr = i2c;

    ioctl_data.msgs = &(msgs[0]);
    ioctl_data.nmsgs = 1;

    for (i = 0; i < SSA_POE_OP_CNT; i++) {
        rv= ioctl(fd, I2C_RDWR, (unsigned long)&ioctl_data);
        if(rv < 0) {
            poedrv_add_i2c_error();
            usleep(POE_I2C_SLEEP_TIME);
            continue;
        }
        break;
    }

    if (rv < 0) {
        POE_DRV_ZLOG_DEBUG("ioctl fail(ret:%d, errno:%s)!\n", rv, strerror(errno));
        return -1;
    }

    return SSA_POE_E_NONE;
}

/* 通过i2c�?*/
static int32_t poedrv_read_i2c(uint32_t pse_chip_addr, uint16_t reg, void *buf, int32_t size)
{
    int32_t fd, rv;

    fd = open(POE_I2C_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        POE_DRV_ZLOG_ERROR("open c dev %s failed, fd %d.\n", POE_I2C_NAME, fd);
        return -1;
    }

    rv = poe_i2c_ioctl_read(fd, buf, pse_chip_addr, reg, size);	

    (void)close(fd);

    return rv;
}

/* 通过i2c写数�?*/
static int32_t poedrv_write_i2c(uint32_t pse_chip_addr, uint16_t reg, void *buf, int32_t size)
{
    int32_t fd, rv;

    /* 打开字符设备，即打开channel */
    fd = open(POE_I2C_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        POE_DRV_ZLOG_ERROR("open c dev %s failed, fd %d.\n", POE_I2C_NAME, fd);
        return -1;
    }

    rv = poe_i2c_ioctl_write(fd, buf, pse_chip_addr, reg, size);

    (void)close(fd);

    return rv;
}

/**
 * tps_WriteI2CMultiple 写入1个字�?
 * @ i2cAddress i2c地址
 * @ registerAddress 寄存器地址
 * @ writeValues 写入寄存器的�?
 *
 * 返回�?0-成功返回0；失败返回负�?
 */
uint8_t tps_WriteI2CReg (uint8_t i2cAddress, uint8_t registerAddress, uint8_t Value)
{
    uint8_t rtn;

    rtn = poedrv_write_i2c((uint32_t)i2cAddress, (uint16_t)registerAddress, &Value, 1);
    if (rtn != 0) {
        POE_DRV_ZLOG_ERROR("poedrv_write_i2c error");
    }

    return (rtn);
}

/**
 * tps_ReadI2CReg 读出1个字节信�?
 * @ i2cAddress i2c地址
 * @ registerAddress 寄存器地址
 * @ readValue 读出寄存器的�?
 *
 * 返回�?0-成功返回0；失败返回负�?
 */
uint8_t tps_ReadI2CReg (uint8_t i2cAddress, uint8_t registerAddress, uint8_t *readValue)
{
    uint8_t rtn;

    rtn = poedrv_read_i2c((uint32_t)i2cAddress, (uint16_t)registerAddress, readValue, 1);
    if (rtn != 0) {
        POE_DRV_ZLOG_ERROR("poedrv_read_i2c error");
    }

    return (rtn);
}

/**
 * tps_ReadI2CMultiple 读出多个字节信息
 * @ i2cAddress i2c地址
 * @ registerAddress 寄存器地址
 * @ readValue 读出寄存器的�?
 * @ numReadBytes 读出的字节数
 *
 * 返回�?0-成功返回0；失败返回负�?
 */
uint8_t tps_ReadI2CMultiple (uint8_t i2cAddress, uint8_t registerAddress, uint8_t *readValue, 
            uint8_t numReadBytes)
{
    uint8_t rtn;

    rtn = poedrv_read_i2c((uint32_t)i2cAddress, (uint16_t)registerAddress, readValue, 
        (int32_t)numReadBytes);
    if (rtn != 0) {
        POE_DRV_ZLOG_ERROR( "poedrv_read_i2c error");
    }

    return (rtn);
}

/**
 * tps_WriteI2CMultiple 写入多个字节
 * @ i2cAddress i2c地址
 * @ registerAddress 寄存器地址
 * @ writeValues 写入寄存器的�?
 * @ numWriteBytes 写入的字节数
 *
 * 返回�?0-成功返回0；失败返回负�?
 */
uint8_t tps_WriteI2CMultiple(uint8_t i2cAddress, uint8_t registerAddress, uint8_t *writeValues, 
            uint8_t numWriteBytes)
{
    uint8_t rtn;

    rtn = poedrv_write_i2c((uint32_t)i2cAddress, (uint16_t)registerAddress, writeValues, 
        (int32_t)numWriteBytes);
    if (rtn != 0) {
        POE_DRV_ZLOG_ERROR("poedrv_write_i2c error");
    }

    return (rtn);
}

static int32_t ssa_poe_drv_init(void)
{

    return POE_E_NONE;
}

/**
 * I2C_init
 *
 * poe驱动初始化函�?
 *
 * 返回�?
 *     成功返回0；失败返回负�?
 */
void I2C_init(void)
{
    POE_DRV_ZLOG_DEBUG("i2c init enter");

    (void)ssa_poe_drv_init();

    POE_DRV_ZLOG_DEBUG("i2c init end");
}