#include "nvs.h"
#include "nvs_flash.h"
#include "esp_gatt_defs.h"
#include <iostream>
#include "menu/menu.h"



extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
    std::cout << "NVS got erased" << std::endl;
  }
}
