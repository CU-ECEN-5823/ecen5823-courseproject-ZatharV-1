/*
 * VCNL4040.h
 *
 *  Created on: 15-Apr-2026
 *      Author: Atharv More, Shruti Kalyankar
 */

#ifndef SRC_VCNL4040_H_
#define SRC_VCNL4040_H_

#include "stdint.h"

void VCNL4040_init();
void VCNL4040_clear_interrupt(void);
uint16_t VCNL4040_read_proximity(void);


#endif /* SRC_VCNL4040_H_ */
