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
#include <vector>
#include <map>
#include <functional>

#define GATTC_TAG "LEDSYNC_CLIENT"

#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE 0

class GattClient {
public:
    using NotifyCallback = std::function<void(uint16_t handle, uint8_t value)>;

    static void create(const std::map<uint16_t, std::vector<uint16_t>>& services);
    static NotifyCallback on_notify;

private:
    static bool start_bt();
    static void set_services(const std::map<uint16_t, std::vector<uint16_t>>& service_data);
    static esp_bt_uuid_t get_characteristic_by_handle(uint16_t handle);

    struct Attribute {
        esp_bt_uuid_t uuid;
        uint16_t handle;
    };

    struct Service {
        esp_bt_uuid_t uuid;
        uint16_t handle;
        std::vector<Attribute> characteristics;
    };

    static std::vector<Service> services;

    static gpio_num_t LED_A;
    static gpio_num_t LED_B;
    static bool connect;
    static bool get_server;
    static const char* remote_device_name;

    static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
    static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

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