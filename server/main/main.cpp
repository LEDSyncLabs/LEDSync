#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_gatt_defs.h"
#include "esp_log.h"
#include "input/input.hpp"
#include "menu/menu.h"
#include "nvs.h"
#include "nvs_flash.h"

#define PIN_MIC_OUT ADC2_CHANNEL_4

#define PIN_SELECT_LEFT GPIO_NUM_21
#define PIN_SELECT_RIGHT GPIO_NUM_22
#define PIN_SELECT_ABORT GPIO_NUM_18
#define PIN_SELECT_CONFIRM GPIO_NUM_5

#define PIN_ENCODER_RIGHT GPIO_NUM_17
#define PIN_ENCODER_LEFT GPIO_NUM_16

#define PIN_POTENTIOMETER_RIGHT ADC2_CHANNEL_0
#define PIN_POTENTIOMETER_LEFT ADC2_CHANNEL_2

#define PIN_IR_RECEIVER ADC2_CHANNEL_3

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

  Input::encoder.addListener(PIN_ENCODER_RIGHT, PIN_ENCODER_LEFT,
                             encoder_rotated);

  // Input::potentiometer.addListener(PIN_POTENTIOMETER_LEFT,
  //                                  potentiometer_changed, 500);
  // Input::potentiometer.addListener(PIN_POTENTIOMETER_RIGHT,
  //                                  potentiometer_changed, 500);

  Input::ir.addListener(ir_handler);

  Input::start();
}