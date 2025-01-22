#include "input.hpp"

void button_pressed(int value) {
    ESP_LOGI("Main", "Button pressed, value: %d", value);
}

int counter = 0;
void encoder_rotated(int direction) {
    counter += direction;
    ESP_LOGI("Main", "Encoder rotated, direction: %d, counter: %d", direction, counter);
}

void potentiometer_changed(int value) {
    ESP_LOGI("Main", "Potentiometer value: %d", value);
}

void ir_handler(uint16_t command) {
    ESP_LOGI("Main", "IR received, command: %04X", command);
}

extern "C" void app_main(void) {
    // On button change (can be encoder button)
    Input::button.addListener(GPIO_NUM_18, button_pressed);
    Input::button.addListener(GPIO_NUM_21, button_pressed);

    // On encoder rotation
    Input::encoder.addListener(GPIO_NUM_22, GPIO_NUM_23, encoder_rotated);

    // Read potentiometer value every interval
    Input::potentiometer.addListener(ADC2_CHANNEL_0, potentiometer_changed, 500);

    // Read potentiometer value once
    int value = Input::potentiometer.read(ADC2_CHANNEL_0);
    ESP_LOGI("Main", "Potentiometer value: %d", value);

    // On any IR commands
    Input::ir.addListener(ir_handler);
    // On specific IR commands
    Input::ir.addListener(0xFF00, ir_handler);

    // Required to start all input handlers, must be called after adding listeners
    Input::start();
}