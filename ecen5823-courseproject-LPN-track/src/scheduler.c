/*
 * scheduler.c
 *
 *  Created on: 04-Feb-2026
 *      Author: Atharv More
 *
 *  @file       irq.c
 *  @author     Atharv More, atharv.more@colorado.edu
 *
 *  @brief      Add all the Event schedulers this file
 */

#include "scheduler.h"
#include "em_core.h"
#include "gpio.h"
#include "i2c.h"
#include "timers.h"
#include "sl_power_manager.h"
#include "BLE_mesh.h"
#include "lcd.h"

#define SI7021_PWRUP_TIME_us        80000

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

volatile uint64_t lap_time_ms = 0;
volatile uint64_t prev_car_pass_time_ms = 0;
volatile uint64_t curr_car_pass_time_ms = 0;

EVENTS_t APP_event;

static void lcd_update_lap_info(uint32_t lap_count, uint32_t lap_time_ms);

/**
 * @brief Sets Event for LETIMER0_UF
 */
void schedulerSetEvent_LE_UF()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();        // Enter critical section
  // Accessing LE_event
//  APP_event |= LETIMER0_UF;  // Set LETIMER0 UF event flag
  sl_bt_external_signal(LETIMER0_UF);
  LETIMER0_current_time += LETIMER_PERIOD_MS;
  CORE_EXIT_CRITICAL();         // Exit critical section
}

/**
 * @brief Sets Event for LETIMER0_COMP1
 */
void schedulerSetEvent_LE_COMP1()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();        // Enter critical section
  // Accessing LE_event
//  APP_event |= LETIMER0_COMP1;  // Set LETIMER0 COMP1 event flag
  sl_bt_external_signal(LETIMER0_COMP1);
  CORE_EXIT_CRITICAL();         // Exit critical section
}

/**
 * @brief Sets event for I2C_Transfer_Complete
 */
void schedulerSetEvent_I2C_Transfer_Complete()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();        // Enter critical section
  // Accessing LE_event
//  APP_event |= I2C_TRANSFER_COMPLETE;  // Set I2C_TRANSFER_COMPLETE_FLAG
  sl_bt_external_signal(I2C_TRANSFER_COMPLETE);
  CORE_EXIT_CRITICAL();         // Exit critical section
}

/**
 * @brief Sets event for VCNL4040_INT
 */
void schedulerSetEvent_VCNL4040_INT()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();        // Enter critical section
  // Accessing LE_event
  curr_car_pass_time_ms = get_LETIMER_ms();
  if(!prev_car_pass_time_ms){
      prev_car_pass_time_ms = curr_car_pass_time_ms;
  }
  else {
      lap_time_ms = curr_car_pass_time_ms - prev_car_pass_time_ms;
      prev_car_pass_time_ms = curr_car_pass_time_ms;
  }
//  APP_event |= VCNL4040_INT;  // Set I2C_TRANSFER_COMPLETE_FLAG
  sl_bt_external_signal(VCNL4040_INT);
  CORE_EXIT_CRITICAL();         // Exit critical section
}

/**
 * @brief   Reads the events and returns the next event to run
 * @return  Next Event
 */
EVENTS_t getNextEvent()
{
  EVENTS_t event = EVT_NONE;

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();        // Enter critical section
  // Accessing LE_event

  if(APP_event & LETIMER0_UF)
  {
      // Reads and clears the flag and sets the event
      event = LETIMER0_UF;
      APP_event &= ~LETIMER0_UF;
  }
  else if(APP_event & LETIMER0_COMP1)
  {
      event = LETIMER0_COMP1;
      APP_event &= ~LETIMER0_COMP1;
  }
  else if(APP_event & VCNL4040_INT)
  {
      event = VCNL4040_INT;
      APP_event &= ~VCNL4040_INT;
  }
  else if(APP_event & I2C_TRANSFER_COMPLETE)
  {
      event = I2C_TRANSFER_COMPLETE;
      APP_event &= ~I2C_TRANSFER_COMPLETE;
  }
  else
  {
      event = EVT_NONE;
  }

  CORE_EXIT_CRITICAL();         // Exit critical section

  return event;
}

/*---------VCNL4040 Code---------*/


void lap_time_logic(void)
{
  static uint32_t lap_count = 0;
  static uint32_t last_lap_start_time = 0;
  uint32_t current_time = get_LETIMER_ms();
  uint32_t lap_time_ms = 0;

  if (lap_count > 0) {
    lap_time_ms = current_time - last_lap_start_time;

    // Publish to mesh Friend node
    publish_lap_data(lap_count, lap_time_ms);

    // Update LCD
    lcd_update_lap_info(lap_count, lap_time_ms);
  }

  lap_count++;
  last_lap_start_time = current_time;
}



/*---------SI7021 Code---------*/

typedef enum{
  SI7021_SEND_CMD,
  SI7021_READ_DATA,
  SI7021_CONV_TEMP,
  SI7021_COMPLETE
}SI7021_I2C_STATES;


/**
 * @brief Reads temperature value from SI7021 temperature sensor
 * @ref   Lecture Slide 6 on canvas
 */
void si7021_init()
{
  enable_SI7021();
  timerWaitUs_irq(SI7021_PWRUP_TIME_us);
}

/**
 * @brief State Machine for reading SI7021 temperature
 */
void si7021_I2C()
{
  static SI7021_I2C_STATES state = SI7021_SEND_CMD;

  switch(state)
  {
    case SI7021_SEND_CMD:
      if(SI7021_send_msr_temp_cmd())
      {
          // If success then read the temperature data
          state = SI7021_READ_DATA;
      }
      else
      {
          // On failure, disable the sensor
          state = SI7021_COMPLETE;
      }
      break;

    case SI7021_READ_DATA:
      if(SI7021_read_temp_data())
      {
          state = SI7021_CONV_TEMP;
      }
      else
      {
          state = SI7021_COMPLETE;
      }
      break;

    case SI7021_CONV_TEMP:
      conv_temp();
      /* fallthrough */

    case SI7021_COMPLETE:
      // Disable sensor and go to sleep
      disable_SI7021();
      sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      NVIC_DisableIRQ(I2C0_IRQn); // Disable interrupt
      state = SI7021_SEND_CMD;    //Reset for next read
      break;
  }
}

static void lcd_update_lap_info(uint32_t lap_count, uint32_t lap_time_ms)
{
  displayPrintf(DISPLAY_ROW_LAPTIME, "Lap Time = %ld.%03lds", lap_time_ms/1000, lap_time_ms%1000);
  displayPrintf(DISPLAY_ROW_LAPNUM, "Lap Number = %lu", lap_count);
}



