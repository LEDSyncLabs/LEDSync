#include "wifi_manager.h"

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdexcept>
#include <string.h>
#include <string>

// Funkcja obsługi zdarzeń
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    printf("WiFi disconnected. Attempting to reconnect...\n");
    esp_wifi_connect();
    // WifiManager::STA::start();
  }
}

bool WifiManager::STA::load_credentials(std::string &ssid,
                                        std::string &password) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
  if (err != ESP_OK)
    return false;

  char ssid_buf[32], password_buf[64];
  size_t ssid_len = sizeof(ssid_buf), password_len = sizeof(password_buf);

  if (nvs_get_str(nvs_handle, WIFI_STA_SSID_KEY, ssid_buf, &ssid_len) ==
          ESP_OK &&
      nvs_get_str(nvs_handle, WIFI_STA_PASSWORD_KEY, password_buf,
                  &password_len) == ESP_OK) {
    ssid = ssid_buf;
    password = password_buf;
    nvs_close(nvs_handle);
    return true;
  }

  nvs_close(nvs_handle);
  return false;
}

void WifiManager::STA::save_credentials(const std::string &ssid,
                                        const std::string &password) {
  nvs_handle_t nvs_handle;
  nvs_open("storage", NVS_READWRITE, &nvs_handle);

  nvs_set_str(nvs_handle, WIFI_STA_SSID_KEY, ssid.c_str());
  nvs_set_str(nvs_handle, WIFI_STA_PASSWORD_KEY, password.c_str());
  nvs_commit(nvs_handle);

  nvs_close(nvs_handle);
}

bool WifiManager::STA::has_saved_credentials() {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
  if (err != ESP_OK)
    return false;

  char ssid_buf[32], password_buf[64];
  size_t ssid_len = sizeof(ssid_buf), password_len = sizeof(password_buf);

  if (nvs_get_str(nvs_handle, WIFI_STA_SSID_KEY, ssid_buf, &ssid_len) ==
          ESP_OK &&
      nvs_get_str(nvs_handle, WIFI_STA_PASSWORD_KEY, password_buf,
                  &password_len) == ESP_OK) {
    nvs_close(nvs_handle);
    return true;
  }

  nvs_close(nvs_handle);
  return false;
}

bool WifiManager::STA::is_connected() {
  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    return true;
  }
  return false;
}

bool WifiManager::STA::is_started() {
  wifi_mode_t mode;
  esp_wifi_get_mode(&mode);
  return mode == WIFI_MODE_STA;
}

bool WifiManager::STA::start() {
  std::string ssid, password;

  if (!WifiManager::STA::load_credentials(ssid, password)) {
    printf("No saved credentials found. Save first");
    return false;
  }

  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

   // Rejestracja handlera zdarzeń
  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &event_handler, NULL, &instance_any_id);

  wifi_config_t sta_config = {};
  strcpy(reinterpret_cast<char *>(sta_config.sta.ssid), ssid.c_str());
  strcpy(reinterpret_cast<char *>(sta_config.sta.password), password.c_str());

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &sta_config);
  esp_wifi_start();

  while (true) {
    esp_err_t ret = esp_wifi_connect();
    if (ret == ESP_OK) {
      wifi_ap_record_t ap_info;
      if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        printf("Connected to SSID: %s\n", ap_info.ssid);
        return true; // Połączenie udane
      } else {
        printf("Failed to get AP info with cred:\n");
        printf("SSID: %s\n", ssid.c_str());
        printf("Password: %s\n", password.c_str());
        printf("Retrying...\n");
      }
    } else {
      printf("Failed to connect to WiFi. Retrying...\n");
    }

    vTaskDelay(pdMS_TO_TICKS(2000)); // Odczekaj 2 sekundy przed kolejną próbą
  }

  // printf("Failed to connect after %d retries.\n", max_retries);
  printf("Press the 'change mode button' to enter AP mode while rebooting.\n");

  return false;
}

void WifiManager::STA::stop() {
  esp_wifi_stop();
  esp_wifi_deinit();
}

bool WifiManager::AP::load_ssid(std::string &ssid) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
  if (err != ESP_OK)
    return false;

  char ssid_buf[32];
  size_t ssid_len = sizeof(ssid_buf);

  if (nvs_get_str(nvs_handle, WIFI_AP_SSID_KEY, ssid_buf, &ssid_len) ==
      ESP_OK) {
    ssid = ssid_buf;
    nvs_close(nvs_handle);
    return true;
  }

  nvs_close(nvs_handle);
  return false;
}

void WifiManager::AP::save_ssid(const std::string &ssid) {
  nvs_handle_t nvs_handle;
  nvs_open("storage", NVS_READWRITE, &nvs_handle);

  nvs_set_str(nvs_handle, WIFI_AP_SSID_KEY, ssid.c_str());
  nvs_commit(nvs_handle);

  nvs_close(nvs_handle);
}

bool WifiManager::AP::is_started() {
  wifi_mode_t mode;
  esp_wifi_get_mode(&mode);
  return mode == WIFI_MODE_AP;
}

void WifiManager::AP::start() {
  // get AP name
  std::string ap_ssid;
  if (load_ssid(ap_ssid)) {
    printf("AP SSID: %s\n", ap_ssid.c_str());
  } else {
    printf("AP SSID not found. Saving default name.\n");
    ap_ssid = WIFI_SETUP_AP_SSID;
    save_ssid(ap_ssid);
  }

  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  wifi_config_t ap_config = {};
  strcpy(reinterpret_cast<char *>(ap_config.ap.ssid), ap_ssid.c_str());
  ap_config.ap.ssid_len = strlen(ap_ssid.c_str());
  ap_config.ap.max_connection = 4;
  ap_config.ap.authmode = WIFI_AUTH_OPEN;

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();
  // wifi_ap_record_t ap_info;
  // if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
  //   printf("AP is applied with SSID: %s\n", ap_info.ssid);
  // } else {
  //   printf("Failed to get AP info.\n");
  // }
}

void WifiManager::AP::stop() {
  esp_wifi_stop();
  esp_wifi_deinit();
}
