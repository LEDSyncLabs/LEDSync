#include "input_handler.hpp"

QueueHandle_t Input::gpio_evt_queue = NULL;
std::unordered_map<int, Input::Callback> Input::callbacks;
volatile int64_t Input::last_interrupt_time = 0;

const int Input::ESP_INTR_FLAG_DEFAULT;
const int64_t Input::DEBOUNCE_THRESHOLD_US;

Input::Input() {}

Input::~Input() {}

void Input::start() {
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    
    for (const auto& pair : callbacks) {
        gpio_isr_handler_add(static_cast<gpio_num_t>(pair.first), gpio_isr_handler, (void*) pair.first);
    }
}

void Input::addListener(gpio_num_t pin, Callback callback) {
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

void IRAM_ATTR Input::gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    int64_t now = esp_timer_get_time();

    if (now - last_interrupt_time > DEBOUNCE_THRESHOLD_US) {
        last_interrupt_time = now;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
}

void Input::button_task(void* arg) {
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            int pin_value = gpio_get_level((gpio_num_t)io_num);
            // ESP_LOGI("Input", "Pin [%lu] value: %d", io_num, pin_value);
            if (callbacks.find(io_num) != callbacks.end()) {
                callbacks[io_num](pin_value);
            }
        }
    }
}