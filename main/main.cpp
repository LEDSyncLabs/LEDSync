#include "input.hpp"

void ir_handler(uint16_t command) {
    ESP_LOGI("Main", "IR command received: %04X", command);
}

extern "C" void app_main(void) {
    Input::ir.addListener(0xFF00, ir_handler);
    Input::start();
}