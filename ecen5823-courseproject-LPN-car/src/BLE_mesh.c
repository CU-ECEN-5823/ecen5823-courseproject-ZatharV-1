/*
 * BLE_mesh.c
 *
 *  Created on: 20-Apr-2026
 *
 *
 *  @file       BLE_mesh.c
 *
 *  @author     Shruti Kalyankar, shruti.kalyankar@colorado.edu
 *
 *  @brief      All BT mesh function and operations in this file
 */




#include <stdio.h> // DOS: for snprintf()

#include "em_common.h"
#include "app_assert.h"
#include "app_log.h"
#include "sl_status.h"
#include "app.h"
#include "src/gpio.h"
#include "sl_btmesh_lpn.h"

#include "sl_btmesh_api.h"
#include "sl_bt_api.h"
#include "sl_simple_timer.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_rtcc.h"
#include "lcd.h"
#include "MMA8452.h"

#include "my_model_def.h"

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
#define CUSTOM_CTRL_GRP_ADDR_CAR                    0xC003  // Server SUB address

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

// Used button indexes
#define BUTTON_PRESS_BUTTON_0                       0
#define BUTTON_PRESS_BUTTON_1                       1

uint8_t conn_handle = 0xFF;

static bool lpn_active = false;

// DOS: The Client can send an "update interval" message to the Server, and the Server
//      will respond with temp measurement updates at this rate. The Server
//      initially starts out not sending periodic updates, but once the Client
//      sends the first "update interval" command to the Server, the Server sends
//      temp measurements periodically. Repeated sending of update period commands
//      cycles through this table until periodic updates are off again (end of the table),
//      and then the cycle starts again.
//
//static uint8_t periods[] = {
//  SET_100_MILLI(3),        /* 300ms */
//  0,
//  SET_100_MILLI(20),       /* 2s    */
//  0,
//  SET_1_SEC(10),           /* 10s   */
//  0,
//  SET_10_SEC(12),          /* 2min  */
//  0,
//  SET_10_MIN(1),           /* 10min */
//  0
//};

bool lpn_is_active(void);


// REPLACE with:
my_model_t my_model = {
  .elem_index = PRIMARY_ELEMENT,
  .vendor_id = MY_VENDOR_ID,
  .model_id = MY_MODEL_CLIENT_ID,
  .publish = 1,
  .opcodes_len = NUMBER_OF_OPCODES,
  .opcodes_data[0] = lap_count_get,
  .opcodes_data[1] = lap_count_status,
  .opcodes_data[2] = lap_time_get,
  .opcodes_data[3] = lap_time_status,
  .opcodes_data[4] = lap_data_update,
  .opcodes_data[5] = car_health_status,
  .opcodes_data[6] = crash_alert,
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


// DOS:
// global
uint32_t logging_timestamp = 0; // in ms

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

bool lpn_is_active(void)
{
    return lpn_active;
}

// DOS:
/**************************************************************************//**
 * Public function to retrieve the timestamp
 *****************************************************************************/
uint32_t get_logger_timestamp() {
  return logging_timestamp;
}


void BTMESH_init()
{
  app_log("=================\r\n");
  app_log("Car LPN\r\n");

  // Run a periodic timer for logging timestamp
//  static sl_simple_timer_t logging_timer;
//  sl_simple_timer_start(&logging_timer,
//                        500,   // every 500 ms
//                        logging_timer_cb, // a callback that increments a timestamp for logging
//                        NULL,  // pointer to callback data
//                        true); // is periodic
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

      //display
      displayInit();  // Initialize Display
      displayPrintf(DISPLAY_ROW_NAME, "Car LPN");
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, "Course Project");

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
    // Default event handler.
    default:
      break;

  } // switch

} // sl_bt_on_event()

void publish_car_health(int16_t x, int16_t y, int16_t z, bool is_crash)
{
  if(is_crash)
  {
      displayPrintf(DISPLAY_ROW_CRASH, "*** IMPACT ***");
  }
  displayPrintf(DISPLAY_ROW_ACCELERATION, "G:%d,%d,%d", x, y, z);


    uint8_t payload[CAR_HEALTH_DATA_LENGTH];
    payload[0] = (x >> 8) & 0xFF;
    payload[1] = x & 0xFF;
    payload[2] = (y >> 8) & 0xFF;
    payload[3] = y & 0xFF;
    payload[4] = (z >> 8) & 0xFF;
    payload[5] = z & 0xFF;
    payload[6] = is_crash ? 1 : 0;

    uint8_t opcode = is_crash ? crash_alert : car_health_status;

    sl_status_t sc = sl_btmesh_vendor_model_set_publication(
        my_model.elem_index,
        my_model.vendor_id,
        my_model.model_id,
        opcode,
        1,
        CAR_HEALTH_DATA_LENGTH,
        payload
    );
    if (sc != SL_STATUS_OK) {
        app_log("Set pub error: 0x%04lX\r\n", sc);
        return;
    }
    sc = sl_btmesh_vendor_model_publish(
        my_model.elem_index,
        my_model.vendor_id,
        my_model.model_id
    );
    if (sc != SL_STATUS_OK) {
        app_log("Publish error: 0x%04lX\r\n", sc);
    } else {
        app_log("Published: x=%d y=%d z=%d crash=%d\r\n", x, y, z, is_crash);
    }
}

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
      if (!sc && pub_address == CUSTOM_CTRL_GRP_ADDR_CAR) {
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
                                                CUSTOM_CTRL_GRP_ADDR_CAR,
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

    // -------------------------------
    // Handle vendor model message reception event
    case sl_btmesh_evt_vendor_model_receive_id: {

      // DOS: get the pointer to the vendor message
      sl_btmesh_evt_vendor_model_receive_t *rx_evt = (sl_btmesh_evt_vendor_model_receive_t *)&evt->data;

      // DOS: log the received data
      app_log("Client: Vendor model data received.\r\n" // DOS: removed all \t instances
              "  Element index = %d\r\n"
              "  Vendor id = 0x%04X\r\n"
              "  Model id = 0x%04X\r\n"
              "  Source address = 0x%04X\r\n"
              "  Destination address = 0x%04X\r\n"
              "  Destination label UUID index = 0x%02X\r\n"
              "  App key index = 0x%04X\r\n"
              "  Non-relayed = 0x%02X\r\n"
              "  Opcode = 0x%02X\r\n"
              "  Final = 0x%04X\r\n"
              "  Payload: ",
              rx_evt->elem_index,
              rx_evt->vendor_id,
              rx_evt->model_id,
              rx_evt->source_address,
              rx_evt->destination_address,
              rx_evt->va_index,
              rx_evt->appkey_index,
              rx_evt->nonrelayed,
              rx_evt->opcode,
              rx_evt->final);
      for(int i = 0; i < evt->data.evt_vendor_model_receive.payload.len; i++) {
          app_log("%x ", evt->data.evt_vendor_model_receive.payload.data[i]);
      }
      app_log("\r\n");

      // DOS: Handle the opcode from the Server.
      //      This is the code that handles messages/requests/commands from the Server.
      app_log("\r\n"); // DOS, added an extra blank line after message reception

      break;

    } // sl_btmesh_evt_vendor_model_receive_id


    // --------------------------------------------------
    // Handle vendor Friend - Low Power Node (LPN) events
    case sl_btmesh_evt_lpn_friendship_failed_id:
//      memset(friend_status, 0, LCD_ROW_LEN);
//      snprintf(friend_status, LCD_ROW_LEN, "Friend Failed"); // friendship failed
//      sl_btmesh_LCD_write(friend_status, LCD_ROW_2);
      lpn_active = false;
      displayPrintf(DISPLAY_ROW_FRIENDSTATUS, "Friend Failed");
      app_log("  ***Friendship Failed\r\n"); // DOS

      break;

    case sl_btmesh_evt_lpn_friendship_established_id:
//      memset(friend_status, 0, LCD_ROW_LEN);
//      snprintf(friend_status, LCD_ROW_LEN, "Friend Est."); // friendship established
//      sl_btmesh_LCD_write(friend_status, LCD_ROW_2);
      lpn_active = true;
      displayPrintf(DISPLAY_ROW_FRIENDSTATUS, "Friend Est.");
      app_log("  ***Friendship Established\r\n"); // DOS

      break;

    case sl_btmesh_evt_lpn_friendship_terminated_id:

//      memset(friend_status, 0, LCD_ROW_LEN);
//      snprintf(friend_status, LCD_ROW_LEN, "Friend Term."); // friendship terminated
//      sl_btmesh_LCD_write(friend_status, LCD_ROW_2);
      lpn_active = false;

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
  displayPrintf(DISPLAY_ROW_RESETSTATUS, "Factory Reset");
//
//  memset(reset_status, 0, LCD_ROW_LEN);
//  snprintf(reset_status, LCD_ROW_LEN, "Factory Reset");
//  sl_btmesh_LCD_write(reset_status, LCD_ROW_2);

  sl_btmesh_node_reset();

  delay_reset_ms(100);
}

// DOS: Simple timer callback
static void app_reset_timer_cb(sl_simple_timer_t *handle, void *data)
{
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


