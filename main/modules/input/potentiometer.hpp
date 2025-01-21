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

    void addListener(adc2_channel_t channel, Callback callback, int intervalMs = 100);
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