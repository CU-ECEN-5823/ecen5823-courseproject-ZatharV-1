/*
 * i2c.c
 *
 *  Created on: 04-Feb-2026
 *      Author: Atharv More
 *
 *  @file       i2c.c
 *  @author     Atharv More, atharv.more@colorado.edu
 *
 *  @brief      Add all the I2C functions in this file
 */

#include "i2c.h"
#include "sl_i2cspm.h"
#include "timers.h"
#include "gpio.h"
#include "em_i2c.h"
#include "sl_power_manager.h"

#define INCLUDE_LOG_DEBUG 0
#include "src/log.h"

#define I2C0_PORT     gpioPortC
#define I2C0_SCL_PIN  11
#define I2C0_SDA_PIN  10

/**
 * @brief Initializes the I2C0 on pin PC10, PC11
 * @ref   Lecture Slide 6 on canvas
 */
void I2C0_init()
{
  I2CSPM_Init_TypeDef I2C_Config = {
  .port = I2C0,
  .sclPort = gpioPortC,
  .sclPin = I2C0_SCL_PIN,
  .sdaPort = gpioPortC,
  .sdaPin = I2C0_SDA_PIN,
  .portLocationScl = 14,
  .portLocationSda = 16,
  .i2cRefFreq = 0,
  .i2cMaxFreq = I2C_FREQ_STANDARD_MAX,
  .i2cClhr = i2cClockHLRStandard
  };
  I2CSPM_Init(&I2C_Config);
}

/**
 * @brief Write the data buffer on the I2C line
 *
 * @param slaveAddr   I2C Slave Address
 * @param buffer      Data buffer
 * @param buffer_len  Data buffer length
 *
 * @return  I2C transfer status
 */
I2C_TransferReturn_TypeDef I2C0_write(uint8_t slaveAddr, uint8_t* buffer, uint8_t buffer_len)
{
  I2C_TransferSeq_TypeDef transferSequence;
  transferSequence.addr = slaveAddr << 1;
  transferSequence.flags = I2C_FLAG_WRITE;
  transferSequence.buf[0].data = buffer;         // Read Temperature Command
  transferSequence.buf[0].len = buffer_len;

  I2C_TransferReturn_TypeDef transferStatus = I2CSPM_Transfer (I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone)
  {
      LOG_ERROR("I2CSPM_Transfer: I2C bus write failed.\r");
      return false;
  }

  return true;
}

/**
 * @brief Reads I2C slave on the I2C line
 *
 * @param slaveAddr   I2C Slave Address
 * @param buffer      Data buffer
 * @param buffer_len  Data buffer length
 *
 * @return  I2C transfer status
 */
I2C_TransferReturn_TypeDef I2C0_read_reg(uint8_t slaveAddr, uint8_t reg_addr, uint8_t* buffer, uint8_t buffer_len)
{
  I2C_TransferSeq_TypeDef transferSequence;
  transferSequence.addr = slaveAddr << 1;
  transferSequence.flags = I2C_FLAG_WRITE_READ;
  transferSequence.buf[0].data = &reg_addr;         // Read Temperature Command
  transferSequence.buf[0].len = 1;
  transferSequence.buf[1].data = buffer;         // Read Temperature Command
  transferSequence.buf[1].len = buffer_len;


  I2C_TransferReturn_TypeDef transferStatus = I2CSPM_Transfer (I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone)
  {
      LOG_ERROR("I2CSPM_Transfer: I2C bus read of reg=0x%X failed\r", reg_addr);
      return transferStatus;
  }

  return transferStatus;
}



/*--------------SI7021 Code------------------*/

#define SI7021_MSR_TEMPERATURE_CMD  0xE3
#define SI7021_DEVICE_ADDR          0x40
#define SI7021_READ_DATA_LEN        2

uint8_t read_data[SI7021_READ_DATA_LEN];
uint8_t cmd_data = SI7021_MSR_TEMPERATURE_CMD;

/**
 * @brief   Sends the command to the SI7021 to measure temperature
 * @return  1 on success, 0 on failure
 */
int SI7021_send_msr_temp_cmd()
{
  static I2C_TransferSeq_TypeDef transferSequence;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1;  // shift device address left
  transferSequence.flags = I2C_FLAG_WRITE;
  transferSequence.buf[0].data = &cmd_data;         // Read Temperature Command
  transferSequence.buf[0].len = sizeof(cmd_data);

  NVIC_EnableIRQ(I2C0_IRQn);  // Enable interrupt

  I2C_TransferReturn_TypeDef transferStatus = I2C_TransferInit(I2C0, &transferSequence);
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
  if (transferStatus < 0)
  {
      LOG_ERROR("I2CSPM_Transfer: I2C bus write of cmd=0x%X failed\r", SI7021_MSR_TEMPERATURE_CMD);
      sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      return 0;
  }
  return 1;
}

/**
 * @brief   Reads the temperature data from SI7021
 * @return  1 on success, 0 on failure
 */
int SI7021_read_temp_data()
{

  static I2C_TransferSeq_TypeDef transferSequence;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1;  // shift device address left
  transferSequence.flags = I2C_FLAG_READ;
  transferSequence.buf[0].data = read_data;         // Read Temperature Command
  transferSequence.buf[0].len = SI7021_READ_DATA_LEN;

  NVIC_EnableIRQ(I2C0_IRQn);  // Enable interrupt
  I2C_TransferReturn_TypeDef transferStatus = I2C_TransferInit(I2C0, &transferSequence);
  if (transferStatus < 0)
  {
      LOG_ERROR("I2CSPM_Transfer: I2C bus write of cmd=0x%X failed\r", SI7021_MSR_TEMPERATURE_CMD);
      sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      return 0;
  }
  return 1;
}

/**
 * @brief Converts the I2C read_data to temperature in *C
 */
void conv_temp()
{
  uint16_t temp_data = (read_data[0] << 8) | read_data[1];
  int temperature_C = (175.72 * temp_data / 65536 - 46.85);       // Formula given in SI7021 datasheet
  LOG_INFO("Temperature Reading: %d\r", temperature_C);
}
