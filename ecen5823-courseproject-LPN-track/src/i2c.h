/*
 * i2c.h
 *
 *  Created on: 04-Feb-2026
 *      Author: Atharv More
 *
 *  @file       irq.h
 *  @author     Atharv More, atharv.more@colorado.edu
 *
 *  @brief      I2C functions header file
 */

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include "em_i2c.h"

void I2C0_init();
int SI7021_read_temp_data();
int SI7021_send_msr_temp_cmd();
void conv_temp();

I2C_TransferReturn_TypeDef I2C0_write(uint8_t slaveAddr, uint8_t* buffer, uint8_t buffer_len);
I2C_TransferReturn_TypeDef I2C0_read_reg(uint8_t slaveAddr, uint8_t reg_addr, uint8_t* buffer, uint8_t buffer_len);

#endif /* SRC_I2C_H_ */
