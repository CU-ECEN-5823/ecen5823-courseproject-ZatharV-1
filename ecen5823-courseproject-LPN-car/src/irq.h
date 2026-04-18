/*
 * irq.h
 *
 *  Created on: 28-Jan-2026
 *      Author: Atharv More, Shruti Kalyankar
 *
 *  @file       irq.c
 *  @author     Atharv More, atharv.more@colorado.edu
 *  @author     Shruti Kalyankar, shruti.kalyankar@colorado.edu
 *
 *  @brief      Add all the IRQs of interrupts in this file
 */

#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_

#include "stdint.h"

void LETIMER0_IRQHandler(void);
uint32_t letimerMilliseconds();

#endif /* SRC_IRQ_H_ */
