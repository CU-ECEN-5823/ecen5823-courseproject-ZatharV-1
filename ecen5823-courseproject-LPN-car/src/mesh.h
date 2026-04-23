/*
 * mesh.h
 *  Author: Atharv More, Shruti Kalyankar
 */

#ifndef SRC_MESH_H_
#define SRC_MESH_H_

#include "sl_btmesh_api.h"
#include "stdbool.h"
#include "stdint.h"

void car_lpn_init(void);
void handle_mesh_event(sl_btmesh_msg_t *evt);
void mesh_publish_car_health(int16_t x, int16_t y, int16_t z, bool crash);
bool mesh_lpn_is_active(void);

#endif /* SRC_MESH_H_ */
