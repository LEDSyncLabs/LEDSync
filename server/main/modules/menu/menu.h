#pragma once

#include <string>
#include <vector>
#include <functional>

class Menu {
public:
   using Callback = std::function<void()>;

   Menu();
   static void drawDeviceInfoWindow();
   static void drawWifiInfoWindow();
private:
   int screenIndex = 0;   

   void changeScreen(int direction);

   std::vector<Callback> listeners = {
      drawDeviceInfoWindow,
      drawWifiInfoWindow,
   };
};