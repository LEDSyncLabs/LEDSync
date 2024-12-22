#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <string>

#define WIFI_SETUP_AP_SSID "LEDSync Setup"

#define WIFI_AP_SSID_KEY "ap_ssid"
#define WIFI_STA_SSID_KEY "sta_ssid"
#define WIFI_STA_PASSWORD_KEY "sta_password"

class WifiManager {
private:
public:
  class STA {
  public:
    static bool load_credentials(std::string &ssid, std::string &password);
    static void save_credentials(const std::string &ssid,
                                 const std::string &password);
    static bool start();
    static void stop();
  };
  class AP {
  public:
    static bool load_ssid(std::string &ssid);
    static void save_ssid(const std::string &ssid);
    static void start();
    static void stop();
  };
};

#endif // WIFI_MANAGER_H