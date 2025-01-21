#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <functional>
#include <unordered_map>

class Button {
public:
    using Callback = std::function<void(int)>;

    Button();
    ~Button();

    void addListener(gpio_num_t pin, Callback callback);

private:
    void start();
    
    static void IRAM_ATTR gpio_isr_handler(void* arg);
    static void button_task(void* arg);

    static QueueHandle_t gpio_evt_queue;
    static std::unordered_map<gpio_num_t, Callback> callbacks;
    static volatile int64_t last_interrupt_time;

    static const int64_t DEBOUNCE_THRESHOLD_US = 500000;

    friend class Input;
};