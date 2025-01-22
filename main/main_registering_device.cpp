#include "cJSON.h"
#include "gatt_client/gatt_client.h" // TODO remove but include here dependencies for nvs_... and ESP_...
#include "gatt_server/gatt_server.h"
#include "http_server/http_server.h"
#include "mqtt/mqtt_manager.h"
#include "persistent_storage/persistent_storage.h"
#include "utils/utils.h"
#include "device_registerer/device_registerer.h"
#include "wifi/wifi_manager.h"
#include <esp_timer.h>
#include <iostream>



extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
    std::cout << "NVS got erased" << std::endl;
  }

  WifiManager::STA::save_credentials("ESP32", "zaq1@WSX");
  WifiManager::STA::start();
  
  DeviceRegisterer::tryRegister();

  PersistentStorage storage("MQTT");
  while (true) {
    vTaskDelay(4000 / portTICK_PERIOD_MS);

    std::string deviceName = storage.getValue("deviceName");
    std::string secretKey = storage.getValue("secretKey");
    std::string accessToken = storage.getValue("accessToken");

    if(deviceName != "" && secretKey != "" && accessToken != "") {
      break;
    }
  }

  WifiManager::STA::stop();
  WifiManager::AP::save_ssid("LEDSync Setup");
  WifiManager::AP::start();

  HttpServer *http_server = new HttpServer();
}
