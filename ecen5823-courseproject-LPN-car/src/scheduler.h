/*
 * scheduler.h
 *
 *  Created on: 04-Feb-2026
 *      Author: Atharv More
 *
 *  @file       irq.h
 *  @author     Atharv More, atharv.more@colorado.edu
 *
 *  @brief      Event schedulers header file
 */

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

typedef enum
{
  EVT_NONE,
  LETIMER0_UF = 1,
  LETIMER0_COMP1 = 2,
  I2C_TRANSFER_COMPLETE = 4,
  MMA8452_INT = 8
}EVENTS_t;

extern EVENTS_t APP_event;

void schedulerSetEvent_LE_UF();
EVENTS_t getNextEvent();
void schedulerSetEvent_I2C_Transfer_Complete();
void schedulerSetEvent_LE_COMP1();
void schedulerSetEvent_MMA8452_INT();

void si7021_init();
void si7021_I2C();

void si7021_init1();
#endif /* SRC_SCHEDULER_H_ */
