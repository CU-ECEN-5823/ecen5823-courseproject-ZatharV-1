/*
 * MMA8452.h
 *
 *  Created on: 16-Apr-2026
 *  @author     Atharv More, atharv.more@colorado.edu
 */

#ifndef SRC_MMA8452_H_
#define SRC_MMA8452_H_

#include "stdint.h"

typedef struct {
    int16_t x, y, z;
} mma_data_t;

mma_data_t MMA8452Q_read_val();
void MMA8452Q_init();
void MMA8452_clear_interrupt();


#endif /* SRC_MMA8452_H_ */
