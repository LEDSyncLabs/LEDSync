
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_gatt_defs.h"
#include <iostream>
#include "led_strip/led_strip.h"

#define LED_STRIP_LENGTH 17U
#define LED_STRIP_RMT_INTR_NUM 19U

static struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
static struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];



extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
    std::cout << "NVS got erased" << std::endl;
  }
  led_strip_t* led_strip = new led_strip_t {
    .rgb_led_type = RGB_LED_TYPE_WS2812,
    .led_strip_length = LED_STRIP_LENGTH,
    .rmt_channel = RMT_CHANNEL_1,
    .rmt_interrupt_num = LED_STRIP_RMT_INTR_NUM,
    .gpio = GPIO_NUM_21,
    .led_strip_buf_1 = led_strip_buf_1,
    .led_strip_buf_2 = led_strip_buf_2,
    
  };
  led_strip->access_semaphore = xSemaphoreCreateBinary();
  bool led_init_ok = led_strip_init(led_strip);

  if (led_init_ok) {
    led_color_t* color = new led_color_t{ .red = 255, .green = 0, .blue = 0 }; // Red color
    for (uint32_t i = 0; i < LED_STRIP_LENGTH; ++i) {
      led_strip_set_pixel_color(led_strip, i, color);
    }
    led_strip_show(led_strip);
    std::cout << "LED strip initialized" << std::endl;
  } else {
    std::cout << "LED strip initialization failed" << std::endl;
  }
  // while(true) {
  //   vTaskDelay(1000 / portTICK_PERIOD_MS);
  // }
}
