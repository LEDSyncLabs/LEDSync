#include "encoder.hpp"

QueueHandle_t Encoder::gpio_evt_queue = NULL;
std::unordered_map<std::pair<gpio_num_t, gpio_num_t>, Encoder::Callback, pair_hash> Encoder::callbacks;
volatile int64_t Encoder::last_interrupt_time = 0;

const int64_t Encoder::DEBOUNCE_THRESHOLD_US;

Encoder::Encoder() {}

Encoder::~Encoder() {
    vQueueDelete(gpio_evt_queue);
}

void Encoder::start() {
    if (gpio_evt_queue != NULL) {
        ESP_LOGW("Encoder", "Encoder handler already started");        
        return;
    }

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(encoder_task, "encoder_task", 2048, NULL, 10, NULL);

    for (const auto& pair : callbacks) {
        gpio_isr_handler_add(pair.first.first, gpio_isr_handler, (void*)pair.first.first);
        gpio_isr_handler_add(pair.first.second, gpio_isr_handler, (void*)pair.first.second);
    }
}

void Encoder::addListener(gpio_num_t pinA, gpio_num_t pinB, Callback callback) {
    if (gpio_evt_queue != NULL) {
        ESP_LOGW("Button", "Cannot add listener after starting the handler");        
        return;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pinA) | (1ULL << pinB),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);

    callbacks[{pinA, pinB}] = callback;
}

void IRAM_ATTR Encoder::gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    int64_t now = esp_timer_get_time();

    if (now - last_interrupt_time > DEBOUNCE_THRESHOLD_US) {
        last_interrupt_time = now;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
}

void Encoder::encoder_task(void* arg) {
    gpio_num_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            for (const auto& pair : callbacks) {
                gpio_num_t pinA = pair.first.first;
                gpio_num_t pinB = pair.first.second;

                if (io_num == pinA || io_num == pinB) {
                    int stateA = gpio_get_level(pinA);
                    int stateB = gpio_get_level(pinB);
                    int direction = 0;

                    if (stateA == stateB) {
                        direction = (io_num == pinA) ? 1 : -1; // Clockwise if pinA changes, Counter-clockwise if pinB changes
                    } else {
                        direction = (io_num == pinA) ? -1 : 1; // Counter-clockwise if pinA changes, Clockwise if pinB changes
                    }

                    // ESP_LOGI("Encoder", "Pins [%d, %d] rotated, direction: %d", pinA, pinB, direction);
                    pair.second(direction);
                }
            }
        }
    }
}