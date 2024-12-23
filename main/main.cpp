#include "modules/gatt_client/gatt_client.h"


gpio_num_t LED_A = GPIO_NUM_16;
gpio_num_t LED_B = GPIO_NUM_5;

void handle_notification(uint16_t handle, uint8_t value) {
    gpio_num_t pin = GPIO_NUM_NC;

    if (handle == 0xDE01) {
        pin = LED_A;
    } else {
        pin = LED_B;
    }

    if (pin == GPIO_NUM_NC) {
        ESP_LOGI(GATTC_TAG, "Unknown handle in notify event %x", handle);
        return;
    }

    gpio_set_level(pin, value);
}

void init_leds() {
    gpio_config_t io_conf_a = {
        .pin_bit_mask = (1ULL << GPIO_NUM_16),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf_a));
    gpio_set_level(LED_A, 0);

    gpio_config_t io_conf_b = {.pin_bit_mask = (1ULL << LED_B), .mode = GPIO_MODE_OUTPUT};
    ESP_ERROR_CHECK(gpio_config(&io_conf_b));
    gpio_set_level(LED_B, 0);
}


extern "C" void app_main(void) {
    std::map<uint16_t, std::vector<uint16_t>> service_data = {
        {0xDEAD, {0xDE01, 0xDE02}},
        {0xBEEF, {0xBE01, 0xBE02}},
    };

    GattClient::on_notify = handle_notification;

    GattClient::create(service_data);

    init_leds();
}
