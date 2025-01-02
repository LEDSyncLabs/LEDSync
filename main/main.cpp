#include "gatt_client.h"
#include "http_server.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"
#include <iostream>

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
    std::cout << "NVS got erased" << std::endl;
  }


  WifiManager::STA::save_credentials("SkyLink 49", "SkyBros49");
  WifiManager::STA::start();

  MQTTManager* mqtt_manager = new MQTTManager("mqtt://mqtt.eclipseprojects.io");
  while (!WifiManager::STA::is_connected()) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  mqtt_manager->set_message_callback(
      [](const std::string &topic, const std::string &message) {
        printf("Received message: %s on topic: %s\n", message.c_str(),
               topic.c_str());
      });

  mqtt_manager->connect();

  mqtt_manager->subscribe("my_topic");

  mqtt_manager->publish("my_topic", "Hello from ESP32");

  // delete mqtt_manager;

}
