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

extern "C" void app_main(void) {
    Input::button.addListener(GPIO_NUM_18, button_pressed);
    Input::button.addListener(GPIO_NUM_19, button_pressed);
    Input::button.addListener(GPIO_NUM_21, button_pressed);

    Input::encoder.addListener(GPIO_NUM_22, GPIO_NUM_23, encoder_rotated);

    Input::potentiometer.addListener(ADC2_CHANNEL_0, potentiometer_changed, 500);

    int value = Input::potentiometer.read(ADC2_CHANNEL_0);
    ESP_LOGI("Main", "Potentiometer value: %d", value);

    Input::start();
}