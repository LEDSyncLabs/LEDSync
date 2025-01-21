#pragma once

#include "button.hpp"
#include "encoder.hpp"
#include "potentiometer.hpp"

class Input {
public:
    static Button button;
    static Encoder encoder;
    static Potentiometer potentiometer;

    static void start();
private:
    static const int ESP_INTR_FLAG_DEFAULT = 0;
    static bool started;
};