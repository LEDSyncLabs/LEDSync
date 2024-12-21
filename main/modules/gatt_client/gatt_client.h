#ifndef GATT_CLIENT_H
#define GATT_CLIENT_H

#include "driver/gpio.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memory>


#define GATTC_TAG "LEDSYNC_CLIENT"

#define REMOTE_SERVICE_A_UUID 0xDEAD
#define REMOTE_NOTIFY_CHAR_A_UUID 0xDE01

#define REMOTE_SERVICE_B_UUID 0xBEEF
#define REMOTE_NOTIFY_CHAR_B_UUID 0xBE01

#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE 0


class GattClient {
public:
    GattClient();

private:
    static gpio_num_t LED_A;
    static gpio_num_t LED_B;
    static int connected_a;
    static bool connect;
    static bool get_server;
    static esp_gattc_char_elem_t *char_elem_result_a;
    static esp_gattc_char_elem_t *char_elem_result_b;
    static esp_gattc_descr_elem_t *descr_elem_result_b;
    static uint16_t handle_a;
    static uint16_t handle_b;

    static const char* remote_device_name;

    static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
    static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

    static esp_bt_uuid_t remote_filter_service_a_uuid;
    static esp_bt_uuid_t remote_filter_char_a_uuid;
    static esp_bt_uuid_t notify_descr_a_uuid;
    static esp_bt_uuid_t remote_filter_service_b_uuid;
    static esp_bt_uuid_t remote_filter_char_b_uuid;
    static esp_bt_uuid_t notify_descr_b_uuid;
    static esp_ble_scan_params_t ble_scan_params;

    struct gattc_profile_inst {
        esp_gattc_cb_t gattc_cb;
        uint16_t gattc_if;
        uint16_t app_id;
        uint16_t conn_id;
        uint16_t service_start_handle;
        uint16_t service_end_handle;
        uint16_t char_handle;
        esp_bd_addr_t remote_bda;
    };

    static gattc_profile_inst gl_profile_tab[PROFILE_NUM];
};

#endif // GATT_CLIENT_H