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

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();

    ESP_LOGW(TAG_MAIN, "NVS got erased");
  }

  // Input::button.addListener(PIN_SELECT_LEFT, button_pressed);
  // Input::button.addListener(PIN_SELECT_RIGHT, button_pressed);
  // Input::button.addListener(PIN_SELECT_ABORT, button_pressed);
  // Input::button.addListener(PIN_SELECT_CONFIRM, button_pressed);

  // Input::encoder.addListener(PIN_ENCODER_RIGHT, PIN_ENCODER_LEFT,
  //  encoder_rotated);

  Input::potentiometer.addListener(PIN_POTENTIOMETER_LEFT,
                                   potentiometer_changed, 500);
  Input::potentiometer.addListener(PIN_POTENTIOMETER_RIGHT,
                                   potentiometer_changed, 500);

  // Input::ir.addListener(ir_handler);

  // i2s_config_t adcI2SConfig = {
  //     .mode =
  //         (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX |
  //         I2S_MODE_ADC_BUILT_IN),
  //     .sample_rate = MIC_SAMPLE_RATE,
  //     .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  //     .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  //     .communication_format = I2S_COMM_FORMAT_I2S_LSB,
  //     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  //     .dma_buf_count = 4,
  //     .dma_buf_len = 1024,
  //     .use_apll = false,
  //     .tx_desc_auto_clear = false,
  //     .fixed_mclk = 0};

  // adcSampler = new ADCSampler(ADC_UNIT_1, PIN_MIC_OUT, adcI2SConfig);

  // TaskHandle_t adcWriterTaskHandle;
  // adcSampler->start();
  // xTaskCreatePinnedToCore(adcWriterTask, "ADC Writer Task", 4096, adcSampler,
  // 1,
  //                         &adcWriterTaskHandle, 1);

  Input::start();
}
