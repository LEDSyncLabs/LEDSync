#include "modules/gatt_client/gatt_client.h"
#include "modules/types/ble_types.h"

gpio_num_t LED_A = GPIO_NUM_16;
gpio_num_t LED_B = GPIO_NUM_5;

void init_leds() {
  gpio_config_t io_conf_a = {
      .pin_bit_mask = (1ULL << GPIO_NUM_16),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io_conf_a));
  gpio_set_level(LED_A, 0);

  gpio_config_t io_conf_b = {.pin_bit_mask = (1ULL << LED_B),
                             .mode = GPIO_MODE_OUTPUT};
  ESP_ERROR_CHECK(gpio_config(&io_conf_b));
  gpio_set_level(LED_B, 0);
}

void changeLeds(void *value) {
  RGB_LED *led = (RGB_LED *)value;

  printf("nice red: %d\n", led->red);
  printf("nice green: %d\n", led->green);
  printf("nice blue: %d\n", led->blue);
}

void changeBrightness(void *value) {
  brightness *bright = (brightness *)value;

  printf("nice brightness: %f\n", bright->value);
}

std::map<uint16_t, std::function<void(void *value)>> commandMap = {
    {0xDE01, changeLeds},
    {0xDE02, changeBrightness},
    {0xBE01, changeLeds},
    {0xBE02, changeBrightness},
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
      {0xDEAD, {0xDE01, 0xDE02}},
      {0xBEEF, {0xBE01, 0xBE02}},
  };

  GattClient::on_notify = handle_notification;

  GattClient::create(service_data);

  init_leds();
}


