// WHAT: Wi-Fi and MQTT connectivity manager for Oplà using official Arduino libraries
// WHY:  Publishes tilt/button 20/s and receives game events asynchronously without delay()
// USED BY: TamaPal.ino

#ifndef NET_MQTT_H
#define NET_MQTT_H

#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include "settings.h"
#include "tilt_logic.h"

#if __has_include("arduino_secrets.h")
  #include "arduino_secrets.h"
#else
  #include "arduino_secrets.example.h"
#endif

// Callback signature for handling incoming game events from backend (hit, miss, gameover)
typedef void (*GameEventCallback)(GameEvent event);

class NetMqttClient {
public:
  NetMqttClient();

  // Initialize network module and register event callback
  void begin(GameEventCallback callback = nullptr);

  // Maintain Wi-Fi and MQTT connection non-blockingly
  void update(unsigned long now);

  // Publish tilt & button JSON packet to tamapal/opla-1/input (rate-limited to 20 Hz)
  bool publishInput(const char* jsonPayload, unsigned long now);

  // Safe disconnection when exiting to main menu
  void disconnect();

  // Connection state queries
  bool isConnected();
  bool isConnecting() const;

private:
  WiFiClient wifiClient;
  MqttClient mqttClient;
  GameEventCallback onEventCallback;

  unsigned long lastPublishTime;
  unsigned long lastReconnectAttempt;
  bool connectingState;

  // Internal connection helper (non-blocking)
  void attemptConnection(unsigned long now);

  // Check and dispatch incoming messages from subscribed topic
  void pollIncomingEvents();
};

#endif // NET_MQTT_H

