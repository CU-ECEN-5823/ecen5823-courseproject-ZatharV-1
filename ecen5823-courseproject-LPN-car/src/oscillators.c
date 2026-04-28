/*
 * oscillators.c
 *
 *  Created on: 28-Jan-2026
 *      Author: Atharv More
 *
 *  @file       oscillators.c
 *
 *  @author     Atharv More, atharv.more@colorado.edu
 *
 *  @brief      Add all the oscillator related functions in this file.
 *
 *  @reference  ECEN 5823-001,002: IOT Embedded Firmware lecture 5 slides
 *
 */

#include "oscillators.h"
#include "app.h"
#include "em_cmu.h"


/**
 * @brief   Initializes LETIMER oscillator depending on the mode (ULFRCO for EM3 & LFXO for EM0/1/2)
 * @return  Frequency of LETIMER
 */
uint32_t LETIMER_osc_init()
{
  uint32_t LETIMER_clock_freq;

#if (LOWEST_ENERGY_MODE == 3)
    CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
    CMU_ClockEnable(cmuClock_LFA, true);
    CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_1);
    CMU_ClockEnable(cmuClock_LETIMER0, true);
#else
    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
    CMU_ClockEnable(cmuClock_LFA, true);
    CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_2);
    CMU_ClockEnable(cmuClock_LETIMER0, true);
#endif

    LETIMER_clock_freq = CMU_ClockFreqGet(cmuClock_LETIMER0);
    return LETIMER_clock_freq;
}
