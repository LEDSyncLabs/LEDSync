#include "mqtt_manager.h"
#include <esp_log.h>
#include <iostream>

#define TAG "MQTT"

const std::string MQTTManager::url = "mqtt://ledsync.spookyless.net";

MQTTManager::MQTTManager() = default;

MQTTManager::MQTTManager(const std::string &broker_uri,
                         const std::string &username) {
  mqtt_cfg.broker.address.uri = strdup(broker_uri.c_str());
  mqtt_cfg.credentials.username = strdup(username.c_str());
}

MQTTManager::MQTTManager(const std::string &broker_uri) {
  mqtt_cfg.broker.address.uri = strdup(broker_uri.c_str());
}

MQTTManager::~MQTTManager() { 
  printf("Destructor");
  for (const auto &topic : topic_callbacks) {
    esp_mqtt_client_unsubscribe(client, topic.first.c_str());
  }
  printf("Destructor 2");
  topic_callbacks.clear();
  disconnect(); 
  }

void MQTTManager::connect() {
  start_connecting();
  while (!is_connected()) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void MQTTManager::start_connecting() {
  if (client != nullptr) {
    ESP_LOGI(TAG, "Already connected");
    return;
  }
  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_start(client);
  esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
                                 (esp_event_handler_t)event_handler, this);
}

bool MQTTManager::is_connected() const { return connected; }

void MQTTManager::disconnect() {
  if (client == nullptr) {
    ESP_LOGI(TAG, "Already disconnected");
    return;
  }
  // esp_mqtt_client_disconnect(client);
  printf("Destructor 3 - after disconnect");

  // while (is_connected()) {
  //     vTaskDelay(pdMS_TO_TICKS(100)); // Wait for disconnection
  // }
  printf("Destructor 4 - after while");

  esp_mqtt_client_stop(client);
  esp_mqtt_client_destroy(client);
  printf("Destructor 5 - after destroy");
  client = nullptr;
  ESP_LOGI(TAG, "Client disconnected");
}

void MQTTManager::subscribe(const std::string &topic,
                            MessageCallback callback) {
  if (client == nullptr) {
    ESP_LOGE(TAG, "Not connected");
    return;
  }
  int result = esp_mqtt_client_subscribe(client, topic.c_str(), 0);
  if (result == -1) {
    ESP_LOGE(TAG, "Failed to subscribe to %s", topic.c_str());
  } else {
    ESP_LOGI(TAG, "Subscribed to %s", topic.c_str());
    topic_callbacks[topic] = callback;
  }
}

void MQTTManager::unsubscribe(const std::string &topic) {
  if (client == nullptr) {
    ESP_LOGE(TAG, "Not connected");
    return;
  }
  esp_mqtt_client_unsubscribe(client, topic.c_str());
  topic_callbacks.erase(topic);
}

void MQTTManager::publish(const std::string &topic,
                          const std::string &message) {
  if (!is_connected() || client == nullptr) {
    ESP_LOGE(TAG, "Not connected");
    return;
  }
  esp_mqtt_client_publish(client, topic.c_str(), message.c_str(), 0, 0, 0);
}

void MQTTManager::event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id,
                                esp_mqtt_event_handle_t event_data) {
  auto *manager = static_cast<MQTTManager *>(handler_args);
  esp_mqtt_event_handle_t event = event_data;

  std::string topic;
  std::string data;
  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    manager->connected = true;
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    manager->connected = false;
    break;
  case MQTT_EVENT_DATA:
    topic.assign(event->topic, event->topic_len);
    data.assign(event->data, event->data_len);
    ESP_LOGI(TAG, "Received message on topic %s: %s", topic.c_str(),
             data.c_str());

    // Check if there's a callback for the topic
    if (manager->topic_callbacks.find(topic) !=
        manager->topic_callbacks.end()) {
      manager->topic_callbacks[topic](data);
    } else {
      ESP_LOGW(TAG, "No callback registered for topic: %s", topic.c_str());
    }
    break;
  case MQTT_EVENT_BEFORE_CONNECT:
    ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
    break;
  default:
    ESP_LOGI(TAG, "Other event id: %d", event->event_id);
    break;
  }
}
