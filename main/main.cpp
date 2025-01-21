#include "input.hpp"

void button_pressed(int value) {
    ESP_LOGI("Main", "Button pressed, value: %d", value);
}

void encoder_rotated(int direction) {
    ESP_LOGI("Main", "Encoder rotated, direction: %d", direction);
}

extern "C" void app_main(void) {
    Input::button.addListener(GPIO_NUM_18, button_pressed);
    Input::button.addListener(GPIO_NUM_19, button_pressed);
    Input::button.addListener(GPIO_NUM_21, button_pressed);

    // Input::encoder.addListener(GPIO_NUM_21, button_pressed);
    Input::encoder.addRotationListener(GPIO_NUM_22, GPIO_NUM_23, encoder_rotated);

    Input::button.start();
    Input::encoder.start();

    // while(1){
    //     int pin_value = gpio_get_level(GPIO_NUM_22);
    //     ESP_LOGI("Main", "Pin [22] value: %d", pin_value);
    //     vTaskDelay(100 / portTICK_PERIOD_MS);
    // }
}