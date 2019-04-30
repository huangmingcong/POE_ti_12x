/*
 * Description: I2C FUNCTIONS RUIJIE layer header (MAIN)
 * Copyright: (C)2001-2009 锐捷网络.  All rights reserved.
 */

/*
 *  I2C_functions_ruijie.h
 *  Original Author:  zhangzhaobing@ruijie.com.cn, 2018-12
 *  
 *  
 *  History:
 *
 */

#ifndef __I2C_FUNCTIONS_RUIJIE_H
#define __I2C_FUNCTIONS_RUIJIE_H

#include"stdint.h"

#define I2C_1_BYTE     1
#define I2C_2_BYTES    2
#define I2C_3_BYTES    3
#define I2C_4_BYTES    4
#define I2C_5_BYTES    5
#define I2C_6_BYTES    6
#define I2C_7_BYTES    7
#define I2C_8_BYTES    8
#define I2C_9_BYTES    9
#define I2C_10_BYTES  10
#define I2C_11_BYTES  11
#define I2C_12_BYTES  12
#define I2C_13_BYTES  13
#define I2C_14_BYTES  14
#define I2C_15_BYTES  15 
#define I2C_16_BYTES  16

#define I2C_SUCCESSFUL         0
#define I2C_RECEIVE_SUCCESS    0x1
#define I2C_TRANSMIT_SUCCESS   0x2
#define I2C_NACK			   0x20
#define I2C_ARB_LOST		   0x10


/********************************************************************************************************************
*  I2C Device Parameters Structure Definition                                                                       *
********************************************************************************************************************/

typedef struct 
{
    uint8_t Address_Mode;            ///< 7 bit or 10 bit address mode
    uint8_t Clock_Rate;              ///< 100 KHz, 400 KHz
    uint8_t Initialized;
}I2C_Device_Parameter_type;

/**************************************************************************************************/

extern uint8_t I2C_RX_Data;

#define SSA_POE_E_NONE 0

#define POE_I2C_SLEEP_TIME      (100000)
#define SSA_POE_OP_CNT          (5)
#define I2C_RD_MSG_NUM      2   /* 读操作需要的消息个数 */
#define POEi2C_IOMSGLEN     2   /* ioctl模式最大消息长度 */
#define POEI2C_WRMSGLEN     3   /* WR模式最大消息长度 */

/**************************************************************************************************************************************************
*               Prototypes                                                                                                                        *
**************************************************************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
  
void I2C_init (void);
#if(1)
uint8_t tps_WriteI2CReg (uint8_t i2cAddress, uint8_t registerAddress, uint8_t Value);
uint8_t tps_WriteI2CMultiple (uint8_t i2cAddress, uint8_t registerAddress, uint8_t *writeValues, uint8_t numWriteBytes);
uint8_t tps_ReadI2CReg (uint8_t i2cAddress, uint8_t registerAddress, uint8_t *readValue);
uint8_t tps_ReadI2CMultiple (uint8_t i2cAddress, uint8_t registerAddress, uint8_t *readValue, uint8_t numReadBytes);
#endif


#ifdef __cplusplus
}
#endif
             

#endif  // #ifndef __I2C_FUNCTIONS_RUIJIE_H
