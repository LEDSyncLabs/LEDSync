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
#include "mqtt/mqtt_manager.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "persistent_storage/persistent_storage.h"
#include "utils/utils.h"
#include "wifi/wifi_manager.h"
#include <cJSON.h>
#include <esp_system.h>
#include <inttypes.h>
#include <numeric>

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

void hue_set(int value) {
  current_h = value;

  if (current_h > 360) {
    current_h -= 360;
  }

  if (current_h < 0) {
    current_h += 360;
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

void hue_change(int value) {
  int new_hue = current_h + hue_step * value;

  hue_set(new_hue);
}

void ir_handler(uint16_t command) {
  ESP_LOGI(TAG_MAIN, "IR received, command: %04X", command);
}

std::vector<uint8_t> volume_samples;
MQTTManager *mqttManagerTelemetry = nullptr;

void send_volume_average() {
  uint64_t sum = std::accumulate(volume_samples.begin(), volume_samples.end(),
                                 uint64_t(0));

  double average = static_cast<double>(sum) / volume_samples.size();

  cJSON *root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "volume", average);
  char *jsonString = cJSON_Print(root);

  mqttManagerTelemetry->publish("v1/devices/me/telemetry", jsonString);

  cJSON_Delete(root);
  free(jsonString);
}

void add_volume_sample(uint8_t level) {
  volume_samples.push_back(level);

  if (volume_samples.size() == 256) {
    send_volume_average();
    volume_samples.clear();
  }
}

ADCSampler *adcSampler = NULL;

void handle_mic(int16_t *samples, int count) {
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

  uint8_t level = diff / 256;

  // ESP_LOGI(TAG_MAIN, "min:%d max:%d diff:%d level:%hu", min, max, diff,
  // level);
  gatts_indicate_brightness(level);
  add_volume_sample(level);
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

  std::string mode = storage.getValue("mode");
  std::string ssid, password;
  WifiManager::STA::load_credentials(ssid, password);

  if (mode == "") {
    mode = NORMAL_MODE;
    storage.saveValue("mode", mode);
  }

  new Menu(PIN_ENCODER_LEFT, PIN_ENCODER_RIGHT);

  if (mode == NORMAL_MODE && ssid != "" && password != "") {

    WifiManager::STA::start();
    DeviceRegisterer::tryRegister();

    PersistentStorage storage("MQTT");
    while (true) {
      std::string deviceName = storage.getValue("deviceName");
      std::string secretKey = storage.getValue("secretKey");
      std::string accessToken = storage.getValue("accessToken");

      if (deviceName != "" && secretKey != "" && accessToken != "") {
        mqttManagerTelemetry = new MQTTManager(MQTTManager::url, accessToken);
        mqttManagerTelemetry->connect();

        break;
      }

      vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    start_gatt_server();

    Input::button.addListener(PIN_SELECT_LEFT,
                              [](int value) { hue_change(-1); });
    Input::button.addListener(PIN_SELECT_RIGHT,
                              [](int value) { hue_change(1); });

    Input::ir.addListener(ir_handler);
    Input::ir.addListener(0xE916, [](uint16_t command) { hue_set(0.0 * 360); });
    Input::ir.addListener(0xF30C, [](uint16_t command) { hue_set(0.1 * 360); });
    Input::ir.addListener(0xE718, [](uint16_t command) { hue_set(0.2 * 360); });
    Input::ir.addListener(0xA15E, [](uint16_t command) { hue_set(0.3 * 360); });
    Input::ir.addListener(0xF708, [](uint16_t command) { hue_set(0.4 * 360); });
    Input::ir.addListener(0xE31C, [](uint16_t command) { hue_set(0.5 * 360); });
    Input::ir.addListener(0xA55A, [](uint16_t command) { hue_set(0.6 * 360); });
    Input::ir.addListener(0xBD42, [](uint16_t command) { hue_set(0.7 * 360); });
    Input::ir.addListener(0xAD52, [](uint16_t command) { hue_set(0.8 * 360); });
    Input::ir.addListener(0xB54A, [](uint16_t command) { hue_set(0.9 * 360); });

    new ADCSampler(PIN_MIC_OUT, handle_mic);
  } else {
    mode = CONFIG_MODE;
    storage.saveValue("mode", mode);

    WifiManager::AP::start();
    HttpServer *httpServer = new HttpServer();
  }

  Input::button.addListener(PIN_SELECT_ABORT, change_device_mode_and_restart);
  Input::ir.addListener(0xB847, change_device_mode_and_restart);

  Input::start();
}
