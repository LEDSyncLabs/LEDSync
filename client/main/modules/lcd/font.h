#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_BOUND_LOWER 32
#define FONT_BOUND_UPPER 127
#define FONT_CHAR_WIDTH 5
#define FONT_CHAR_HEIGHT 8

// Deklaracja tablicy (extern), nie definicja
extern const uint8_t font_default[96][5];

#endif // FONT_H



