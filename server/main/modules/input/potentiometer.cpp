#include "potentiometer.hpp"

std::unordered_map<adc2_channel_t, Potentiometer::Listener>
    Potentiometer::listeners;
QueueHandle_t Potentiometer::adc_evt_queue = NULL;

Potentiometer::Potentiometer() {}

Potentiometer::~Potentiometer() {
  if (adc_evt_queue != NULL) {
    vQueueDelete(adc_evt_queue);
  }
}

void Potentiometer::start() {
  if (adc_evt_queue != NULL) {
    ESP_LOGW("Potentiometer", "Potentiometer handler already started");
    return;
  }

  adc_evt_queue = xQueueCreate(10, sizeof(adc2_channel_t));
  xTaskCreate(potentiometer_task, "potentiometer_task", 2048, NULL, 10, NULL);
}

void Potentiometer::addListener(adc2_channel_t channel, Callback callback,
                                int intervalMs) {
  if (adc_evt_queue != NULL) {
    ESP_LOGW("Potentiometer", "Cannot add listener after starting the handler");
    return;
  }

  adc2_config_channel_atten(channel, ADC_ATTEN_DB_12);

  TickType_t delay = pdMS_TO_TICKS(intervalMs);
  listeners[channel] = {callback, delay};
}

int Potentiometer::read(adc2_channel_t channel) {
  adc2_config_channel_atten(channel, ADC_ATTEN_DB_12);

  int value;
  if (adc2_get_raw(channel, ADC_WIDTH_BIT_12, &value) == ESP_OK) {
    return value;
  } else {
    ESP_LOGE("Potentiometer", "Failed to read ADC2 channel [%d]", channel);
    return -1;
  }
}

void Potentiometer::potentiometer_task(void *arg) {
  while (1) {
    if (listeners.empty()) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    TickType_t delay = 0;

    for (const auto &pair : listeners) {
      adc2_channel_t channel = pair.first;
      int value;
      if (adc2_get_raw(channel, ADC_WIDTH_BIT_12, &value) == ESP_OK) {
        // ESP_LOGI("Potentiometer", "Channel [%d] value: %d", channel, value);
        pair.second.callback(value);
      } else {
        ESP_LOGE("Potentiometer", "Failed to read ADC2 channel [%d]", channel);
      }
      delay = pair.second.delay;
    }

    vTaskDelay(delay);
  }
}