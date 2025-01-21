#include "input.hpp"

void button_pressed(int value) {
    ESP_LOGI("Main", "Button pressed, value: %d", value);
}

extern "C" void app_main(void) {
    Input::button.addListener(GPIO_NUM_18, button_pressed);
    Input::button.addListener(GPIO_NUM_19, button_pressed);
    Input::button.addListener(GPIO_NUM_21, button_pressed);
    Input::button.start();
}