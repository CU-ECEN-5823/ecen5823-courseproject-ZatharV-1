/*
 * timers.h
 *
 *  Created on: 28-Jan-2026
 *      Author: Atharv More
 *
 *  @file       timers.h
 *  @author     Atharv More, atharv.more@colorado.edu
 *
 *  @brief      Timers functions header file
 */

#ifndef SRC_TIMERS_H_
#define SRC_TIMERS_H_

#include "stdint.h"
#include "oscillators.h"

/*
// Changing these values will change the on time and period on LED0
#define LETIMER_ON_TIME_MS  175
#define LETIMER_PERIOD_MS   2250
*/

extern volatile uint64_t LETIMER0_current_time;

// Interrupt period
#define LETIMER_PERIOD_MS     1000
#define LETIMER0_PERIOD_COUNT (LETIMER0_CLK_FREQ * LETIMER_PERIOD_MS) / 1000

// Change this macros value to 1 to unit test the function at start
#define PERFORM_timerWaitUs_UNIT_TEST  0

void test_timerWaitUs_polled();
void timerWaitUs_polled(uint32_t us_wait);
void timerWaitUs_irq(uint32_t us_wait);
void letimer0_init();
uint64_t get_LETIMER_ms();

#endif /* SRC_TIMERS_H_ */
