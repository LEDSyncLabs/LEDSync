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

#define PIN_MIC_OUT ADC1_CHANNEL_3

#define TAG_MAIN "main"


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

  ESP_LOGI("ADC", "%d, %" PRId16 ", %" PRId16 ", %d", count, min,
           max, diff);
}

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();

    ESP_LOGW(TAG_MAIN, "NVS got erased");
  }


  new ADCSampler(PIN_MIC_OUT, handle_mic);

  Input::start();
}
