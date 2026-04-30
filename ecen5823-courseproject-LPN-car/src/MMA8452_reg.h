/*
 * MMA8452_reg.h
 *
 *  Created on: 16-Apr-2026
 *
 *  This file has been referenced from
 *  https://github.com/sparkfun/SparkFun_MMA8452Q_Arduino_Library/blob/main/src/SparkFun_MMA8452Q.h
 *
 *  Original Creation Date: June 3, 2014
 *  https://github.com/sparkfun/MMA8452_Accelerometer
 */

#ifndef SRC_MMA8452_REG_H_
#define SRC_MMA8452_REG_H_

#define MMA8452Q_ADDR      0x1D // Default for SparkFun breakout


#define MMA8452_CTRL_REG1_ACTIVE_POS  0x01

#define MMA8452_XYZ_DATA_CFG_FS_MSK   0x03
#define MMA8452_XYZ_DATA_CFG_FS_2G    0x00
#define MMA8452_XYZ_DATA_CFG_FS_4G    0x01
#define MMA8452_XYZ_DATA_CFG_FS_8G    0x02

#define MMA8452_CTRL_REG4_INT_EN_TRANS  0x20
#define MMA8452_CTRL_REG5_INT_CFG_TRANS 0x20

#define MMA8452_Transient_CFG_YTEFE   0x04
#define MMA8452_Transient_CFG_XTEFE   0x02
#define MMA8452_Transient_CFG_ELE     0x10

#define MMA8452_CTRL_REG1_ODR_50      0x20
#define MMA8452_CTRL_REG2_SLPE_EN     0x04
#define MMA8452_CTRL_REG2_MODS_Low_Power  0x03


///////////////////////////////////
// MMA8452Q Register Definitions //
///////////////////////////////////
enum MMA8452Q_Register
{
  STATUS_MMA8452Q = 0x00,
  OUT_X_MSB = 0x01,
  OUT_X_LSB = 0x02,
  OUT_Y_MSB = 0x03,
  OUT_Y_LSB = 0x04,
  OUT_Z_MSB = 0x05,
  OUT_Z_LSB = 0x06,
  SYSMOD = 0x0B,
  INT_SOURCE = 0x0C,
  WHO_AM_I = 0x0D,
  XYZ_DATA_CFG = 0x0E,
  HP_FILTER_CUTOFF = 0x0F,
  PL_STATUS = 0x10,
  PL_CFG = 0x11,
  PL_COUNT = 0x12,
  PL_BF_ZCOMP = 0x13,
  P_L_THS_REG = 0x14,
  FF_MT_CFG = 0x15,
  FF_MT_SRC = 0x16,
  FF_MT_THS = 0x17,
  FF_MT_COUNT = 0x18,
  TRANSIENT_CFG = 0x1D,
  TRANSIENT_SRC = 0x1E,
  TRANSIENT_THS = 0x1F,
  TRANSIENT_COUNT = 0x20,
  PULSE_CFG = 0x21,
  PULSE_SRC = 0x22,
  PULSE_THSX = 0x23,
  PULSE_THSY = 0x24,
  PULSE_THSZ = 0x25,
  PULSE_TMLT = 0x26,
  PULSE_LTCY = 0x27,
  PULSE_WIND = 0x28,
  ASLP_COUNT = 0x29,
  CTRL_REG1 = 0x2A,
  CTRL_REG2 = 0x2B,
  CTRL_REG3 = 0x2C,
  CTRL_REG4 = 0x2D,
  CTRL_REG5 = 0x2E,
  OFF_X = 0x2F,
  OFF_Y = 0x30,
  OFF_Z = 0x31
};

////////////////////////////////
// MMA8452Q Misc Declarations //
////////////////////////////////
enum MMA8452Q_Scale
{
  SCALE_2G = 2,
  SCALE_4G = 4,
  SCALE_8G = 8
}; // Possible full-scale settings
enum MMA8452Q_ODR
{
  ODR_800,
  ODR_400,
  ODR_200,
  ODR_100,
  ODR_50,
  ODR_12,
  ODR_6,
  ODR_1
}; // possible data rates
// Possible portrait/landscape settings
#define PORTRAIT_U 0
#define PORTRAIT_D 1
#define LANDSCAPE_R 2
#define LANDSCAPE_L 3
#define LOCKOUT 0x40
#define MMA8452Q_DEFAULT_ADDRESS 0x1D

// Posible SYSMOD (system mode) States
#define SYSMOD_STANDBY 0b00
#define SYSMOD_WAKE 0b01
#define SYSMOD_SLEEP 0b10

#endif /* SRC_MMA8452_REG_H_ */
