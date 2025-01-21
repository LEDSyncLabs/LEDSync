#include "input.hpp"

void button_pressed(int value) {
    ESP_LOGI("Main", "Button pressed, value: %d", value);
}

int counter = 0;
void encoder_rotated(int direction) {
    counter += direction;
    ESP_LOGI("Main", "Encoder rotated, direction: %d, counter: %d", direction, counter);
}

void potentiometer_changed(const rmt_rx_done_event_data_t& rx_data) {
    ESP_LOGI("Main", "Potentiometer value: %d", rx_data.num_symbols);
}

// void ir_received(int value) {
//     ESP_LOGI("Main", "IR signal received, value: %d", value);
// }

void ir_received(const rmt_rx_done_event_data_t& rx_data) {
    ESP_LOGI("Main", "IR signal received, num_symbols: %d", rx_data.num_symbols);
    // Process the received IR data here
}

extern "C" void app_main(void) {
    Input::button.addListener(GPIO_NUM_18, button_pressed);
    // Input::button.addListener(GPIO_NUM_19, button_pressed);
    Input::button.addListener(GPIO_NUM_21, button_pressed);

    Input::encoder.addListener(GPIO_NUM_22, GPIO_NUM_23, encoder_rotated);

    // Input::potentiometer.addListener(ADC2_CHANNEL_0, potentiometer_changed, 100);

    int value = Input::potentiometer.read(ADC2_CHANNEL_0);
    ESP_LOGI("Main", "Potentiometer value: %d", value);

    Input::ir.addListener(GPIO_NUM_19, ir_received);

    //read in loop
    // while (1) {
    //     int value = Input::potentiometer.read(ADC2_CHANNEL_0);
    //     ESP_LOGI("Main", "Potentiometer value: %d", value);
    //     vTaskDelay(pdMS_TO_TICKS(100));
    // }

    Input::start();
}