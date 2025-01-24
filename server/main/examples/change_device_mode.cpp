
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "esp_gatt_defs.h"
#include "esp_log.h"
#include "input/input.hpp"
#include "menu/menu.h"
#include "mic/ADCSampler.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <inttypes.h>
#include "persistent_storage/persistent_storage.h"
#include <esp_system.h>
#include "wifi/wifi_manager.h"
#include "http_server/http_server.h"


#define NORMAL_MODE "normal"
#define CONFIG_MODE "config"

#define RESET_BUTTON_PIN GPIO_NUM_23

#define TAG_MAIN "main"


extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();

    ESP_LOGW(TAG_MAIN, "NVS got erased");
  }

  PersistentStorage storage("main");
  std::string mode = storage.getValue("mode");

  if(mode == "") {
    mode = NORMAL_MODE;
    storage.saveValue("mode", mode);
  }

  std::string ssid, password;
  WifiManager::STA::load_credentials(ssid, password);

  if(mode == NORMAL_MODE && ssid != "" && password != "") {   
    WifiManager::STA::start();
  } else {
    WifiManager::AP::save_ssid("LEDSync Setup");
    WifiManager::AP::start();
    HttpServer* httpServer = new HttpServer();
  }

  Input::button.addListener(RESET_BUTTON_PIN, [](int value) {
    ESP_LOGI(TAG_MAIN, "Button pressed, value: %d", value);
      
    PersistentStorage storage("main");
    std::string mode = storage.getValue("mode");

    if(mode == NORMAL_MODE) {
      mode = CONFIG_MODE;
    } else {
      mode = NORMAL_MODE;
    }

    storage.saveValue("mode", mode);
    ESP_LOGI(TAG_MAIN, "Mode changed to: %s", mode.c_str());
    esp_restart();
  });

  Input::start();
}