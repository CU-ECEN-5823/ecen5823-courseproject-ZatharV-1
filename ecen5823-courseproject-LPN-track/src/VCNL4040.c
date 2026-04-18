/*
 * VCNL4040.c
 *
 *  Created on: 14-Apr-2026
 *      Author: Atharv More, Shruti Kalyankar
 *
 */

#include "VCNL4040.h"
#include "VCNL4040_REG.h"

#include "i2c.h"
#include "sl_i2cspm.h"
#include "app_assert.h"

#include "gpio.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#define VCNL4040_I2C_ADDR 0x60
#define PS_THDH_THRESHOLD 50
#define PS_THDL_THRESHOLD 35

/**
 * @brief Initializes sensor and configures interrupt pin.
 */
void VCNL4040_init()
{
  LOG_INFO("Initializing VCNL4040\r");

  uint8_t device_id[2];

  // Read & verify the device
  app_assert_status_f(I2C0_read_reg(VCNL4040_I2C_ADDR, VCNL4040_ID, device_id, 2),
                      "Failed to Communicate with sensor VCNL4040");

  app_assert_status_f((device_id[0] != VCNL4040_DEVICE_ID_LSB),
                      "VCNL4040 ID mismatch, read ID = 0x%x\r", device_id[0]);

  uint8_t thdh_payload[3] = {
      VCNL4040_PS_THDH,
      (uint8_t)(PS_THDH_THRESHOLD & 0xFF),
      (uint8_t)(PS_THDH_THRESHOLD >> 8)
  };
  I2C0_write(VCNL4040_I2C_ADDR, thdh_payload, 3);

  uint8_t thdl_payload[3] = {
      VCNL4040_PS_THDL,
      (uint8_t)(PS_THDL_THRESHOLD & 0xFF),
      (uint8_t)(PS_THDL_THRESHOLD >> 8)
    };
    I2C0_write(VCNL4040_I2C_ADDR, thdl_payload, 3);



  uint8_t ps_conf[3] = {
      VCNL4040_PS_CONF1,
      VCNL4040_PS_IT_2T | VCNL4040_PS_PERS_1 | VCNL4040_PS_DUTY_160,
      VCNL4040_PS_HD_16_BIT | VCNL4040_PS_INT_CLOSE
  };
  I2C0_write(VCNL4040_I2C_ADDR, ps_conf, 3);

  uint8_t ps_conf3_MS[3] = {
      VCNL4040_PS_CONF3,
      VCNL4040_PS_SMART_PERS_ENABLE,
      VCNL4040_LED_50MA
  };
  I2C0_write(VCNL4040_I2C_ADDR, ps_conf3_MS, 3);

  VCNL4040_clear_interrupt();

  LOG_INFO("Initialized\r");
}


/**
 * @brief   Clears the interrupt
 */
void VCNL4040_clear_interrupt()
{
  uint8_t int_flags[2];
  if (I2C0_read_reg(VCNL4040_I2C_ADDR, VCNL4040_INT_FLAG, int_flags, 2) == 0) {
//      LOG_INFO("VCNL4040 Int Cleared: 0x%02X 0x%02X\r\n", int_flags[1], int_flags[0]);
  }
  else
    {
      LOG_ERROR("VCNL clear interrupt \r");
    }
}


/**
 * @brief   Reads the current proximity value from the sensor.
 * @return  16-bit proximity counts (0 to 65535)
 */
uint16_t VCNL4040_read_proximity(void) {
    uint8_t cmd = VCNL4040_PS_DATA; // 0x08
    uint8_t data[2] = {0};

    I2C_TransferSeq_TypeDef seq;
    seq.addr = VCNL4040_I2C_ADDR << 1;
    seq.flags = I2C_FLAG_WRITE_READ; // Write command, then Restart and Read

    seq.buf[0].data = &cmd;
    seq.buf[0].len  = 1;
    seq.buf[1].data = data;
    seq.buf[1].len  = 2;

    if (I2CSPM_Transfer(I2C0, &seq) != i2cTransferDone) {
        LOG_ERROR("I2C read of proximity data failed!\r\n");
        return 0;
    }
    uint16_t pr = (uint16_t)((data[1] << 8) | data[0]);
    LOG_INFO("PROXIMITY: %d\r", pr);

    int pin_level = read_int_pin();

    LOG_INFO("GPIO_level: %d\r\n", pin_level);

    if(pin_level == 0)
    {
        VCNL4040_clear_interrupt();
    }
    return pr;
}
