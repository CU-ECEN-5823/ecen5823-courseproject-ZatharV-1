/*
 * BLE_mesh.c
 *
 *  Created on: 20-Apr-2026
 *      Author: athar
 */




#include <stdio.h> // DOS: for snprintf()

#include "em_common.h"
#include "app_assert.h"
#include "app_log.h"
#include "sl_status.h"
#include "app.h"
#include "src/gpio.h"

#include "sl_btmesh_api.h"
#include "sl_bt_api.h"
#include "sl_simple_timer.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_rtcc.h"

#include "src/my_model_def.h"

#define INCLUDE_LOG_DEBUG 1
#include "log.h"

//#include "app_button_press.h"
//#include "sl_simple_button.h"
//#include "sl_simple_button_instances.h"

//#include "sl_btmesh_wstk_lcd.h"

#include "em_gpio.h" // DOS: for gpio

// DOS added this #define to control counting of
//     sl_btmesh_evt_lpn_friendship_terminated_id events
//#define COUNT_FRIENDSHIP_TERMINATED_EVENTS (1)

#ifdef PROV_LOCALLY
// Group Addresses
// Choose any 16-bit address starting at 0xC000
#define CUSTOM_STATUS_GRP_ADDR                      0xC001  // Server PUB address
#define CUSTOM_CTRL_GRP_ADDR                        0xC002  // Server SUB address

// The default settings of the network and the node
#define NET_KEY_IDX                                 0
#define APP_KEY_IDX                                 0
#define IVI                                         0
#define DEFAULT_TTL                                 5
// #define ELEMENT_ID
#endif // #ifdef PROV_LOCALLY

#define EX_B0_PRESS                                 ((1) << 5)
#define EX_B0_LONG_PRESS                            ((1) << 6)
#define EX_B1_PRESS                                 ((1) << 7)
#define EX_B1_LONG_PRESS                            ((1) << 8)

// Timing
// Check section 4.2.2.2 of Mesh Profile Specification 1.0 for format
#define STEP_RES_100_MILLI                          0
#define STEP_RES_1_SEC                              ((1) << 6)
#define STEP_RES_10_SEC                             ((2) << 6)
#define STEP_RES_10_MIN                             ((3) << 6)

#define STEP_RES_BIT_MASK                           0xC0

// Max x is 63
#define SET_100_MILLI(x)                            (uint8_t)(STEP_RES_100_MILLI | ((x) & (0x3F)))
#define SET_1_SEC(x)                                (uint8_t)(STEP_RES_1_SEC | ((x) & (0x3F)))
#define SET_10_SEC(x)                               (uint8_t)(STEP_RES_10_SEC | ((x) & (0x3F)))
#define SET_10_MIN(x)                               (uint8_t)(STEP_RES_10_MIN | ((x) & (0x3F)))

// Advertising Provisioning Bearer
#define PB_ADV                                      0x1
// GATT Provisioning Bearer
#define PB_GATT                                     0x2


my_model_t my_model = {
  .elem_index = PRIMARY_ELEMENT,
  .vendor_id = MY_VENDOR_ID,
  .model_id = MY_MODEL_CLIENT_ID,  // ✅ Use SERVER not CLIENT
  .publish = 1,
  .opcodes_len = NUMBER_OF_OPCODES,
  .opcodes_data[0] = lap_count_get,
  .opcodes_data[1] = lap_count_status,
  .opcodes_data[2] = lap_time_get,
  .opcodes_data[3] = lap_time_status,
  .opcodes_data[4] = lap_data_update,
  .opcodes_data[5] = lap_reset,
  .opcodes_data[6] = lap_reset_unack,
  .opcodes_data[7] = update_interval_get,
  .opcodes_data[8] = update_interval_set,
  .opcodes_data[9] = update_interval_status
};

#ifdef PROV_LOCALLY
static uint16_t uni_addr = 0;
// DOS: encryption key to use for provisioning. This is terribly insecure
//      >>> NEVER DO THIS FOR A PRODUCTION PRODUCT!!! <<<
static aes_key_128 enc_key = {
  .data = "\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03\x03"
};
#endif


// Globals
bd_addr          myAddress;
uint8_t          myAddressType;


static void factory_reset(void);
static void delay_reset_ms(uint32_t ms);
//static void parse_period(uint8_t interval);
//static void delayApprox(int delay);


// DOS:
// global
//uint32_t logging_timestamp = 0; // in ms

/**************************************************************************//**
 * Logging timer callback
 *****************************************************************************/
//static void logging_timer_cb(sl_simple_timer_t *handle, void *data)
//{
//  (void)handle;
//  (void)data;
//
//  logging_timestamp += 500; // increment the timestamp
//}

// DOS:
/**************************************************************************//**
 * Public function to retrieve the timestamp
 *****************************************************************************/
//uint32_t get_logger_timestamp() {
//  return logging_timestamp;
//}


void BTMESH_init()
{
  app_log("=================\r\n");
  app_log("Track LPN\r\n");
}


/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void handle_ble_event(struct sl_bt_msg *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {

    case sl_bt_evt_system_boot_id:
      // Factory reset the device if Button 0 or 1 is being pressed during reset
//      if((sl_simple_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_PRESSED) || (sl_simple_button_get_state(&sl_button_btn1) == SL_SIMPLE_BUTTON_PRESSED)) {
//          factory_reset();
//          break;
//      }
      // DOS ----------------------------------------
      // Hold down PB0, then press and release the reset button on the main PCB,
      // lastly release PB0.
      if (is_Switch0_pressed()) {

          while (is_Switch0_pressed()) {
              ; // stay here while PB0 is pressed
          }

          // We get here on the release of PB0
          factory_reset();

      }
      // DOS ----------------------------------------

      // Initialize Mesh stack in Node operation mode,
      // wait for initialized event
      app_log("Node init\r\n");
      sc = sl_btmesh_node_init();
      app_assert_status_f(sc, "Failed to init node\r\n");

      // get our BT Device Address and type
      // address type: 0: public
      //               1: static random
      sc = sl_bt_system_get_identity_address(&myAddress, &myAddressType);
      if (sc != SL_STATUS_OK) {
          app_log("sl_bt_system_get_identity_address() returned != 0 status=0x%04x", (unsigned int) sc);
      } else {
//          char   myAddressStr[LCD_ROW_LEN];
//          memset(myAddressStr, 0, LCD_ROW_LEN);
//          // handle % format, no printf() capability in sl_btmesh_LCD_write(), by
//          // using snprintf() prior to calling sl_btmesh_LCD_write() as snprintf() can
//          // handle all of the % format conversion characters
//          snprintf(myAddressStr, LCD_ROW_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
//                   myAddress.addr[5],
//                   myAddress.addr[4],
//                   myAddress.addr[3],
//                   myAddress.addr[2],
//                   myAddress.addr[1],
//                   myAddress.addr[0]
//                   );
//          sl_btmesh_LCD_write(myAddressStr, LCD_ROW_4);
      }

      break;

    // -------------------------------
    // Handle Button Presses
    // DOS: See app_button_press_cb() below.
//    case sl_bt_evt_system_external_signal_id: {
//
//      uint8_t opcode = 0, length = 0, data = 0;
//
//      // DOS: Get the temp measurement from the Server - PB0 press
//      if(evt->data.evt_system_external_signal.extsignals & EX_B0_PRESS) {
//          opcode = temperature_get;
//          app_log("PB0 Pressed.\r\n");
//      }
//
//      // DOS: update interval set in the Server - PB0 long press
//      if(evt->data.evt_system_external_signal.extsignals & EX_B0_LONG_PRESS) {
//          opcode = update_interval_set_unack;
//          length = 1;
//          data = periods[period_idx];
//          if(period_idx == sizeof(periods) - 1) {
//              period_idx = 0;
//          } else {
//              period_idx++;
//          }
////          app_log("PB0 Long Pressed, period_idx=%d.\r\n", period_idx);
//      }
//
//      // DOS: Get the units from the Server - PB1 press
//      if(evt->data.evt_system_external_signal.extsignals & EX_B1_PRESS) {
//          opcode = unit_get;
//          app_log("PB1 Pressed.\r\n");
//      }
//
//      // DOS: Toggle the units in the Server - PB1 long press
//      if(evt->data.evt_system_external_signal.extsignals & EX_B1_LONG_PRESS) {
//          if (unit == celsius) {
//              opcode = unit_set_unack;
//              length = 1;
//              data = fahrenheit;
//          } else {
//            opcode = unit_set;
//            length = 1;
//            data = celsius;
//          }
//          app_log("PB1 Long Pressed.\r\n");
//      }
//
//      // DOS: Set the message data to publish, i.e. queue the data for publishing.
//
//      // Student edit: Un-comment this code to send the Request message to the Server node.
//
//       sc = sl_btmesh_vendor_model_set_publication(my_model.elem_index,
//                                                   my_model.vendor_id,
//                                                   my_model.model_id,
//                                                   opcode,
//                                                   1, // DOS: the final payload "chunk"
//                                                   length,
//                                                   &data);
//       if(sc != SL_STATUS_OK) {
//         app_log("Set publication error: 0x%04lX\r\n", sc);
//       } else {
//         app_log("Set publication done. Publishing...\r\n");
//         // DOS: Publish the queued message to the group address.
//         sc = sl_btmesh_vendor_model_publish(my_model.elem_index,
//                                             my_model.vendor_id,
//                                             my_model.model_id);
//         if (sc != SL_STATUS_OK) {
//           app_log("Publish error = 0x%04lX\r\n", sc);
//         } else {
//           app_log("Publish done.\r\n");
//         }
//
//       } // else
//
//    } // case item
//    break;

    // -------------------------------
    // Default event handler.
    default:
      break;

  } // switch

} // sl_bt_on_event()

/**************************************************************************//**
 * Bluetooth Mesh stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth Mesh stack.
 *****************************************************************************/
void sl_btmesh_on_event(sl_btmesh_msg_t *evt)
{
  sl_status_t      sc;
//  char             friend_status[LCD_ROW_LEN];

#if (COUNT_FRIENDSHIP_TERMINATED_EVENTS == 1)
  static int       friend_term_count = 0; // count of number of friendship termination events
#endif

  switch (SL_BT_MSG_ID(evt->header)) {

    case sl_btmesh_evt_node_initialized_id:
      app_log("Node initialized ...\r\n");

      // DOS: Init the vendor model
      sc = sl_btmesh_vendor_model_init(my_model.elem_index,
                                       my_model.vendor_id,
                                       my_model.model_id,
                                       my_model.publish,
                                       my_model.opcodes_len,
                                       my_model.opcodes_data);
      app_assert_status_f(sc, "Failed to initialize vendor model\r\n");

      if(evt->data.evt_node_initialized.provisioned) {
          app_log("Node already provisioned.\r\n");
      } else {
          // Start unprovisioned Beaconing using PB-ADV and PB-GATT Bearers
          app_log("Node unprovisioned\r\n");

#ifdef PROV_LOCALLY
          // Derive the unicast address from the LSB 2 bytes from the BD_ADDR
          bd_addr address;
          sc = sl_bt_system_get_identity_address(&address, 0);
          uni_addr = ((address.addr[1] << 8) | address.addr[0]) & 0x7FFF;
          app_log("Unicast Address = 0x%04X\r\n", uni_addr);
          app_log("Provisioning itself.\r\n");
          // DOS: The device must be reset after this command has been issued,
          //      see delay_reset_ms() below.
          sc = sl_btmesh_node_set_provisioning_data(enc_key, // DOS: device key
                                                    enc_key, // DOS: network key
                                                    NET_KEY_IDX,
                                                    IVI,
                                                    uni_addr,
                                                    0); // key refresh = false
          app_assert_status_f(sc, "Failed to provision itself\r\n");
          delay_reset_ms(100);
//          delayApprox(100);
//          sl_bt_system_reset(0);
          break;

#else
        app_log("Send unprovisioned beacons.\r\n");
        sc = sl_btmesh_node_start_unprov_beaconing(PB_ADV | PB_GATT);
        app_assert_status_f(sc, "Failed to start unprovisioned beaconing\r\n");
#endif // #ifdef PROV_LOCALLY
      }

#ifdef PROV_LOCALLY
      // Set the publication and subscription
      uint16_t appkey_index;
      uint16_t pub_address;
      uint8_t ttl;
      uint8_t period;
      uint8_t retrans;
      uint8_t credentials;
      sc = sl_btmesh_test_get_local_model_pub(my_model.elem_index,
                                              my_model.vendor_id,
                                              my_model.model_id,
                                              &appkey_index,
                                              &pub_address,
                                              &ttl,
                                              &period,
                                              &retrans,
                                              &credentials);
      if (!sc && pub_address == CUSTOM_CTRL_GRP_ADDR) {
        app_log("Configuration done already.\r\n");
      } else {
        app_log("Pub setting result = 0x%04lX, pub setting address = 0x%04X\r\n", sc, pub_address);
        app_log("Add local app key ...\r\n");
        sc = sl_btmesh_test_add_local_key(1,
                                          enc_key, // DOS: app key
                                          APP_KEY_IDX,
                                          NET_KEY_IDX);
        app_assert_status_f(sc, "Failed to add local app key\r\n");

        app_log("Bind local app key ...\r\n");
        sc = sl_btmesh_test_bind_local_model_app(my_model.elem_index,
                                                 APP_KEY_IDX,
                                                 my_model.vendor_id,
                                                 my_model.model_id);
        app_assert_status_f(sc, "Failed to bind local app key\r\n");

        app_log("Set local model pub ...\r\n");
        sc = sl_btmesh_test_set_local_model_pub(my_model.elem_index,
                                                APP_KEY_IDX,
                                                my_model.vendor_id,
                                                my_model.model_id,
                                                CUSTOM_CTRL_GRP_ADDR,
                                                DEFAULT_TTL,
                                                0, 0, 0);
        app_assert_status_f(sc, "Failed to set local model pub\r\n");

        app_log("Add local model sub ...\r\n");
        sc = sl_btmesh_test_add_local_model_sub(my_model.elem_index,
                                                my_model.vendor_id,
                                                my_model.model_id,
                                                CUSTOM_STATUS_GRP_ADDR);
        app_assert_status_f(sc, "Failed to add local model sub\r\n");

        app_log("Set relay ...\r\n");
        sc = sl_btmesh_test_set_relay(1, 0, 0);
        app_assert_status_f(sc, "Failed to set relay\r\n");

        app_log("Set Network tx state.\r\n");
        sc = sl_btmesh_test_set_nettx(2, 4);
        app_assert_status_f(sc, "Failed to set network tx state\r\n");
      }
#endif // #ifdef PROV_LOCALLY

      break;

    // -------------------------------
    // Provisioning Events
    case sl_btmesh_evt_node_provisioned_id:
      app_log("Provisioning done.\r\n");
      break;

    case sl_btmesh_evt_node_provisioning_failed_id:
      app_log("Provisioning failed. Result = 0x%04x\r\n",
              evt->data.evt_node_provisioning_failed.result);
      break;

    case sl_btmesh_evt_node_provisioning_started_id:
      app_log("Provisioning started.\r\n");
      break;

    case sl_btmesh_evt_node_key_added_id:
      app_log("got new %s key with index %x\r\n",
              evt->data.evt_node_key_added.type == 0 ? "network " : "application ",
              evt->data.evt_node_key_added.index);
      break;

    case sl_btmesh_evt_node_config_set_id:
      app_log("evt_node_config_set_id\r\n\t");
      break;

    case sl_btmesh_evt_node_model_config_changed_id:
      app_log("model config changed, type: %d, elem_addr: %x, model_id: %x, vendor_id: %x\r\n",
              evt->data.evt_node_model_config_changed.node_config_state,
              evt->data.evt_node_model_config_changed.element_address,
              evt->data.evt_node_model_config_changed.model_id,
              evt->data.evt_node_model_config_changed.vendor_id);
      break;

    case sl_btmesh_evt_vendor_model_receive_id:
      {
        uint8_t opcode = evt->data.evt_vendor_model_receive.opcode;
        uint8_t *data = evt->data.evt_vendor_model_receive.payload.data;

        if (opcode == lap_data_update) {
          uint32_t lap_count = data[0] | (data[1] << 8) |
                              (data[2] << 16) | (data[3] << 24);
          uint32_t lap_time = data[4] | (data[5] << 8) |
                             (data[6] << 16) | (data[7] << 24);

          LOG_INFO("Received: Lap %ld, Time %ld.%03lds\r\n",
                   lap_count, lap_time/1000, lap_time%1000);

          // Update Friend node LCD
//          displayPrintf(DISPLAY_ROW_LAPNUM, "Lap: %d", lap_count);
//          displayPrintf(DISPLAY_ROW_LAPTIME, "Time: %d.%03ds",
//                        lap_time/1000, lap_time%1000);
        }
      }
      break;


    // --------------------------------------------------
    // Handle vendor Friend - Low Power Node (LPN) events
    case sl_btmesh_evt_lpn_friendship_failed_id:
//      memset(friend_status, 0, LCD_ROW_LEN);
//      snprintf(friend_status, LCD_ROW_LEN, "Friend Failed"); // friendship failed
//      sl_btmesh_LCD_write(friend_status, LCD_ROW_2);

      app_log("  ***Friendship Failed\r\n"); // DOS

      break;

    case sl_btmesh_evt_lpn_friendship_established_id:
//      memset(friend_status, 0, LCD_ROW_LEN);
//      snprintf(friend_status, LCD_ROW_LEN, "Friend Est."); // friendship established
//      sl_btmesh_LCD_write(friend_status, LCD_ROW_2);

      app_log("  ***Friendship Established\r\n"); // DOS

      break;

    case sl_btmesh_evt_lpn_friendship_terminated_id:

//      memset(friend_status, 0, LCD_ROW_LEN);
//      snprintf(friend_status, LCD_ROW_LEN, "Friend Term."); // friendship terminated
//      sl_btmesh_LCD_write(friend_status, LCD_ROW_2);

#if (COUNT_FRIENDSHIP_TERMINATED_EVENTS == 1)
      friend_term_count++;

      memset(friend_status, 0, LCD_ROW_LEN);
      snprintf(friend_status, LCD_ROW_LEN, "FriTermCount=%d", friend_term_count); // friendship terminated count
      sl_btmesh_LCD_write(friend_status, LCD_ROW_3);

      // log it
      app_log("  ***Friendship terminated, count=%d, time=%d ms\r\n", friend_term_count, get_logger_timestamp());
#endif

      break;



    // -------------------------------
    // Default event handler.
    default:
      break;


  } // switch

} // sl_btmesh_on_event()


// DOS: The App / Utility / Button Press (app_button_press) software component
//      implements a callback scheme for handling button press interrupts. The
//      duration of the presses are measured starting from the "press" until
//      the "release". So we're really getting "button release" interrupts.
//      This callback is called on the release, and the code here switch/cases on
//      the duration of the press and calls sl_bt_external_signal() which is handled
//      in sl_bt_on_event() and the sl_bt_evt_system_external_signal_id event
//      handler.

//void app_button_press_cb(uint8_t button, uint8_t duration)
//{
//  // Selecting action by duration
//  switch (duration) {
//
//    case APP_BUTTON_PRESS_DURATION_SHORT:
//      // Handling of button press less than 0.25s
//    case APP_BUTTON_PRESS_DURATION_MEDIUM:
//      // Handling of button press greater than 0.25s and less than 1s
//      if (button == BUTTON_PRESS_BUTTON_0) {
//        sl_bt_external_signal(EX_B0_PRESS);
//      } else {
//        sl_bt_external_signal(EX_B1_PRESS);
//      }
//      break;
//
//    case APP_BUTTON_PRESS_DURATION_LONG:
//      // Handling of button press greater than 1s and less than 5s
//    case APP_BUTTON_PRESS_DURATION_VERYLONG:
//      if (button == BUTTON_PRESS_BUTTON_0) {
//        sl_bt_external_signal(EX_B0_LONG_PRESS);
//      } else {
//        sl_bt_external_signal(EX_B1_LONG_PRESS);
//      }
//      break;
//
//    default:
//      break;
//  }
//}

/// Reset
static void factory_reset(void)
{
  // DOS moved log statement here + update the LCD here
//  char   reset_status[LCD_ROW_LEN];
//
  app_log("factory reset\r\n");
//
//  memset(reset_status, 0, LCD_ROW_LEN);
//  snprintf(reset_status, LCD_ROW_LEN, "Factory Reset");
//  sl_btmesh_LCD_write(reset_status, LCD_ROW_2);

  sl_btmesh_node_reset();

  delay_reset_ms(100);
//  delayApprox(100);
//  sl_bt_system_reset(0);
}

// DOS: Simple timer callback
static void app_reset_timer_cb(sl_simple_timer_t *handle, void *data)
{
  LOG_INFO("Resetting\r");
  (void)handle;
  (void)data;
  sl_bt_system_reset(0);
}

static sl_simple_timer_t app_reset_timer;
static void delay_reset_ms(uint32_t ms)
{
  if(ms < 10) {
      ms = 10;
  }
  sl_simple_timer_start(&app_reset_timer,
                         ms,
                         app_reset_timer_cb, // DOS: function to call when time is expired
                         NULL,               // pointer to callback data
                         false);             // not periodic, i.e. a one-shot
}


/// Update Interval
//static void parse_period(uint8_t interval)
//{
//  switch (interval & STEP_RES_BIT_MASK) {
//    case STEP_RES_100_MILLI:
//      periodic_timer_ms = 100 * (interval & (~STEP_RES_BIT_MASK));
//      break;
//    case STEP_RES_1_SEC:
//      periodic_timer_ms = 1000 * (interval & (~STEP_RES_BIT_MASK));
//      break;
//    case STEP_RES_10_SEC:
//      periodic_timer_ms = 10000 * (interval & (~STEP_RES_BIT_MASK));
//      break;
//    case STEP_RES_10_MIN:
//      // 10 min = 600000ms
//      periodic_timer_ms = 600000 * (interval & (~STEP_RES_BIT_MASK));
//      break;
//    default:
//      break;
//  }
//  if (periodic_timer_ms) {
//      app_log("Update period [hh:mm:ss:ms]= %02ld:%02ld:%02ld:%04ld\r\n",
//              (periodic_timer_ms / (1000 * 60 * 60)),
//              (periodic_timer_ms % (1000 * 60 * 60)) / (1000 * 60),
//              (periodic_timer_ms % (1000 * 60)) / 1000,
//              ((periodic_timer_ms % (1000)) / 1000) * 100);
//  } else {
//      app_log("  *** Periodic update off.\r\n");
//  }
//}


void publish_lap_data(uint32_t lap_count, uint32_t lap_time_ms)
{
  sl_status_t sc;
  uint8_t data[LAP_DATA_LENGTH];

  // Pack data: [lap_count (4 bytes)] [lap_time (4 bytes)]
  data[0] = (lap_count & 0xFF);
  data[1] = (lap_count >> 8) & 0xFF;
  data[2] = (lap_count >> 16) & 0xFF;
  data[3] = (lap_count >> 24) & 0xFF;
  data[4] = (lap_time_ms & 0xFF);
  data[5] = (lap_time_ms >> 8) & 0xFF;
  data[6] = (lap_time_ms >> 16) & 0xFF;
  data[7] = (lap_time_ms >> 24) & 0xFF;

  sc = sl_btmesh_vendor_model_set_publication(my_model.elem_index,
                                         my_model.vendor_id,
                                         my_model.model_id,
                                         lap_data_update,  // opcode
                                         0,  // final = 0 (not final segment)
                                         LAP_DATA_LENGTH,
                                         data);
  if (sc != SL_STATUS_OK) {
      LOG_ERROR("Set publication error: 0x%04lX\r\n", sc);
    }


  // Publish via vendor model
  sc = sl_btmesh_vendor_model_publish(my_model.elem_index,
                                      my_model.vendor_id,
                                      my_model.model_id);

  if (sc == SL_STATUS_OK) {
    LOG_INFO("Published: Lap %ld, Time %ld.%03lds\r\n",
             lap_count, lap_time_ms/1000, lap_time_ms%1000);
  } else {
    LOG_ERROR("Publish failed: 0x%04lX\r\n", sc);
  }
}
