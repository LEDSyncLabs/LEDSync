#pragma once

#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <functional>
#include <unordered_map>

class Potentiometer {
public:
    using Callback = std::function<void(int)>;

    Potentiometer();
    ~Potentiometer();

    /// @brief Start checking the potentiometer value in a loop
    /// @param channel Pin to listen for
    /// @param callback The callback function to be called when the value changes. Passed the value from range 0-4095
    /// @param intervalMs The interval in milliseconds to read the value
    void addListener(adc2_channel_t channel, Callback callback, int intervalMs = 100);
    
    /// @brief Read the potentiometer value
    /// @param channel Pin to read
    /// @return The value of the potentiometer from range 0-4095
    int read(adc2_channel_t channel);

private:
    void start();

    struct Listener {
        Callback callback;
        TickType_t delay;
    };

    static void potentiometer_task(void* arg);

    static std::unordered_map<adc2_channel_t, Listener> listeners;
    static QueueHandle_t adc_evt_queue;

    friend class Input;
};