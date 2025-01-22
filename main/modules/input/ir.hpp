// #pragma once

// #include "driver/rmt_rx.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "esp_log.h"
// #include <functional>
// #include <vector>

// class IR {
// public:
//     using Callback = std::function<void(const rmt_rx_done_event_data_t&)>;

//     IR();
//     ~IR();

//     void addListener(gpio_num_t pin, Callback callback);
//     void start();

// private:
//     struct Listener {
//         gpio_num_t pin;
//         Callback callback;
//         rmt_channel_handle_t rx_channel;
//     };

//     static void ir_task(void* arg);
//     static bool rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data);

//     static QueueHandle_t gpio_evt_queue;
//     static std::vector<Listener> listeners;
//     static rmt_symbol_word_t raw_symbols[64]; // Buffer for RMT symbols
// };