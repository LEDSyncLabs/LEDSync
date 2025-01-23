#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "esp_gatt_defs.h"
#include "esp_log.h"
#include "esp_random.h"
#include "gatt_server/gatt_server.h"
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
#include "device_registerer/device_registerer.h"
#include "mqtt/mqtt_manager.h"
#include <cJSON.h>



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

#define TAG_MAIN "main"


#define NORMAL_MODE "normal"
#define CONFIG_MODE "config"

#define RESET_BUTTON_PIN GPIO_NUM_23


MQTTManager* mqttManagerTelemetry = nullptr;


void button_pressed(int value) {
  ESP_LOGI(TAG_MAIN, "Button pressed, value: %d", value);
}

int counter = 0;
void encoder_rotated(int direction) {
  counter += direction;
  ESP_LOGI(TAG_MAIN, "Encoder rotated, direction: %d, counter: %d", direction,
           counter);
}

void potentiometer_changed(int value) {
  ESP_LOGI(TAG_MAIN, "Potentiometer value: %d", value);
}

void ir_handler(uint16_t command) {
  ESP_LOGI(TAG_MAIN, "IR received, command: %04X", command);
}

void adcWriterTask(void *param) {
  I2SSampler *sampler = (I2SSampler *)param;
  int16_t *samples = (int16_t *)malloc(sizeof(uint16_t) * MIC_SAMPLE_SIZE);

  if (!samples) {
    ESP_LOGI("main", "Failed to allocate memory for samples");
    return;
  }

  while (true) {
    int samples_read = sampler->read(samples, MIC_SAMPLE_SIZE);

    int16_t min = INT16_MAX, max = INT16_MIN;

    for (int16_t i = 0; i < samples_read; i++) {
      int16_t sample = samples[i];

      if (sample > max) {
        max = sample;
      } else if (sample < min) {
        min = sample;
      }
    }

    int diff = max - min;

    ESP_LOGI(TAG_MAIN, "%d, %" PRId16 ", %" PRId16 ", %d", samples_read, min,
             max, diff);
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

  ESP_LOGI("ADC", "%d, %" PRId16 ", %" PRId16 ", %d", count, min, max, diff);

  uint8_t level = diff / 256;

  gatts_indicate_brightness(level);

  cJSON *root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "dbLevel", level);
  char *jsonString = cJSON_Print(root);

  mqttManagerTelemetry->publish("v1/devices/me/telemetry", jsonString);

  cJSON_Delete(root);
  
  free(jsonString);
}




void when_mode_normal(){
  WifiManager::STA::start();
  DeviceRegisterer::tryRegister();

  PersistentStorage storage("MQTT");

  std::string deviceName;
  std::string secretKey;
  std::string accessToken;

  while (true) {
    vTaskDelay(4000 / portTICK_PERIOD_MS);

    deviceName = storage.getValue("deviceName");
    secretKey = storage.getValue("secretKey");
    accessToken = storage.getValue("accessToken");

    if(deviceName != "" && secretKey != "" && accessToken != "") {
      break;
    }
  }

  start_gatt_server();

  mqttManagerTelemetry = new MQTTManager(MQTTManager::url, accessToken);
  mqttManagerTelemetry->connect();  

  new ADCSampler(PIN_MIC_OUT, handle_mic);
}

void when_mode_config(){
  WifiManager::AP::save_ssid("LEDSync Setup");
  WifiManager::AP::start();
  HttpServer* httpServer = new HttpServer();
}


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
    when_mode_normal();
  } else {
    when_mode_config();
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