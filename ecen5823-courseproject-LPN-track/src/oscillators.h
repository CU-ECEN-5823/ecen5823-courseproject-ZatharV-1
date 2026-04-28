/*
 * oscillators.h
 *
 *  Created on: 28-Jan-2026
 *      Author: Atharv More
 *
 *  @file       oscillators.h
 *  @author     Atharv More, atharv.more@colorado.edu
 *
 *  @brief      Oscillators functions header file
 */

#ifndef SRC_OSCILLATORS_H_
#define SRC_OSCILLATORS_H_

#include "stdint.h"
#include "app.h"

uint32_t LETIMER_osc_init();

#if LOWEST_ENERGY_MODE == 3
#define LETIMER0_CLK_FREQ 1000
#else
#define LETIMER0_CLK_FREQ 32768
#endif

#endif /* SRC_OSCILLATORS_H_ */
