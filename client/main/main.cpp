#include "esp_log.h"
#include "gatt_client/gatt_client.h"
#include "led_strip/led_strip.h"
#include "soc/soc.h"

#define LED_STRIP_LENGTH 30U
#define LED_STRIP_RMT_INTR_NUM 19U

#define PIN_LED_DATA GPIO_NUM_5

static struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
static struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];

led_color_t *current_color = new led_color_t{.red = 255, .green = 0, .blue = 0};
uint8_t current_brightness = 0x00;

void change_color(void *data) {
  led_color_t *colors = (led_color_t *)data;
  current_color->red = colors->red;
  current_color->green = colors->green;
  current_color->blue = colors->blue;
}

void change_brightness(void *data) { current_brightness = *(uint8_t *)data; }

std::map<uint16_t, std::function<void(void *value)>> commandMap = {
    {0xDE01, change_color},
    {0xBE01, change_brightness},
};

void handle_notification(uint16_t handle, void *value) {
  auto it = commandMap.find(handle);
  if (it != commandMap.end()) {
    it->second(value);
  } else {
    ESP_LOGI(GATTC_TAG, "Unknown handle in notify event %x", handle);
  }
}

extern "C" void app_main(void) {
  std::map<uint16_t, std::vector<uint16_t>> service_data = {
      {0xDEAD, {0xDE01}},
      {0xBEEF, {0xBE01}},
  };

  GattClient::on_notify = handle_notification;

  GattClient::create(service_data);

  led_strip_t *led_strip = new led_strip_t{
      .rgb_led_type = RGB_LED_TYPE_WS2812,
      .led_strip_length = LED_STRIP_LENGTH,
      .rmt_channel = RMT_CHANNEL_1,
      .rmt_interrupt_num = LED_STRIP_RMT_INTR_NUM,
      .gpio = PIN_LED_DATA,
      .led_strip_buf_1 = led_strip_buf_1,
      .led_strip_buf_2 = led_strip_buf_2,

  };
  led_strip->access_semaphore = xSemaphoreCreateBinary();
  bool led_init_ok = led_strip_init(led_strip);

  if (!led_init_ok) {
    ESP_LOGW("main", "LED strip initialization failed");

    return;
  }

  led_strip_clear(led_strip);

  while (1) {
    vTaskDelay(30 / portTICK_PERIOD_MS);

    led_color_t color = {
        .red = (uint8_t)(current_color->red * (int)current_brightness / 255),
        .green =
            (uint8_t)(current_color->green * (int)current_brightness / 255),
        .blue = (uint8_t)(current_color->blue * (int)current_brightness / 255),
    };

    // ESP_LOGI("led", "%d %d %d", color.red, color.green, color.blue);

    for (uint32_t i = 0; i < LED_STRIP_LENGTH; ++i) {
      led_strip_set_pixel_color(led_strip, i, &color);
    }
    led_strip_show(led_strip);
  }
}
