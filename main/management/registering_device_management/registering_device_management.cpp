#include "registering_device_management.h"
#include "cJSON.h"
#include "persistent_storage.h"
#include "mqtt_manager.h"
#include "utils.h" 
#include <iostream>
#include "esp_timer.h"

void RegisteringDeviceManagement::setClaim() {
   PersistentStorage storage("MQTT");

   std::string secretKey = Utils::random_string(10);
   std::string accessToken = storage.getValue("accessToken");

   MQTTManager mqtt_manager(MQTTManager::url, accessToken);
   mqtt_manager.connect();

   std::string message = R"rawliteral(
      {
         "secretKey":"{1}", 
         "durationMs":{2}
         }
      )rawliteral";

   Utils::replace(message, "{1}", secretKey);
   Utils::replace(message, "{2}", std::to_string(expireTime));

   mqtt_manager.publish("v1/devices/me/claim", message);
   printf("Claim message sent\n");
   printf("Secret key: %s\n", secretKey.c_str());

   storage.saveValue("secretKey", secretKey);
   
}

void RegisteringDeviceManagement::checkIfDeviceIsRegistered() {
  PersistentStorage storage("MQTT");
  std::string deviceName = storage.getValue("deviceName");
  std::string secretKey = storage.getValue("secretKey");
  std::string accessToken = storage.getValue("accessToken");

  printf("Device name: %s\n", deviceName.c_str());
  printf("Secret key: %s\n", secretKey.c_str());
  printf("Access token: %s\n", accessToken.c_str());

  if (deviceName == "" || accessToken == "") {
    MQTTManager *mqtt_manager = new MQTTManager(MQTTManager::url, "provision");
    mqtt_manager->connect();

    deviceName = Utils::random_string(10);
    storage.saveValue("deviceName", deviceName);

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
            PersistentStorage storage("MQTT");
            storage.saveValue("accessToken", token);            

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
                   storage.getValue("deviceName").c_str());

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

}
