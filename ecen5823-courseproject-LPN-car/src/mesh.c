/*
 * mesh.c
 *
 *  Author: Atharv More, Shruti Kalyankar
 *  Car LPN - Mesh roles, friendship, sensor publish
 */

#include "mesh.h"
#include "sl_btmesh_api.h"
#include "sl_bt_api.h"
#include "app_assert.h"
#include "lcd.h"
#include "MMA8452.h"
#include "i2c.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

// LPN retry parameters
#define LPN_FRIEND_TIMEOUT_MS     6000
#define LPN_FRIEND_RETRY_MS       2000
#define LPN_MIN_QUEUE_LENGTH      2
#define LPN_RECEIVE_DELAY_MS      50

static bool lpn_active = false;

/**
 * @brief Called once mesh node is initialized.
 *        Starts LPN and tries to find a Friend node.
 */
void car_lpn_init(void)
{
    sl_status_t sc;

    sc = sl_btmesh_lpn_init();
    app_assert_status_f(sc, "LPN init failed\n");

    LOG_INFO("LPN initialized, searching for friend...\r");
    displayPrintf(DISPLAY_ROW_ACTION, "Finding friend");

    sc = sl_btmesh_lpn_establish_friendship(0);
    app_assert_status_f(sc, "LPN establish friendship failed\n");
}

/**
 * @brief Publish car health data (accel + crash flag) to the mesh network.
 *        Called on periodic timer or crash interrupt.
 */
void mesh_publish_car_health(int16_t x, int16_t y, int16_t z, bool crash)
{
    uint8_t payload[7];
    payload[0] = (x >> 8) & 0xFF;
    payload[1] = x & 0xFF;
    payload[2] = (y >> 8) & 0xFF;
    payload[3] = y & 0xFF;
    payload[4] = (z >> 8) & 0xFF;
    payload[5] = z & 0xFF;
    payload[6] = crash ? 1 : 0;

    sl_status_t sc = sl_btmesh_vendor_model_send(
        0xC000,         // destination_address (group address, set during provisioning)
        -1,             // va_index (-1 = not a virtual address)
        0,              // appkey_index (primary appkey)
        0,              // elem_index (element 0)
        0x02FF,         // vendor_id (Silicon Labs)
        0x0001,         // model_id
        0,              // nonrelayed (0 = relayed)
        0x01,           // opcode
        1,              // final (1 = last or only segment)
        sizeof(payload),
        payload
    );

    if (sc != SL_STATUS_OK) {
        LOG_ERROR("Publish car health failed: 0x%lx\r", sc);
    } else {
        LOG_INFO("Published: x=%d y=%d z=%d crash=%d\r", x, y, z, crash);
    }
}
/**
 * @brief Handle all btmesh events for Car LPN
 */
void handle_mesh_event(sl_btmesh_msg_t *evt)
{
    switch (SL_BT_MSG_ID(evt->header))
    {
        case sl_btmesh_evt_node_initialized_id:
            LOG_INFO("Mesh node initialized\r");
            displayPrintf(DISPLAY_ROW_ACTION, "Mesh Init");

            if (!evt->data.evt_node_initialized.provisioned) {
                // Not provisioned yet — start unprovisioned beacon
                LOG_INFO("Node unprovisioned, starting beaconing\r");
                sl_btmesh_node_start_unprov_beaconing(0x3); // ADV + GATT bearer
                displayPrintf(DISPLAY_ROW_ACTION, "Unprovisioned");
            } else {
                // Already provisioned — init LPN
                LOG_INFO("Node already provisioned\r");
                car_lpn_init();
            }
            break;

        case sl_btmesh_evt_node_provisioned_id:
            LOG_INFO("Node provisioned! Address: 0x%x\r",
                     evt->data.evt_node_provisioned.address);
            displayPrintf(DISPLAY_ROW_ACTION, "Provisioned");
            car_lpn_init();
            break;

        case sl_btmesh_evt_node_provisioning_failed_id:
            LOG_ERROR("Provisioning failed, error: 0x%x\r",
                      evt->data.evt_node_provisioning_failed.result);
            displayPrintf(DISPLAY_ROW_ACTION, "Prov Failed");
            break;

        case sl_btmesh_evt_lpn_friendship_established_id:
            lpn_active = true;
            LOG_INFO("Friendship established!\r");
            displayPrintf(DISPLAY_ROW_ACTION, "Friend found");
            // Now safe to initialize the accelerometer
            MMA8452Q_init();
            break;

        case sl_btmesh_evt_lpn_friendship_failed_id:
            lpn_active = false;
            LOG_ERROR("Friendship failed, retrying...\r");
            displayPrintf(DISPLAY_ROW_ACTION, "No friend");
            // Retry after a delay
            sl_btmesh_lpn_establish_friendship(0);
            break;

        case sl_btmesh_evt_lpn_friendship_terminated_id:
            lpn_active = false;
            LOG_ERROR("Friendship terminated\r");
            displayPrintf(DISPLAY_ROW_ACTION, "Friend lost");
            sl_btmesh_lpn_establish_friendship(0);
            break;

        default:
            break;
    }
}

bool mesh_lpn_is_active(void)
{
    return lpn_active;
}
