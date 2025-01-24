#pragma once

#include "driver/gpio.h"
#include <functional>
#include <string>
#include <vector>

class Menu {
public:
  using Callback = std::function<void()>;

  Menu(gpio_num_t left, gpio_num_t right);
  static void drawDeviceInfoWindow();
  void changeScreen(int direction);
  static void drawWifiInfoWindow();
  static void updateScreenTaskWrapper(void *param);
  static bool draw;

private:
  int screenIndex = 0;
  int counter = 0;
  bool draw2;

  void updateScreenTask(void);

  std::vector<Callback> listeners = {
      drawWifiInfoWindow,
      drawDeviceInfoWindow,
  };
};