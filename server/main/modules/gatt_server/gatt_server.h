#pragma once

#include "types/ble_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void start_gatt_server(void);
void gatts_indicate_color(led_color_t color);
void gatts_indicate_brightness(uint8_t brightness);
void gatts_start_advert(int64_t time);

#ifdef __cplusplus
}
#endif