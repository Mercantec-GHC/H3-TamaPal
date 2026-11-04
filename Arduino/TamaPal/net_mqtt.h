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

  // Publish tilt & button JSON packet to tamapal/opla-1/input
  // Sends immediately if forceImmediate is true (instant state-change edge),
  // otherwise maintains periodic heartbeat at 20 Hz (~50 ms)
  bool publishInput(const char* jsonPayload, unsigned long now, bool forceImmediate = false);

  // Safe disconnection when exiting to main menu
  void disconnect();

  // Connection state queries
  bool isConnected();
  bool isConnecting() const;

  // Configure dynamic player slot (1 or 2) for MQTT topics and client ID
  void setPlayerId(int id);
  int getPlayerId() const;

private:
  WiFiClient wifiClient;
  MqttClient mqttClient;
  GameEventCallback onEventCallback;

  int playerId;
  char topicInput[36];
  char topicEvent[36];
  char clientMqttId[36];

  unsigned long lastPublishTime;
  unsigned long lastReconnectAttempt;
  bool connectingState;

  // Internal connection helper (non-blocking)
  void attemptConnection(unsigned long now);

  // Check and dispatch incoming messages from subscribed topic
  void pollIncomingEvents();
};

#endif // NET_MQTT_H

