#include "menu/menu.h"
#include "lcd/lcd.h"
#include <iostream>
#include "persistent_storage/persistent_storage.h" 
#include "wifi/wifi_manager.h"
#include "input/input.hpp"
#include "menu.h"


#define SCREEN_PIN_LEFT GPIO_NUM_21
#define SCREEN_PIN_RIGHT GPIO_NUM_22


Menu::Menu() {
  drawWifiInfoWindow();  

 
   
  Input::encoder.addListener(SCREEN_PIN_LEFT, SCREEN_PIN_RIGHT, std::bind(&Menu::changeScreen, this, std::placeholders::_1));  

  Input::start();

}

void Menu::drawDeviceInfoWindow() {
   PersistentStorage storage("MQTT");

   std::string deviceName = storage.getValue("deviceName");
   std::string secretKey = storage.getValue("secretKey");

   if(deviceName == "" || secretKey == "") {
      lcd_display_clear(LCD_COLOR_BLACK);
      lcd_draw_text(6, 6, "Connect to internet", LCD_COLOR_WHITE);
      lcd_draw_text(6, 26, "to get device info", LCD_COLOR_WHITE);

      lcd_display_update();

      return;
   }

   lcd_display_clear(LCD_COLOR_BLACK);
   lcd_draw_text(6, 6, "Device Info", LCD_COLOR_WHITE);
   lcd_draw_text(6, 26, const_cast<char*>(("Device name: " + deviceName).c_str()), LCD_COLOR_WHITE);
   lcd_draw_text(6, 46, const_cast<char*>(("Secret key: " + secretKey).c_str()), LCD_COLOR_WHITE);

   lcd_display_update();
}

void Menu::drawWifiInfoWindow() {
   std::string ssid;
   WifiManager::AP::load_ssid(ssid);
  
   lcd_display_clear(LCD_COLOR_BLACK);
   lcd_draw_text(6, 6, "WiFi Info", LCD_COLOR_WHITE);
   lcd_draw_text(6, 26, const_cast<char*>(("SSID: " + ssid).c_str()), LCD_COLOR_WHITE);

   lcd_display_update();
}

void Menu::changeScreen(int direction) {
   screenIndex += direction;

   listeners[screenIndex % listeners.size()]();

}
