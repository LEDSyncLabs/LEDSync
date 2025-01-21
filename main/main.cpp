#include "input_handler.hpp"

void button_pressed(int value) {
    ESP_LOGI("Main", "Button pressed at GPIO_NUM_18, value: %d", value);
}

extern "C" void app_main(void) {
    Input Input;
    Input.addListener(GPIO_NUM_18, button_pressed);
    Input.addListener(GPIO_NUM_19, button_pressed);
    Input.addListener(GPIO_NUM_21, button_pressed);
    Input.start();
}
