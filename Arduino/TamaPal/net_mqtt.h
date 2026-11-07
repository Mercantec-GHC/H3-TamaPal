// WHAT: Wi-Fi and MQTT connectivity manager for Oplà using official Arduino libraries
// WHY:  Publishes tilt/button 20/s and receives game events asynchronously without delay()
// USED BY: TamaPal.ino

#ifndef NET_MQTT_H
#define NET_MQTT_H

#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include "settings.h"
#include "tilt_logic.h"

#if __has_include("arduino_secrets.h")
  #include "arduino_secrets.h"
#else
  #include "arduino_secrets.example.h"
#endif

// Callback signature for handling incoming game events from backend (hit, miss, gameover)
typedef void (*GameEventCallback)(GameEvent event);

// Callback signature for handling incoming pet rewards from backend (e.g. "happy_boost", 25)
typedef void (*PetRewardCallback)(const char* type, int amount);

class NetMqttClient {
public:
  NetMqttClient();

  // Initialize network module and register event & reward callbacks
  void begin(GameEventCallback eventCallback = nullptr, PetRewardCallback rewardCallback = nullptr);

  // Maintain Wi-Fi and MQTT connection non-blockingly
  void update(unsigned long now);

  // Check if client is ready and due to publish a packet right now
  bool canPublish(unsigned long now, bool forceImmediate = false);

  // Publish tilt & button JSON packet to tamapal/opla-1/input
  // Sends immediately if forceImmediate is true (instant state-change edge),
  // otherwise maintains periodic heartbeat at 20 Hz (~50 ms)
  bool publishInput(const char* jsonPayload, unsigned long now, bool forceImmediate = false);

  // Publish pet vital state JSON packet to StormLamaerne/Tamapal/opla-1/pet/state
  bool publishPetState(const char* jsonPayload);

  // Safe disconnection when exiting to main menu
  void disconnect();

  // Connection state queries
  bool isConnected();
  bool isConnecting() const;

  // Configure dynamic player slot (1 or 2) for MQTT topics and client ID
  void setPlayerId(int id);
  int getPlayerId() const;

private:
  // Objects & pointers
  WiFiClient wifiClient;
  MqttClient mqttClient;
  GameEventCallback onEventCallback;
  PetRewardCallback onRewardCallback;

  // Timers
  unsigned long lastPublishTime;
  unsigned long lastReconnectAttempt;

  // Small integer and compressed string buffers (zero dynamic heap)
  int16_t playerId;
  char topicInput[44];
  char topicEvent[44];
  char topicReward[48];
  char topicPetState[48];
  char clientMqttId[28];

  // Boolean flags & backoff counter
  uint8_t wifiFailCount;
  bool connectingState;
  bool lastReportedWifiConnected;
  bool hasPublishedFirstPacket;

  // Internal connection helper (non-blocking)
  void attemptConnection(unsigned long now);

  // Check and dispatch incoming messages from subscribed topic
  void pollIncomingEvents();
};

#endif // NET_MQTT_H

