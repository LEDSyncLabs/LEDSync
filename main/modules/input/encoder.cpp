#include "encoder.hpp"

QueueHandle_t Encoder::gpio_evt_queue = NULL;
std::unordered_map<gpio_num_t, Encoder::Callback> Encoder::callbacks;
std::unordered_map<std::pair<gpio_num_t, gpio_num_t>, Encoder::Callback, pair_hash> Encoder::rotation_callbacks;
volatile int64_t Encoder::last_interrupt_time = 0;

const int Encoder::ESP_INTR_FLAG_DEFAULT;
const int64_t Encoder::DEBOUNCE_THRESHOLD_US;

Encoder::Encoder() {}

Encoder::~Encoder() {
    gpio_uninstall_isr_service();
    vQueueDelete(gpio_evt_queue);
}

void Encoder::start() {
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(encoder_task, "encoder_task", 2048, NULL, 10, NULL);
    // gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    for (const auto& pair : callbacks) {
        gpio_isr_handler_add(pair.first, gpio_isr_handler, (void*)pair.first);
    }

    for (const auto& pair : rotation_callbacks) {
        gpio_isr_handler_add(pair.first.first, gpio_isr_handler, (void*)pair.first.first);
        gpio_isr_handler_add(pair.first.second, gpio_isr_handler, (void*)pair.first.second);
    }
}

void Encoder::addListener(gpio_num_t pin, Callback callback) {
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

void Encoder::addRotationListener(gpio_num_t pinA, gpio_num_t pinB, Callback callback) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pinA) | (1ULL << pinB),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io_conf);

    rotation_callbacks[{pinA, pinB}] = callback;
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
            // Handle button press
            if (callbacks.find(io_num) != callbacks.end()) {
                int pin_value = gpio_get_level(io_num);
                ESP_LOGI("Encoder", "Button pressed at GPIO[%d], value: %d", io_num, pin_value);
                callbacks[io_num](pin_value);
            }

            // Handle rotation
            for (const auto& pair : rotation_callbacks) {
                gpio_num_t pinA = pair.first.first;
                gpio_num_t pinB = pair.first.second;

                if (io_num == pinA || io_num == pinB) {
                    int stateA = gpio_get_level(pinA);
                    int stateB = gpio_get_level(pinB);
                    int direction = 0;

                    if (stateA == stateB) {
                        direction = (io_num == pinA) ? -1 : 1; // Counter-clockwise if pinA changes, Clockwise if pinB changes
                    } else {
                        direction = (io_num == pinA) ? 1 : -1; // Clockwise if pinA changes, Counter-clockwise if pinB changes
                    }

                    ESP_LOGI("Encoder", "Pins [%d, %d] rotated, direction: %d", pinA, pinB, direction);
                    pair.second(direction);
                }
            }
        }
    }
}