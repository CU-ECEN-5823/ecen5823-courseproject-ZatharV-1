/*
 * timers.c
 *
 *  Created on: 28-Jan-2026
 *      Author: Atharv More
 *
 *
 *  @file       timers.c
 *
 *  @author     Atharv More, atharv.more@colorado.edu
 *
 *  @brief      Add all the timers related functions in this file.
 *
 *  @reference  ECEN 5823-001,002: IOT Embedded Firmware lecture 5 slides
 *
 */

#include "timers.h"
#include "em_core.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "oscillators.h"
#include "em_letimer.h"
#include "gpio.h"

#define INCLUDE_LOG_DEBUG 1
#include "log.h"

/**
 * @brief   Initializes LETIMER0 with predefined LED on time and period
 */
void letimer0_init()
{
  LETIMER_osc_init();
//  uint32_t LETIMER_freq = LETIMER_osc_init();
//  uint32_t LED_ON_COUNT = (LETIMER_freq * LETIMER_ON_TIME_MS) / 1000;
//  uint32_t LETIMER0_PERIOD_COUNT = (LETIMER0_CLK_FREQ * LETIMER_PERIOD_MS) / 1000;

  // It's a good practice to disable the timer to initialize it
  LETIMER_Enable(LETIMER0, false);

  LETIMER_Init_TypeDef LETIMER0_init;
  LETIMER0_init.enable = false;           // Do not start timer immediately after init
  LETIMER0_init.debugRun = false;          // Let it run in debug mode when the code is stopped on breakpoint
  LETIMER0_init.comp0Top = true;          // Loads Comp0 value to counter if it underflows
  LETIMER0_init.bufTop = false;           // bufTop; don't load COMP1 into COMP0 when REP0==0
  LETIMER0_init.ufoa0 = letimerUFOANone;  // ufoa0; no underflow output action
  LETIMER0_init.ufoa1 = letimerUFOANone;  // ufoa1; no underflow output action
  LETIMER0_init.repMode = _LETIMER_CTRL_REPMODE_FREE;   // repMode; free running mode i.e. load & go forever

  // LETIMER0 init function
  LETIMER_Init(LETIMER0, &LETIMER0_init);

  LETIMER_CompareSet(LETIMER0, 0, LETIMER0_PERIOD_COUNT);

  // Clear all the interrupt flags
  LETIMER_IntClear (LETIMER0, UINT32_MAX);      // UINT32_MAX is 0xFFFFFFFF
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);  // Enable interrupt on underflow and Compare

  LETIMER_Enable(LETIMER0, true);   // Enables LETIMER0

  // Check if the counter is running
  uint32_t temp = LETIMER_CounterGet(LETIMER0);
  while(temp == LETIMER_CounterGet(LETIMER0));  // Stops the code if LETIMER0 is not initialized
}

/**
 * @brief Waits for us_wait in blocking, Uses LETIMER0
 *        The step of us_wait time is 1ms with EM3
 * @param us_wait Wait time in us, Range: [0,1E7]
 */
void timerWaitUs_polled(uint32_t us_wait)
{
  // Range check the value
  if(us_wait > 1E7)
  {
      LOG_ERROR("The us_wait cannot be more than 1E7 or 10s.\r");
      return;
  }

  uint32_t curr_timer_count = LETIMER_CounterGet(LETIMER0);
  uint32_t prev_timer_count = curr_timer_count;
  uint32_t entry_timer_count = curr_timer_count;
  uint32_t wait_count = ((uint64_t)LETIMER0_CLK_FREQ * us_wait) / 1E6;

  while((entry_timer_count - curr_timer_count) < wait_count)
  {
      curr_timer_count = LETIMER_CounterGet(LETIMER0);
      if(prev_timer_count < curr_timer_count)
      {
          entry_timer_count += LETIMER0_PERIOD_COUNT;
      }
      prev_timer_count = curr_timer_count;
  }
}

/**
 * @brief Waits for us_wait in non-blocking, Uses LETIMER0
 *        The step of us_wait time is 1ms with EM3
 * @param us_wait Wait time in us, Range: [1E2, 3E6]
 */
void timerWaitUs_irq(uint32_t us_wait)
{
  uint32_t curr_timer_count = LETIMER_CounterGet(LETIMER0);
  uint32_t next_timer_count;

  // Range Check
  if(us_wait < 100 || us_wait > 3E6)
  {
      LOG_ERROR("The range of us_wait is [1E2,3E6], cannot wait for %luus\r", us_wait);
  }

  uint32_t wait_count = (us_wait * LETIMER0_CLK_FREQ) / 1E6;

  // Wrap around the counter
  if(wait_count > curr_timer_count)
  {
      next_timer_count = LETIMER0_PERIOD_COUNT + curr_timer_count - wait_count;
  }
  else
  {
      next_timer_count = curr_timer_count - wait_count;
  }

  // Set the match value
  LETIMER_CompareSet(LETIMER0, 1, next_timer_count);

  LETIMER_IntClear (LETIMER0, LETIMER_IFC_COMP1); // Clear COMP1 interrupt flag
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP1); // Enable interrupt on underflow and Compare
}

/**
 * @brief   This is the unit test function of the timerWaitUs_polled
 *          Blinks LED0 with 9 different wait time
 */
void test_timerWaitUs_polled()
{
  LOG_INFO("Running unit test of timerWaitUs_polled()\r");
  // Time array : 0s, 10us, 100us, 1ms, 10ms, 100ms, 1s, 10s, 100s
  uint32_t test_time[9] = {0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8};

  for(int i = 0; i < 9; i++)
  {
      LOG_INFO("Testing with %lu us\r", test_time[i]);
      gpioLed0SetOn();
      timerWaitUs_polled(test_time[i]);
      gpioLed0SetOff();
      timerWaitUs_polled(test_time[i]);
  }
}

/**
 * @brief This is the unit test function of the timerWaitUs_irq
 *        Blinks LED0 with 9 different wait time
 */
void test_timerWaitUs_irq()
{
  LOG_INFO("Running unit test of timerWaitUs_polled()\r");
  // Time array : 0s, 10us, 100us, 1ms, 10ms, 100ms, 1s, 10s, 100s
  uint32_t test_time[9] = {0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8};

  (void)test_time;
}
