#include "menu/menu.h"
#include "input/input.hpp"
#include "lcd/lcd.h"
#include "menu.h"
#include "persistent_storage/persistent_storage.h"
#include "wifi/wifi_manager.h"
#include <iostream>

bool Menu::draw = true;

Menu::Menu(gpio_num_t left, gpio_num_t right) {
  draw2 = true;

  lcd_init();

  xTaskCreate(updateScreenTaskWrapper,
              "Update Screen Task", // Task name
              4096,                 // Stack size
              this,                 // Task parameters
              1,                    // Task priority
              nullptr               // Task handle
  );

  Input::encoder.addListener(
      left, right, std::bind(&Menu::changeScreen, this, std::placeholders::_1));

  Input::ir.addListener(0xF807,
                        [this](uint16_t command) { this->changeScreen(-1); });

  Input::ir.addListener(0xF609,
                        [this](uint16_t command) { this->changeScreen(1); });
}

void Menu::drawDeviceInfoWindow() {
  PersistentStorage storage("MQTT");

  std::string deviceName = storage.getValue("deviceName");
  std::string secretKey = storage.getValue("secretKey");

  if (deviceName == "" || secretKey == "") {
    lcd_display_clear(LCD_COLOR_BLACK);
    lcd_draw_text(6, 6, "Connect to internet", LCD_COLOR_WHITE);
    lcd_draw_text(6, 26, "to get device info", LCD_COLOR_WHITE);

    lcd_display_update();

    return;
  }

  lcd_display_clear(LCD_COLOR_BLACK);
  lcd_draw_text(6, 6, "Device Info", LCD_COLOR_WHITE);
  lcd_draw_text(6, 26, const_cast<char *>(("Name: " + deviceName).c_str()),
                LCD_COLOR_WHITE);
  lcd_draw_text(6, 46, const_cast<char *>(("Key: " + secretKey).c_str()),
                LCD_COLOR_WHITE);

  lcd_display_update();
}

void Menu::drawWifiInfoWindow() {
  std::string ssid;
  WifiManager::AP::load_ssid(ssid);

  lcd_display_clear(LCD_COLOR_BLACK);

  if (WifiManager::AP::is_started()) {
    lcd_draw_text(6, 6, "AP Info", LCD_COLOR_WHITE);
    lcd_draw_text(6, 26, const_cast<char *>(("SSID: " + ssid).c_str()),
                  LCD_COLOR_WHITE);
    lcd_draw_text(6, 56, "IP: 192.168.4.1", LCD_COLOR_WHITE);
  } else if (WifiManager::STA::is_started()) {
    std::string ssid, pass;
    WifiManager::STA::load_credentials(ssid, pass);

    lcd_draw_text(6, 6, "Wifi Info", LCD_COLOR_WHITE);
    lcd_draw_text(6, 26, const_cast<char *>(("SSID: " + ssid).c_str()),
                  LCD_COLOR_WHITE);
    if (WifiManager::STA::is_connected()) {
      lcd_draw_text(6, 56, "Connected", LCD_COLOR_BLUE);
    } else {
      lcd_draw_text(6, 56, "Connecting", LCD_COLOR_CYAN);
    }
  } else {
    lcd_draw_text(6, 6, "Initializing...", LCD_COLOR_CYAN);
  }

  lcd_display_update();
}

void Menu::changeScreen(int direction) {
  ESP_LOGI("menu", "change");
  screenIndex += direction;

  if (counter < 80) {
    counter = 0;
  } else {
    counter = 20;
  }
}

void Menu::updateScreenTask() {
  while (1) {
    if (--counter <= 0) {
      if (draw) {
        draw2 = true;
        listeners[screenIndex % listeners.size()]();
      } else {
        if (draw2 == true) {
          draw2 = false;
          lcd_display_clear(LCD_COLOR_BLACK);
          lcd_display_update();
        }
      }
      counter = 100;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void Menu::updateScreenTaskWrapper(void *param) {
  Menu *instance = static_cast<Menu *>(param);
  instance->updateScreenTask();
}
