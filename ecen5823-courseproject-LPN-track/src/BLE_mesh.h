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
void publish_lap_data(uint32_t lap_count, uint32_t lap_time_ms);

#endif /* SRC_BLE_MESH_H_ */
