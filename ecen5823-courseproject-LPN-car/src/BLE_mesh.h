/*
 * BLE_mesh.h
 *
 *  Created on: 20-Apr-2026
 *      Author: athar
 */

#ifndef SRC_BLE_MESH_H_
#define SRC_BLE_MESH_H_

#include "sl_bt_api.h"

#define PROV_LOCALLY

void handle_ble_event(struct sl_bt_msg *evt);
void BTMESH_init();
bool lpn_is_active(void);
void publish_car_health(int16_t x, int16_t y, int16_t z, bool is_crash);

#endif /* SRC_BLE_MESH_H_ */
