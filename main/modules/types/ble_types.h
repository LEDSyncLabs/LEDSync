#ifndef BLE_TYPE_H
#define BLE_TYPE_H

#include <cstdint>

struct RGB_LED {
   uint8_t red;
   uint8_t green;
   uint8_t blue;
};

/**
 * @struct brightness
 * @brief Represents the brightness level.
 * 
 * Value is between 0 and 1.
 */
struct brightness {
   float value; 
};


#endif // BLE_TYPE_H