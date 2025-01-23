#include "button.hpp"

QueueHandle_t Button::gpio_evt_queue = NULL;
std::unordered_map<gpio_num_t, Button::Callback> Button::callbacks;
volatile int64_t Button::last_interrupt_time = 0;

const int64_t Button::DEBOUNCE_THRESHOLD_US;

Button::Button() {}

Button::~Button() { vQueueDelete(gpio_evt_queue); }

void Button::start() {
  if (gpio_evt_queue != NULL) {
    ESP_LOGW("Button", "Button handler already started");
    return;
  }

  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
  xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);

  for (const auto &pair : callbacks) {
    gpio_isr_handler_add(pair.first, gpio_isr_handler, (void *)pair.first);
  }
}

void Button::addListener(gpio_num_t pin, Callback callback) {
  if (gpio_evt_queue != NULL) {
    ESP_LOGW("Button", "Cannot add listener after starting the handler");
    return;
  }

  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << pin),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_ENABLE,
      .intr_type = GPIO_INTR_POSEDGE,
  };
  gpio_config(&io_conf);

  callbacks[pin] = callback;
}

void IRAM_ATTR Button::gpio_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  int64_t now = esp_timer_get_time();

  if (now - last_interrupt_time > DEBOUNCE_THRESHOLD_US) {
    last_interrupt_time = now;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
  }
}

void Button::button_task(void *arg) {
  gpio_num_t io_num;
  while (1) {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
      int pin_value = gpio_get_level(io_num);
      // ESP_LOGI("Button", "Pin [%lu] value: %d", io_num, pin_value);
      if (callbacks.find(io_num) != callbacks.end()) {
        callbacks[io_num](pin_value);
      }
    }
  }
}