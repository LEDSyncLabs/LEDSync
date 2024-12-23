#include "gatt_client.h"

#define INVALID_HANDLE 0
#define PROFILE_A_APP_ID 0

const char* GattClient::remote_device_name = "LEDSYNC_SERVER";

bool GattClient::connect = false;
bool GattClient::get_server = false;
GattClient::gattc_profile_inst GattClient::gl_profile_tab[PROFILE_NUM];
std::vector<GattClient::Service> GattClient::services = {};

GattClient::NotifyCallback GattClient::on_notify = nullptr;

esp_ble_scan_params_t GattClient::ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30
};

void GattClient::set_services(const std::map<uint16_t, std::vector<uint16_t>>& service_data) {
    for (const auto& service_datum : service_data) {
        GattClient::Service service = {
            .uuid = {
                .len = ESP_UUID_LEN_16, 
                .uuid = {
                    .uuid16 = service_datum.first
                }
            }, 
            .handle = 0
        };

        for (const auto& characteristic_uuid : service_datum.second) {
            service.characteristics.push_back({
                .uuid = {
                    .len = ESP_UUID_LEN_16, 
                    .uuid = {
                        .uuid16 = characteristic_uuid
                        }
                    }, 
                .handle = 0
            });
        }

        GattClient::services.push_back(service);
    }
}

bool GattClient::start_bt() {
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    ret = esp_bluedroid_enable();
    if (ret) { 
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    // register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return false;
    }

    // register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return false;
    }

    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
        return false;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret) {
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
        return false;
    }

    return true;
}

esp_bt_uuid_t GattClient::get_characteristic_by_handle(uint16_t handle) {
    for (const auto& service : GattClient::services) {
        for (const auto& characteristic : service.characteristics) {
            if (characteristic.handle == handle) {
                return characteristic.uuid;
            }
        }
    }
    return esp_bt_uuid_t();
}

void GattClient::create(const std::map<uint16_t, std::vector<uint16_t>>& service_data) {
    set_services(service_data);
    
    gl_profile_tab[PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,
    };

    bool bt_started = start_bt();

    if (!bt_started) {
        ESP_LOGE(GATTC_TAG, "Failed to start BT, stopping GattClient");
        return;
    }
}

void GattClient::gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
        case ESP_GATTC_REG_EVT: {

            ESP_LOGI(GATTC_TAG, "REG_EVT");
            esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
            if (scan_ret) {
                ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
            }
            break;
        }
        case ESP_GATTC_CONNECT_EVT: {
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
            gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
            memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
            ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
            esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, p_data->connect.conn_id);
            if (mtu_ret) {
                ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
            }
            break;
        }
        case ESP_GATTC_OPEN_EVT: {
            if (param->open.status != ESP_GATT_OK) {
                ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "open success");
            break;
        }
        case ESP_GATTC_DIS_SRVC_CMPL_EVT: {
            if (param->dis_srvc_cmpl.status != ESP_GATT_OK) {
                ESP_LOGE(GATTC_TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d", param->dis_srvc_cmpl.conn_id);
            for (auto& service : GattClient::services) {
                esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &service.uuid);
            }
            break;
        }
        case ESP_GATTC_CFG_MTU_EVT: {
            if (param->cfg_mtu.status != ESP_GATT_OK) {
                ESP_LOGE(GATTC_TAG, "config mtu failed, error status = %x", param->cfg_mtu.status);
            }
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d",
                    param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
            break;
        }
        case ESP_GATTC_SEARCH_RES_EVT: {
            ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d",
                    p_data->search_res.conn_id, p_data->search_res.is_primary);
            ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d",
                    p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
            for (const auto& service : GattClient::services) {
                if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 
                    && p_data->search_res.srvc_id.uuid.uuid.uuid16 == service.uuid.uuid.uuid16
                ) {
                    ESP_LOGI(GATTC_TAG, "service found");
                    get_server = true;
                    gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
                    gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
                    ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
                }
            }
            break;
        }
        case ESP_GATTC_SEARCH_CMPL_EVT: {
            if (p_data->search_cmpl.status != ESP_GATT_OK) {
                ESP_LOGE(GATTC_TAG, "search service failed, error status = %x",
                        p_data->search_cmpl.status);
                break;
            }
            if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
                ESP_LOGI(GATTC_TAG, "Get service information from remote device");
            } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
                ESP_LOGI(GATTC_TAG, "Get service information from flash");
            } else {
                ESP_LOGI(GATTC_TAG, "unknown service source");
            }
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
            if (GattClient::get_server) {
                uint16_t count = 0;
                esp_gatt_status_t status = esp_ble_gattc_get_attr_count(
                    gattc_if, p_data->search_cmpl.conn_id, ESP_GATT_DB_CHARACTERISTIC,
                    gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                    gl_profile_tab[PROFILE_A_APP_ID].service_end_handle, INVALID_HANDLE,
                    &count);
                if (status != ESP_GATT_OK) {
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
                }

                if (count > 0) {
                    esp_gattc_char_elem_t *char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                    if (!char_elem_result) {
                        ESP_LOGE(GATTC_TAG, "gattc no mem");
                    } else {
                        for (const auto& service : GattClient::services) {
                            for (const auto& characteristic : service.characteristics) {
                                status = esp_ble_gattc_get_char_by_uuid(
                                    gattc_if, p_data->search_cmpl.conn_id,
                                    gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                    gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                    characteristic.uuid, char_elem_result, &count);
                                if (status != ESP_GATT_OK) {
                                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error %x %x",
                                            char_elem_result->uuid.uuid.uuid16, characteristic.uuid.uuid.uuid16);
                                }

                                if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)) {
                                    esp_ble_gattc_register_for_notify(
                                        gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda,
                                        char_elem_result[0].char_handle);
                                }
                            }
                        }
                        free(char_elem_result);
                    }
                } else {
                    ESP_LOGE(GATTC_TAG, "no char found");
                }
            }
            break;
        }
        case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
            if (p_data->reg_for_notify.status != ESP_GATT_OK) {
                ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
            } else {
                uint16_t count = 0;
                uint16_t notify_en = 1;
                esp_err_t ret_status = esp_ble_gattc_get_attr_count(
                    gattc_if, gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                    ESP_GATT_DB_DESCRIPTOR,
                    gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                    gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                    gl_profile_tab[PROFILE_A_APP_ID].char_handle, &count);
                if (ret_status != ESP_GATT_OK) {
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
                }
                if (count > 0) {
                    esp_gattc_descr_elem_t* descr_elem_result = new esp_gattc_descr_elem_t[count];
                    if (!descr_elem_result) {
                        ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                    } else {
                        ret_status = esp_ble_gattc_get_descr_by_char_handle(
                            gattc_if, gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                            p_data->reg_for_notify.handle, get_characteristic_by_handle(p_data->reg_for_notify.handle),
                            descr_elem_result, &count);
                        if (ret_status != ESP_GATT_OK) {
                            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                        }

                        if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 &&
                            descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG
                        ) {
                            ret_status = esp_ble_gattc_write_char_descr(
                                gattc_if, gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                descr_elem_result[0].handle, sizeof(notify_en),
                                (uint8_t *)&notify_en, ESP_GATT_WRITE_TYPE_RSP,
                                ESP_GATT_AUTH_REQ_NONE);
                        }

                        if (ret_status != ESP_GATT_OK) {
                            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                        }

                        delete[] descr_elem_result;
                    }
                } else {
                    ESP_LOGE(GATTC_TAG, "decsr not found");
                }
            }
            break;
        }
        case ESP_GATTC_NOTIFY_EVT: {
            if (p_data->notify.is_notify) {
                ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
            } else {
                ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
            }

            uint16_t handle = p_data->notify.handle;
            ESP_LOGI(GATTC_TAG, "Notify handle is %x", handle);

            uint8_t value = *((uint8_t *)p_data->notify.value);
            esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
            ESP_LOGI(GATTC_TAG, "Converted value is %d", value);

            if (on_notify) {
                on_notify(handle, value);
            }

            break;
        }
        case ESP_GATTC_WRITE_DESCR_EVT: {
            if (p_data->write.status != ESP_GATT_OK) {
                ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "write descr success ");
            uint8_t write_char_data[35];
            for (int i = 0; i < sizeof(write_char_data); ++i) {
                write_char_data[i] = i % 256;
            }
            esp_ble_gattc_write_char(gattc_if, gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                    gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                    sizeof(write_char_data), write_char_data,
                                    ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
            break;
        }
        case ESP_GATTC_SRVC_CHG_EVT: {
            esp_bd_addr_t bda;
            memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
            esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
            break;
        }
        case ESP_GATTC_WRITE_CHAR_EVT: {
            if (p_data->write.status != ESP_GATT_OK) {
                ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "write char success ");
            break;
        }
        case ESP_GATTC_DISCONNECT_EVT: {
            GattClient::connect = false;
            GattClient::get_server = false;
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
            break;
        }
        default: {
            ESP_LOGW(GATTC_TAG, "Unhandled event: %d", event);
            break;
        }
    }
}

void GattClient::esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        // the unit of the duration is second
        uint32_t duration = 30;
        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        // scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "scan start success");

        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
            case ESP_GAP_SEARCH_INQ_RES_EVT:
                esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
                ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d",
                        scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
                adv_name = esp_ble_resolve_adv_data_by_type(
                    scan_result->scan_rst.ble_adv, 
                    scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                    ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

                ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
                esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
                ESP_LOGI(GATTC_TAG, "\n");

                if (adv_name != NULL) {
                    if (strlen(GattClient::remote_device_name) == adv_name_len && 
                        strncmp((char *)adv_name, GattClient::remote_device_name, adv_name_len) == 0
                    ) {
                        ESP_LOGI(GATTC_TAG, "searched device %s\n", GattClient::remote_device_name);
                        if (GattClient::connect == false) {
                            GattClient::connect = true;
                            ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                            esp_ble_gap_stop_scanning();
                            esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, 
                                            scan_result->scan_rst.bda, 
                                            scan_result->scan_rst.ble_addr_type, true);
                        }
                    }
                }
                break;
            case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                break;
            default:
                break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop scan successfully");
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop adv successfully");
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(
            GATTC_TAG,
            "update connection params status = %d, conn_int = %d, latency = %d, "
            "timeout = %d",
            param->update_conn_params.status, param->update_conn_params.conn_int,
            param->update_conn_params.latency, param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
        ESP_LOGI(GATTC_TAG, "packet length updated: rx = %d, tx = %d, status = %d",
                param->pkt_data_length_cmpl.params.rx_len,
                param->pkt_data_length_cmpl.params.tx_len,
                param->pkt_data_length_cmpl.status);
        break;
    default:
        break;
    }
}

void GattClient::esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }

    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}
