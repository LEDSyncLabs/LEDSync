#include "gatt_client.h" // TODO remove but include here dependencies for nvs_... and ESP_...
#include "http_server.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"
#include <iostream>
#include "gatt_server.h"

gpio_num_t LED_A = GPIO_NUM_16;
gpio_num_t LED_B = GPIO_NUM_5;

std::map<uint16_t, std::function<void(void *value)>> commandMap = {
    {0xDE01, changeLeds},
    {0xDE02, changeBrightness},
    {0xBE01, changeLeds},
    {0xBE02, changeBrightness},
};

void handle_notification(uint16_t handle, void *value) {
  auto it = commandMap.find(handle);
  if (it != commandMap.end()) {
    it->second(value);
  } else {
    ESP_LOGI(GATTC_TAG, "Unknown handle in notify event %x", handle);
  }
}

void init_leds() {
  gpio_config_t io_conf_a = {
      .pin_bit_mask = (1ULL << GPIO_NUM_16),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io_conf_a));
  gpio_set_level(LED_A, 0);

  gpio_config_t io_conf_b = {.pin_bit_mask = (1ULL << LED_B),
                             .mode = GPIO_MODE_OUTPUT};
  ESP_ERROR_CHECK(gpio_config(&io_conf_b));
  gpio_set_level(LED_B, 0);
}

void changeLeds(void *value) {
  RGB_LED *led = (RGB_LED *)value;

  printf("nice red: %d\n", led->red);
  printf("nice green: %d\n", led->green);
  printf("nice blue: %d\n", led->blue);
}

void changeBrightness(void *value) {
  brightness *bright = (brightness *)value;

  printf("nice brightness: %f\n", bright->value);
}

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
