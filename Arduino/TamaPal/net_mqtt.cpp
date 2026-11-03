// WHAT: Implementation of Wi-Fi and MQTT connectivity using official Arduino libraries
// WHY:  Asynchronous, non-blocking network I/O for 20 Hz controller inputs and incoming events
// USED BY: TamaPal.ino

#include "net_mqtt.h"

NetMqttClient::NetMqttClient()
  : mqttClient(wifiClient),
    onEventCallback(nullptr),
    lastPublishTime(0),
    lastReconnectAttempt(0),
    connectingState(false) {
}

void NetMqttClient::begin(GameEventCallback callback) {
  onEventCallback = callback;

  // Set MQTT client credentials and keep-alive
  mqttClient.setId(MQTT_CLIENT_ID);
  mqttClient.setKeepAliveInterval(15 * 1000); // 15 seconds keep-alive

  #if defined(SECRET_MQTT_USER) && defined(SECRET_MQTT_PASS)
  if (strlen(SECRET_MQTT_USER) > 0) {
    mqttClient.setUsernamePassword(SECRET_MQTT_USER, SECRET_MQTT_PASS);
  }
  #endif
}

bool NetMqttClient::isConnected() {
  return (WiFi.status() == WL_CONNECTED) && mqttClient.connected();
}

bool NetMqttClient::isConnecting() const {
  return connectingState;
}

void NetMqttClient::disconnect() {
  if (mqttClient.connected()) {
    mqttClient.stop();
  }
  connectingState = false;
}

void NetMqttClient::attemptConnection(unsigned long now) {
  if (isConnected()) {
    connectingState = false;
    return;
  }

  // Rate-limit connection attempts to avoid freezing loop()
  if (now - lastReconnectAttempt < MQTT_RECONNECT_INTERVAL_MS) {
    return;
  }
  lastReconnectAttempt = now;
  connectingState = true;

  // 1. Check / establish Wi-Fi
  uint8_t wifiStatus = WiFi.status();
  if (wifiStatus != WL_CONNECTED) {
    // Only trigger begin() if not already attempting handshake
    if (wifiStatus != WL_IDLE_STATUS) {
      #if defined(SECRET_WIFI_SSID) && defined(SECRET_WIFI_PASS)
      if (strlen(SECRET_WIFI_SSID) > 0) {
        WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
      }
      #endif
    }
    return; // Wait for next tick to verify Wi-Fi connection
  }

  // 2. Connect to MQTT broker
  if (!mqttClient.connected()) {
    #if defined(SECRET_MQTT_HOST) && defined(SECRET_MQTT_PORT)
    if (mqttClient.connect(SECRET_MQTT_HOST, SECRET_MQTT_PORT)) {
      // Subscribe to backend event topic (tamapal/opla-1/event) with QoS 1
      mqttClient.subscribe(MQTT_TOPIC_EVENT, 1);
      connectingState = false;
    }
    #endif
  }
}

void NetMqttClient::pollIncomingEvents() {
  int messageSize = mqttClient.parseMessage();
  if (messageSize <= 0) return;

  // Discard packet if unusually large to protect memory
  if (messageSize > 128) {
    while (mqttClient.available()) mqttClient.read();
    return;
  }

  char buffer[129];
  int bytesRead = 0;
  while (mqttClient.available() && bytesRead < 128) {
    buffer[bytesRead++] = (char)mqttClient.read();
  }
  buffer[bytesRead] = '\0';

  // Parse JSON using battle-tested ArduinoJson v7
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buffer);
  if (error) return;

  const char* typeStr = doc["type"];
  if (typeStr != nullptr && onEventCallback != nullptr) {
    GameEvent event = TiltLogic::parseEventType(typeStr);
    if (event != EVENT_NONE) {
      onEventCallback(event);
    }
  }
}

void NetMqttClient::update(unsigned long now) {
  // Asynchronously check and maintain connection
  attemptConnection(now);

  // Poll client for keep-alives and incoming packets
  if (mqttClient.connected()) {
    mqttClient.poll();
    pollIncomingEvents();
  }
}

bool NetMqttClient::publishInput(const char* jsonPayload, unsigned long now) {
  if (jsonPayload == nullptr || !isConnected()) {
    return false;
  }

  // Rate-limit publishing to ~20 Hz (every 50 ms)
  if (now - lastPublishTime < MQTT_PUBLISH_INTERVAL_MS) {
    return false;
  }

  lastPublishTime = now;

  // Publish to tamapal/opla-1/input with QoS 0 (standard fast sensor stream)
  mqttClient.beginMessage(MQTT_TOPIC_INPUT, false, 0, false);
  mqttClient.print(jsonPayload);
  mqttClient.endMessage();

  return true;
}

