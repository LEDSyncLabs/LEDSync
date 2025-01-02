#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H
#include <esp_log.h>
#include <functional>
#include <mqtt_client.h>
#include <string>


class MQTTManager {
public:
  using MessageCallback =
      std::function<void(const std::string &topic, const std::string &message)>;

  MQTTManager();
  MQTTManager(const std::string &broker_uri, const std::string &username,
              const std::string &password);
  MQTTManager(const std::string &broker_uri);
  ~MQTTManager() = default;

  std::string testStr = "uwu";

  void start_connecting();
  bool is_connected() const;
  void disconnect();
  void subscribe(const std::string &topic);
  void unsubscribe(const std::string &topic);
  void publish(const std::string &topic, const std::string &message);

  void set_message_callback(const MessageCallback &callback);
  MessageCallback message_callback = nullptr;

private:
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker =
          {
              .address =
                  {
                      .uri = "mqtt://192.168.43.210",
                  },
          },
      // .broker.address.uri = "mqtt://mqtt.eclipseprojects.io",
      .credentials =
          {
              .username = "weather_user",
              .authentication =
                  {
                      .password = "zaq1@WSX",
                  },
          },
      .network =
          {
              .timeout_ms = 10000,
          },
  };
  esp_mqtt_client_handle_t client = nullptr;
  bool connected = false;

  static void
  event_handler(void *handler_args, esp_event_base_t base, int32_t event_id,
                esp_mqtt_event_handle_t event_data); // std::string broker_;
};

#endif // MQTT_MANAGER_H