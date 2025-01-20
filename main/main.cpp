#include "gatt_client.h" // TODO remove but include here dependencies for nvs_... and ESP_...
#include "http_server.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"
#include <iostream>
#include "gatt_server.h"

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
    std::cout << "NVS got erased" << std::endl;
  }


  start_gatt_server();

  WifiManager::STA::save_credentials("ESP32", "zaq1@WSX");
  WifiManager::STA::start();

  MQTTManager* mqtt_manager = new MQTTManager();
  while (!WifiManager::STA::is_connected()) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  mqtt_manager->set_message_callback(
      [](const std::string &topic, const std::string &message) {
        printf("Received message: %s on topic: %s\n", message.c_str(),
               topic.c_str());
      });

  mqtt_manager->connect();

  mqtt_manager->subscribe("v1/devices/me/telemetry");

  

  // delete mqtt_manager;
  while (true) {
    std::cout << "Sending message" << std::endl;
    mqtt_manager->publish("v1/devices/me/telemetry", "{'temperature':25}");
    vTaskDelay(4000 / portTICK_PERIOD_MS);
  }

}
