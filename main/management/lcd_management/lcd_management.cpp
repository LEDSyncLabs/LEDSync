#include "lcd_management.h"
#include "lcd.h"
#include <iostream>
#include "persistent_storage.h" 
#include "wifi_manager.h"

void LcdManagement::drawDeviceInfoWindow() {
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

void LcdManagement::drawWifiInfoWindow() {
   std::string ssid;
   WifiManager::AP::load_ssid(ssid);
  
   lcd_display_clear(LCD_COLOR_BLACK);
   lcd_draw_text(6, 6, "WiFi Info", LCD_COLOR_WHITE);
   lcd_draw_text(6, 26, const_cast<char*>(("SSID: " + ssid).c_str()), LCD_COLOR_WHITE);

   lcd_display_update();
}
