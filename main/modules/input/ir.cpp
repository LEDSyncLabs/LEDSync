#include "ir.hpp"

#define EXAMPLE_IR_RESOLUTION_HZ 1000000 // 1MHz resolution, 1 tick = 1us
#define EXAMPLE_IR_NEC_DECODE_MARGIN 200 // Tolerance for parsing RMT symbols into bit stream

static const char *TAG = "IR";

QueueHandle_t IR::gpio_evt_queue = NULL;
std::vector<IR::Listener> IR::listeners;
rmt_symbol_word_t IR::raw_symbols[64]; // Buffer for RMT symbols

IR::IR() {}

IR::~IR() {
    vQueueDelete(gpio_evt_queue);
}

void IR::start() {
    if (gpio_evt_queue != NULL) {
        ESP_LOGW("IR", "IR handler already started");
        return;
    }

    gpio_evt_queue = xQueueCreate(10, sizeof(rmt_rx_done_event_data_t));
    xTaskCreate(ir_task, "ir_task", 2048, NULL, 10, NULL);

    for (auto& listener : listeners) {
        rmt_rx_channel_config_t rx_channel_cfg = {
            .gpio_num = listener.pin,
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = EXAMPLE_IR_RESOLUTION_HZ,
            .mem_block_symbols = 64, // amount of RMT symbols that the channel can store at a time
        };
        ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_cfg, &listener.rx_channel));

        rmt_rx_event_callbacks_t cbs = {
            .on_recv_done = rmt_rx_done_callback,
        };
        ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(listener.rx_channel, &cbs, gpio_evt_queue));

        rmt_receive_config_t receive_config = {
            .signal_range_min_ns = 1250,
            .signal_range_max_ns = 12000000,
        };
        ESP_ERROR_CHECK(rmt_enable(listener.rx_channel));
        ESP_ERROR_CHECK(rmt_receive(listener.rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));
    }
}

void IR::addListener(gpio_num_t pin, Callback callback) {
    if (gpio_evt_queue != NULL) {
        ESP_LOGW("IR", "Cannot add listener after starting the handler");
        return;
    }

    listeners.push_back({pin, callback, nullptr});
}

bool IR::rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data) {
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void IR::ir_task(void* arg) {
    rmt_rx_done_event_data_t rx_data;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &rx_data, portMAX_DELAY)) {
            for (const auto& listener : listeners) {
                listener.callback(rx_data);
            }
        }
    }
}