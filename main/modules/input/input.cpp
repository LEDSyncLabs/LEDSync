#include "input.hpp"

Button Input::button;
Encoder Input::encoder;
Potentiometer Input::potentiometer;
// IR Input::ir;

bool Input::started = false;

void Input::start() {
    if (started) {
        ESP_LOGW("Input", "Input handler already started");        
        return;
    }

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    started = true;

    button.start();
    encoder.start();
    potentiometer.start();
    // ir.start();
}