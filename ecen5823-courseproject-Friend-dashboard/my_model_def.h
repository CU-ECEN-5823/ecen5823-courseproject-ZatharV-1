#ifndef MY_MODEL_DEF_H_
#define MY_MODEL_DEF_H_

#define PROV_LOCALLY

#define PRIMARY_ELEMENT     0
#define MY_VENDOR_ID        0x1111
#define MY_MODEL_SERVER_ID  0x1111
#define MY_MODEL_CLIENT_ID  0x2222

#define LAP_DATA_LENGTH         8
#define CAR_HEALTH_DATA_LENGTH  7

#define NUMBER_OF_OPCODES   7

#define ACK_REQ             (0x1)
#define STATUS_UPDATE_REQ   (0x2)

typedef enum {
  lap_count_get     = 0x1,
  lap_count_status  = 0x2,
  lap_time_get      = 0x3,
  lap_time_status   = 0x4,
  lap_data_update   = 0x5,
  car_health_status = 0x6,
  crash_alert       = 0x7,
} my_msg_t;

typedef struct {
  uint16_t elem_index;
  uint16_t vendor_id;
  uint16_t model_id;
  uint8_t publish;
  uint8_t opcodes_len;
  uint8_t opcodes_data[NUMBER_OF_OPCODES];
} my_model_t;

#endif
