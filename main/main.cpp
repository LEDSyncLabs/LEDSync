#include "cJSON.h"
#include "gatt_client.h" // TODO remove but include here dependencies for nvs_... and ESP_...
#include "gatt_server.h"
#include "http_server.h"
#include "mqtt_manager.h"
#include "persistent_storage.h"
#include "utils.h"
#include "wifi_manager.h"
#include <esp_timer.h>
#include <iostream>
#include "lcd_management.h"
#include "button.hpp"


#define expireTime 7 * 24 * 60 * 60 * 1000 // 7 days





extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
    std::cout << "NVS got erased" << std::endl;
  }

  int i = 0;  
  LcdManagement::drawWifiInfoWindow();

  Button* button = new Button();  

  button->addListener(GPIO_NUM_1, [&i](int state) {
    i--;    

    if(i%2 == 0){
      LcdManagement::drawDeviceInfoWindow();
    } else {
      LcdManagement::drawWifiInfoWindow();
    }
  });

  button->addListener(GPIO_NUM_2, [&i](int state) {
    i++;

    if(i%2 == 0){
      LcdManagement::drawDeviceInfoWindow();
    } else {
      LcdManagement::drawWifiInfoWindow();
    }
  });
}


