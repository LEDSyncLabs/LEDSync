#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H
#include <esp_log.h>
#include <functional>
#include <mqtt_client.h>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>

class MQTTManager {
public:
  const static std::string url;
  using MessageCallback = std::function<void(const std::string &message)>;

  MQTTManager();
  MQTTManager(const std::string &broker_uri, const std::string &username);
  MQTTManager(const std::string &broker_uri);
  ~MQTTManager();

  void connect();
  void start_connecting();
  bool is_connected() const;
  void disconnect();
  
  void subscribe(const std::string &topic, MessageCallback callback);
  void unsubscribe(const std::string &topic);

  void publish(const std::string &topic, const std::string &message);

private:
  esp_mqtt_client_config_t mqtt_cfg = {};
  esp_mqtt_client_handle_t client = nullptr;
  bool connected = false;

  std::unordered_map<std::string, MessageCallback> topic_callbacks;

  static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id,
                            esp_mqtt_event_handle_t event_data);
};

#endif // MQTT_MANAGER_H
