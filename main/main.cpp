#include "input.hpp"

void ir_handler_any(uint16_t command) {
    ESP_LOGI("IR", "Received on listen all:    %04X", command);
}

void ir_handler_specific(uint16_t command) {
    ESP_LOGI("IR", "Received specific command: %04X", command);
}

extern "C" void app_main(void) {
    Input::ir.addListener(ir_handler_any);
    Input::ir.addListener(0xFF00, ir_handler_specific);
    Input::ir.addListener(0x807F, ir_handler_specific);
    Input::start();
}