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

    /// @brief Add listener for button press
    /// @param pin The pin to listen for
    /// @param callback The callback function to be called when the button is pressed. Passed 1 for pressed, 0 for released
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