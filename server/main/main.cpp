#include "device_registerer/device_registerer.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "esp_gatt_defs.h"
#include "esp_log.h"
#include "esp_random.h"
#include "gatt_server/gatt_server.h"
#include "http_server/http_server.h"
#include "input/input.hpp"
#include "lcd/lcd.h"
#include "menu/menu.h"
#include "mic/ADCSampler.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "persistent_storage/persistent_storage.h"
#include "utils/utils.h"
#include "wifi/wifi_manager.h"
#include <esp_system.h>
#include <inttypes.h>

#define PIN_MIC_OUT ADC1_CHANNEL_3

#define PIN_SELECT_LEFT GPIO_NUM_21
#define PIN_SELECT_RIGHT GPIO_NUM_22
#define PIN_SELECT_ABORT GPIO_NUM_18
#define PIN_SELECT_CONFIRM GPIO_NUM_5

#define PIN_ENCODER_RIGHT GPIO_NUM_17
#define PIN_ENCODER_LEFT GPIO_NUM_16

#define PIN_POTENTIOMETER_RIGHT ADC1_CHANNEL_7
#define PIN_POTENTIOMETER_LEFT ADC1_CHANNEL_6

#define PIN_IR_RECEIVER ADC2_CHANNEL_3

#define MIC_SAMPLE_SIZE 4096
#define MIC_SAMPLE_RATE 16384

#define NORMAL_MODE "normal"
#define CONFIG_MODE "config"

#define TAG_MAIN "main"

static int hue_step = 30;
static float current_h = 0;
static float current_s = 1;
static float current_v = 1;

void hue_change(int value) {
  current_h += hue_step * value;

  if (current_h > 360) {
    current_h -= 360;
  }

  float r = 0, g = 0, b = 0;
  HSVtoRGB(&r, &g, &b, current_h, current_s, current_v);

  led_color_t color = {
      .red = (uint8_t)(r * 0xFF),
      .green = (uint8_t)(g * 0xFF),
      .blue = (uint8_t)(b * 0xFF),
  };

  gatts_indicate_color(color);
}

void ir_handler(uint16_t command) {
  ESP_LOGI(TAG_MAIN, "IR received, command: %04X", command);
}

ADCSampler *adcSampler = NULL;

void handle_mic(int16_t *samples, int count) {
  if (!has_clients()) {
    return;
  }

  int16_t min = INT16_MAX, max = INT16_MIN;

  for (int16_t i = 0; i < count; i++) {
    int16_t sample = samples[i];

    if (sample > max) {
      max = sample;
    } else if (sample < min) {
      min = sample;
    }
  }

  int diff = max - min;

  // ESP_LOGI("ADC", "%d, %" PRId16 ", %" PRId16 ", %d", count, min, max, diff);

  uint8_t level = diff / 256;
  gatts_indicate_brightness(level);
}

PersistentStorage storage("main");

void change_device_mode_and_restart(int value) {
  std::string mode = storage.getValue("mode");

  if (mode == NORMAL_MODE) {
    mode = CONFIG_MODE;
  } else {
    mode = NORMAL_MODE;
  }

  storage.saveValue("mode", mode);
  esp_restart();
}

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();

    ESP_LOGW(TAG_MAIN, "NVS got erased");
  }

  // Input::button.addListener(PIN_SELECT_ABORT, button_pressed);
  // Input::button.addListener(PIN_SELECT_CONFIRM, button_pressed);

  // Input::encoder.addListener(PIN_ENCODER_RIGHT, PIN_ENCODER_LEFT,
  //  encoder_rotated);

  // Input::potentiometer.addListener(PIN_POTENTIOMETER_LEFT,
  //                                  potentiometer_changed, 500);
  // Input::potentiometer.addListener(PIN_POTENTIOMETER_RIGHT,
  //                                  potentiometer_changed, 500);

  // Input::start();

  std::string mode = storage.getValue("mode");
  std::string ssid, password;
  WifiManager::STA::load_credentials(ssid, password);

  if (mode == "") {
    mode = NORMAL_MODE;
    storage.saveValue("mode", mode);
  }

  if (mode == NORMAL_MODE && ssid != "" && password != "") {
    new Menu(PIN_ENCODER_LEFT, PIN_ENCODER_RIGHT);

    WifiManager::STA::start();
    DeviceRegisterer::tryRegister();

    PersistentStorage storage("MQTT");
    while (true) {
      vTaskDelay(250 / portTICK_PERIOD_MS);

      std::string deviceName = storage.getValue("deviceName");
      std::string secretKey = storage.getValue("secretKey");
      std::string accessToken = storage.getValue("accessToken");

      if (deviceName != "" && secretKey != "" && accessToken != "") {
        break;
      }
    }

    start_gatt_server();

    Input::button.addListener(PIN_SELECT_LEFT,
                              [](int value) { hue_change(-1); });
    Input::button.addListener(PIN_SELECT_RIGHT,
                              [](int value) { hue_change(1); });

    new ADCSampler(PIN_MIC_OUT, handle_mic);
  } else {
    mode = CONFIG_MODE;
    storage.saveValue("mode", mode);

    WifiManager::AP::start();
    HttpServer *httpServer = new HttpServer();
  }

  Input::button.addListener(PIN_SELECT_ABORT, change_device_mode_and_restart);

  Input::start();
}
