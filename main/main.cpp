#include "cJSON.h"
#include "gatt_client.h" // TODO remove but include here dependencies for nvs_... and ESP_...
#include "gatt_server.h"
#include "http_server.h"
#include "mqtt_manager.h"
#include "persistent_storage.h"
#include "utils.h"
#include "wifi_manager.h"
#include <esp_timer.h>
#include <iostream>


#define expireTime 7 * 24 * 60 * 60 * 1000 // 7 days

void setClaim() {
  PersistentStorage *storage = new PersistentStorage("MQTT");

  std::string secretKey = Utils::random_string(10);
  std::string accessToken = storage->getValue("accessToken");

  MQTTManager *mqtt_manager = new MQTTManager(MQTTManager::url, accessToken);
  mqtt_manager->connect();

  std::string message = R"rawliteral(
      {
        "secretKey":"{1}", 
        "durationMs":{2}
        }
    )rawliteral";

  Utils::replace(message, "{1}", secretKey);
  Utils::replace(message, "{2}", std::to_string(expireTime));

  mqtt_manager->publish("v1/devices/me/claim", message);
  printf("Claim message sent\n");
  printf("Secret key: %s\n", secretKey.c_str());

  storage->saveValue("secretKey", secretKey);
  delete storage;
  delete mqtt_manager;
}

void checkIfDeviceIsRegistered() {
  PersistentStorage *storage = new PersistentStorage("MQTT");
  std::string deviceName = storage->getValue("deviceName");
  std::string secretKey = storage->getValue("secretKey");
  std::string accessToken = storage->getValue("accessToken");

  printf("Device name: %s\n", deviceName.c_str());
  printf("Secret key: %s\n", secretKey.c_str());
  printf("Access token: %s\n", accessToken.c_str());

  if (deviceName == "" || accessToken == "") {
    MQTTManager *mqtt_manager = new MQTTManager(MQTTManager::url, "provision");
    mqtt_manager->connect();

    deviceName = Utils::random_string(10);
    storage->saveValue("deviceName", deviceName);

    printf("Device name: %s\n", deviceName.c_str());
    printf("Secret key: %s\n", secretKey.c_str());
    printf("Access token: %s\n", accessToken.c_str());

    mqtt_manager->subscribe(
        "/provision/response", [mqtt_manager](const std::string &message) {
          std::cout << "Received provision response: " << message << std::endl;

          // Parsowanie JSON za pomocą cJSON
          cJSON *root = cJSON_Parse(message.c_str());
          if (root == nullptr) {
            std::cerr << "Failed to parse JSON" << std::endl;
            return;
          }

          // Pobranie pola "credentialsValue"
          cJSON *credentialsValue =
              cJSON_GetObjectItem(root, "credentialsValue");
          if (credentialsValue != nullptr && cJSON_IsString(credentialsValue)) {
            std::string token = credentialsValue->valuestring;
            std::cout << "Extracted credentials value: " << token << std::endl;

            // Możesz tutaj zapisać token do pamięci trwałej np. NVS
            PersistentStorage *storage = new PersistentStorage("MQTT");
            storage->saveValue("accessToken", token);
            delete storage;

            mqtt_manager->unsubscribe("/provision/response");

            esp_timer_handle_t shutdown_timer;
            esp_timer_create_args_t timer_args = {
                .callback =
                    [](void *arg) {
                      MQTTManager *mqtt = static_cast<MQTTManager *>(arg);
                      delete mqtt;
                      printf("MQTT client deleted safely\n");
                    },
                .arg = mqtt_manager,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "mqtt_shutdown"};
            esp_timer_create(&timer_args, &shutdown_timer);
            esp_timer_start_once(shutdown_timer, 500000); // 500ms delay

            printf("Device name: %s\n",
                   storage->getValue("deviceName").c_str());

            setClaim();
          } else {
            std::cerr << "Error: credentialsValue not found or not a string"
                      << std::endl;
          }

          cJSON_Delete(root);
        });

    std::string message = R"rawliteral(
      {
        'deviceName':'{1}',
        'provisionDeviceKey':'hhnivi92xm3c8yek45ct',
        'provisionDeviceSecret':'an2u0xl5xa95i5do86hb'
      }
    )rawliteral";

    Utils::replace(message, "{1}", deviceName);

    mqtt_manager->publish("/provision/request", message);
  } else if (secretKey == "") {
    setClaim();
  }

  delete storage;
}

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
    std::cout << "NVS got erased" << std::endl;
  }

  WifiManager::STA::save_credentials("ESP32", "zaq1@WSX");
  WifiManager::STA::start();

  checkIfDeviceIsRegistered();

  PersistentStorage *storage = new PersistentStorage("MQTT");
  while (true) {
    vTaskDelay(4000 / portTICK_PERIOD_MS);

    std::string deviceName = storage->getValue("deviceName");
    std::string secretKey = storage->getValue("secretKey");
    std::string accessToken = storage->getValue("accessToken");

    if(deviceName != "" && secretKey != "" && accessToken != "") {
      break;
    }
  }

  WifiManager::STA::stop();
  WifiManager::AP::save_ssid("LEDSync Setup");
  WifiManager::AP::start();

  HttpServer *http_server = new HttpServer();
}
