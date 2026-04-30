/***************************************************************************//**
 * @file my_model_def.h
 * @brief Vendor model definitions
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. [www.silabs.com](http://www.silabs.com)</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: __Zlib__
 *
 * The __licensor__ of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be __misrepresented__; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    __misrepresented__ as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#ifndef MY_MODEL_DEF_H_
#define MY_MODEL_DEF_H_

// provisioning locally
#define PROV_LOCALLY

#define PRIMARY_ELEMENT                 0
#define MY_VENDOR_ID                    0x1111

#define MY_MODEL_SERVER_ID              0x1111
#define MY_MODEL_CLIENT_ID              0x2222

#define CAR_HEALTH_DATA_LENGTH          7
#define LAP_DATA_LENGTH                1

#define NUMBER_OF_OPCODES               7

#define ACK_REQ                         (0x1)
#define STATUS_UPDATE_REQ               (0x2)

#define INDEX_OF(x)                     ((x) - 1)

typedef enum {
  lap_count_get     = 0x1,
  lap_count_status  = 0x2,
  lap_time_get      = 0x3,
  lap_time_status   = 0x4,
  lap_data_update   = 0x5,
  car_health_status = 0x6,
  crash_alert       = 0x7,
} my_msg_t;

typedef struct { // DOS: data structure for values we need to save for each model
  uint16_t elem_index;
  uint16_t vendor_id;
  uint16_t model_id;
  uint8_t publish; // publish - 1, not - 0
  uint8_t opcodes_len;
  uint8_t opcodes_data[NUMBER_OF_OPCODES];
} my_model_t;

#endif /* MY_MODEL_DEF_H_ */
