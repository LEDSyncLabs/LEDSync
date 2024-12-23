#include "mqtt_manager.h"
#include <esp_log.h>
#include <functional>
#include "mqtt_client.h"

#define TAG "MQTT"

MQTTManager::MQTTManager() = default;

MQTTManager::MQTTManager(const std::string &broker_uri,
                         const std::string &username,
                         const std::string &password) {
  mqtt_cfg.broker.address.uri = strdup(broker_uri.c_str());
  mqtt_cfg.credentials.username = strdup(username.c_str());
  mqtt_cfg.credentials.authentication.password = strdup(password.c_str());
}

MQTTManager::MQTTManager(const std::string &broker_uri) {
  mqtt_cfg = {};
  mqtt_cfg.broker.address.uri = strdup(broker_uri.c_str());
}

void MQTTManager::start_connecting() {
  if (client != nullptr) {
    ESP_LOGI(TAG, "Already connected");
    return;
  }
  printf("Connecting to %s\n", mqtt_cfg.broker.address.uri);
  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_start(client);
  esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
                                 (esp_event_handler_t)event_handler, this);
}

bool MQTTManager::is_connected() const {
  return connected;
}

void MQTTManager::disconnect() {
  if (client == nullptr) {
    ESP_LOGI(TAG, "Already disconnected");
    return;
  }
  esp_mqtt_client_stop(client);
  esp_mqtt_client_destroy(client);
  client = nullptr;
}

void MQTTManager::subscribe(const std::string &topic) {
  if (client == nullptr) {
    ESP_LOGE(TAG, "Not connected");
    return;
  }
  int result = esp_mqtt_client_subscribe(client, topic.c_str(), 0);

  if (result == -1) {
    ESP_LOGE(TAG, "Failed to subscribe to %s", topic.c_str());
  } else {
    ESP_LOGI(TAG, "Subscribed to %s, with message: %d", topic.c_str(), result);
  }
}

void MQTTManager::unsubscribe(const std::string &topic) {
  if (client == nullptr) {
    ESP_LOGE(TAG, "Not connected");
    return;
  }
  esp_mqtt_client_unsubscribe(client, topic.c_str());
}

void MQTTManager::publish(const std::string &topic,
                          const std::string &message) {
  if (client == nullptr) {
    ESP_LOGE(TAG, "Not connected");
    return;
  }
  esp_mqtt_client_publish(client, topic.c_str(), message.c_str(), 0, 0, 0);
}

void MQTTManager::set_message_callback(const MessageCallback &callback) {
  message_callback = callback;
//   message_callback("test", "test");
}

void MQTTManager::event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id,
                                esp_mqtt_event_handle_t event_data) {

  auto *manager = static_cast<MQTTManager *>(handler_args);
  esp_mqtt_event_handle_t event = event_data;

  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    manager->connected = true;
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      manager->connected = false;
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA received");
    ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
    ESP_LOGI(TAG, "Data: %.*s", event->data_len, event->data);
    if (manager->message_callback) {
      ESP_LOGI(TAG, "MQTT message callback set");
      std::string topic(event->topic, event->topic_len);
      std::string data(event->data, event->data_len);
      ESP_LOGI(TAG, "MQTT message data setted");
      printf("Received message: %s on topic: %s\n", data.c_str(), topic.c_str());
      manager->message_callback(topic, data);
    } else {
      ESP_LOGE(TAG, "No message callback set");
    }
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}
