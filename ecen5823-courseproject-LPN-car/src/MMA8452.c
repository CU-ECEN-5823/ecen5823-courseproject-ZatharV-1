/*
 * MMA8452.c
 *
 *  Created on: 14-Apr-2026
 *      Author: Atharv More
 */


#include "MMA8452.h"
#include "MMA8452_reg.h"

#include "i2c.h"
#include "app_assert.h"

#define INCLUDE_LOG_DEBUG 0
#include "log.h"

#define MMA8452_DEVICE_ID 0x2A

#define IMPACT_THRESHOLD  3 // 3g
#define TRANSIENT_THS_VAL (uint8_t)(IMPACT_THRESHOLD / 0.063)
#define TRANSIENT_COUNT_VAL   2

#define MMA8452_WRITE_REG_LEN 2

/**
 * @brief Initializes sensor and configures interrupt pin.
 */
void MMA8452Q_init()
{
  LOG_INFO("Initializing MMA8452");

  uint8_t device_id = 0;

  // 1. Read and verify the device
  app_assert_status_f(I2C0_read_reg(MMA8452Q_DEFAULT_ADDRESS, WHO_AM_I, &device_id, 1),
                      "Failed to communicate with sensor MMA8452");
  app_assert_status_f((device_id != MMA8452_DEVICE_ID), "MMA8452Q not found! ID: 0x%02X\r\n", device_id);


  // Enter in standby mode to configure registers
  uint8_t ctrl1;
  I2C0_read_reg(MMA8452Q_DEFAULT_ADDRESS, CTRL_REG1, &ctrl1, 1);
  uint8_t standby_cmd[MMA8452_WRITE_REG_LEN] = { CTRL_REG1, (ctrl1 & ~MMA8452_CTRL_REG1_ACTIVE_POS) };
  I2C0_write(MMA8452Q_DEFAULT_ADDRESS, standby_cmd, MMA8452_WRITE_REG_LEN);

  // Set scale
  uint8_t scale;
  I2C0_read_reg(MMA8452Q_DEFAULT_ADDRESS, XYZ_DATA_CFG, &scale, MMA8452_WRITE_REG_LEN);
  scale &= ~MMA8452_XYZ_DATA_CFG_FS_MSK;
  scale |= MMA8452_XYZ_DATA_CFG_FS_8G;
  uint8_t scale_cmd[MMA8452_WRITE_REG_LEN] = {XYZ_DATA_CFG, scale};
  I2C0_write(MMA8452Q_DEFAULT_ADDRESS, scale_cmd, MMA8452_WRITE_REG_LEN);

  // Enable interrupts
  uint8_t int_en[MMA8452_WRITE_REG_LEN] = { CTRL_REG4, MMA8452_CTRL_REG4_INT_EN_TRANS };
  I2C0_write(MMA8452Q_DEFAULT_ADDRESS, int_en, MMA8452_WRITE_REG_LEN);

  // Set transient interrupt at INT1
  uint8_t int_map[MMA8452_WRITE_REG_LEN] = { CTRL_REG5, MMA8452_CTRL_REG5_INT_CFG_TRANS };
  I2C0_write(MMA8452Q_DEFAULT_ADDRESS, int_map, MMA8452_WRITE_REG_LEN);

  uint8_t trans_cfg[MMA8452_WRITE_REG_LEN] = { TRANSIENT_CFG, MMA8452_Transient_CFG_ELE | MMA8452_Transient_CFG_YTEFE | MMA8452_Transient_CFG_XTEFE};
  I2C0_write(MMA8452Q_ADDR, trans_cfg, MMA8452_WRITE_REG_LEN);

  uint8_t trans_ths[MMA8452_WRITE_REG_LEN] = { TRANSIENT_THS, TRANSIENT_THS_VAL };
  I2C0_write(MMA8452Q_ADDR, trans_ths, MMA8452_WRITE_REG_LEN);

  uint8_t trans_count[MMA8452_WRITE_REG_LEN] = { TRANSIENT_COUNT, TRANSIENT_COUNT_VAL };
  I2C0_write(MMA8452Q_ADDR, trans_count, MMA8452_WRITE_REG_LEN);

  uint8_t lp_cmd[MMA8452_WRITE_REG_LEN] = { CTRL_REG2, MMA8452_CTRL_REG2_MODS_Low_Power };
  I2C0_write(MMA8452Q_DEFAULT_ADDRESS, lp_cmd, MMA8452_WRITE_REG_LEN);

  // 5. Set ACTIVE mode to start detection (800Hz ODR by default)
//  uint8_t active_cmd[MMA8452_WRITE_REG_LEN] = { CTRL_REG1, (MMA8452_CTRL_REG1_ACTIVE_POS | MMA8452_CTRL_REG1_ODR_50) };
//  I2C0_write(MMA8452Q_DEFAULT_ADDRESS, active_cmd, MMA8452_WRITE_REG_LEN);


  LOG_INFO("MMA8452Q Initialized and Active\r\n");
}


mma_data_t MMA8452Q_read_val()
{
    uint8_t raw[6];
    mma_data_t accel;

    // Read 6 bytes (MSB, LSB for X, Y, Z)
    if (I2C0_read_reg(MMA8452Q_DEFAULT_ADDRESS, OUT_X_MSB, raw, 6) == 0) {
        // Reconstruct 12-bit signed values
        // Copying SparkFun logic: combine MSB and LSB, then shift right by 4
        accel.x = ((int16_t)(raw[0] << 8 | raw[1])) >> 4;
        accel.y = ((int16_t)(raw[2] << 8 | raw[3])) >> 4;
        accel.z = ((int16_t)(raw[4] << 8 | raw[5])) >> 4;

        LOG_INFO("Accel_val: x = %d, y = %d, z = %d\r", accel.x, accel.y, accel.z);
    } else {
        accel.x = accel.y = accel.z = 0;
    }

    return accel;
}


void MMA8452_clear_interrupt()
{
  uint8_t src_reg_val;
  I2C0_read_reg(MMA8452Q_ADDR, TRANSIENT_SRC, &src_reg_val, 1);
  LOG_INFO("Interrupt Cleared. Source Register: 0x%02X\r\n", src_reg_val);
}
