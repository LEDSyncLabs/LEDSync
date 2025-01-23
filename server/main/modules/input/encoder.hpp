#pragma once

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <functional>
#include <unordered_map>
#include <utility>


struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2> &p) const {
    auto hash1 = std::hash<T1>{}(p.first);
    auto hash2 = std::hash<T2>{}(p.second);
    return hash1 ^ hash2;
  }
};

class Encoder {
public:
  using Callback = std::function<void(int)>;

  Encoder();
  ~Encoder();

  /// @brief Add listener for encoder rotation
  /// @param pinA Clockwise pin
  /// @param pinB Counter-clockwise pin
  /// @param callback The callback function to be called when the encoder is
  /// rotated. Passed 1 for clockwise, -1 for counter-clockwise
  void addListener(gpio_num_t pinA, gpio_num_t pinB, Callback callback);

private:
  void start();

  static void IRAM_ATTR gpio_isr_handler(void *arg);
  static void encoder_task(void *arg);

  static QueueHandle_t gpio_evt_queue;
  static std::unordered_map<std::pair<gpio_num_t, gpio_num_t>, Callback,
                            pair_hash>
      callbacks;
  static volatile int64_t last_interrupt_time;

  static const int64_t DEBOUNCE_THRESHOLD_US = 100 * 1000;

  friend class Input;
};