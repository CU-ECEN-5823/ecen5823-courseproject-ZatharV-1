/*
 * irq.c
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

#include "em_letimer.h"
#include "gpio.h"
#include "stdint.h"
#include "scheduler.h"
#include "timers.h"
#include "em_i2c.h"
#include "em_gpio.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

static uint32_t LETIMER0_UF_count;

/**
 * @brief This function is the Interrupt handler of LETIMER0
 */
void LETIMER0_IRQHandler(void)
{
    // Determine the source of interrupt
    uint32_t interrupt_flags = LETIMER_IntGet(LETIMER0);
    if(interrupt_flags & LETIMER_IF_UF)
    {
        LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);  // Clear the flag
        LETIMER0_UF_count++;
        schedulerSetEvent_LE_UF();
    }
    else if(interrupt_flags & LETIMER_IF_COMP1)
    {
        LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP1);   // Clear the flag
        LETIMER_IntDisable(LETIMER0, LETIMER_IF_COMP1); // Disable interrupt to not wake up
        schedulerSetEvent_LE_COMP1();
    }
    else
    {
        LETIMER_IntClear(LETIMER0, UINT32_MAX);  // If not the other 2 flags then clear all the flags
    }
}

/**
 * @brief   Calculates and returns ms passed since power_on
 * @return  ms passed since power_on
 */
uint32_t letimerMilliseconds()
{
  return LETIMER0_UF_count * LETIMER_PERIOD_MS + LETIMER0_PERIOD_COUNT - LETIMER_CounterGet(LETIMER0);
}


/**
 * @brief I2C0 IRQ handler
 */
void I2C0_IRQHandler(void)
{
  I2C_TransferReturn_TypeDef I2C_Transfer_status = I2C_Transfer(I2C0);

  // No need to do anything if in progress
  if(I2C_Transfer_status == i2cTransferDone)
  {
      schedulerSetEvent_I2C_Transfer_Complete();
  }
  else if(I2C_Transfer_status < 0)
  {
      LOG_ERROR("I2C_Transfer: I2C failed: Error Code: %d\r", I2C_Transfer_status);
  }
}


/**
 * @brief GPIO ODD pin handler
 */
void GPIO_ODD_IRQHandler(void)
{
  uint32_t interrupt_flags = GPIO_IntGet();
  GPIO_IntClear(interrupt_flags); // Clear Flag

  if(interrupt_flags & (1 << VCNL4040_INT_pin))
  {
      schedulerSetEvent_VCNL4040_INT();
  }

}
