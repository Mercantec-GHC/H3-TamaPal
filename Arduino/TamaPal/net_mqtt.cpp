// WHAT: Implementation of Wi-Fi and MQTT connectivity using official Arduino libraries
// WHY:  Asynchronous, non-blocking network I/O for 20 Hz controller inputs and incoming events
// USED BY: TamaPal.ino

#include "net_mqtt.h"

// Helper to translate WiFiNINA status codes into readable Danish/English diagnostics
static const char* wifiStatusToString(uint8_t status) {
  switch (status) {
    case WL_CONNECTED:       return "CONNECTED (Forbundet)";
    case WL_NO_SHIELD:       return "NO_SHIELD (WiFi-modul svarer ikke)";
    case WL_IDLE_STATUS:     return "IDLE_STATUS (Forbinder...)";
    case WL_NO_SSID_AVAIL:   return "NO_SSID_AVAIL (Netvaerk ikke fundet - tjek SSID & 2.4 GHz)";
    case WL_SCAN_COMPLETED:  return "SCAN_COMPLETED";
    case WL_CONNECT_FAILED:  return "CONNECT_FAILED (Forbindelse fejlede - tjek kodeord)";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST (Forbindelse tabt)";
    case WL_DISCONNECTED:    return "DISCONNECTED (Afbrudt)";
    default:                 return "UNKNOWN_STATUS";
  }
}

// Helper to translate ArduinoMqttClient connect error codes into actionable advice
static const char* mqttErrorToString(int err) {
  switch (err) {
    case MQTT_SUCCESS:                        return "SUCCESS (0)";
    case MQTT_CONNECTION_REFUSED:            return "CONNECTION_REFUSED (-2: TCP afvist / port 1883 lukket)";
    case MQTT_CONNECTION_TIMEOUT:            return "CONNECTION_TIMEOUT (-1: Intet svar / firewall blokerer)";
    case MQTT_UNACCEPTABLE_PROTOCOL_VERSION:  return "UNACCEPTABLE_PROTOCOL_VERSION (1)";
    case MQTT_IDENTIFIER_REJECTED:            return "IDENTIFIER_REJECTED (2: Client ID afvist)";
    case MQTT_SERVER_UNAVAILABLE:             return "SERVER_UNAVAILABLE (3: Broker nede)";
    case MQTT_BAD_USER_NAME_OR_PASSWORD:      return "BAD_USER_NAME_OR_PASSWORD (4: Forkert login)";
    case MQTT_NOT_AUTHORIZED:                 return "NOT_AUTHORIZED (5: Kraever login - tjek Teams!)";
    default:                                  return "UKENDT_FEJL";
  }
}

NetMqttClient::NetMqttClient()
  : mqttClient(wifiClient),
    onEventCallback(nullptr),
    playerId(1),
    lastPublishTime(0),
    lastReconnectAttempt(0),
    connectingState(false),
    lastReportedWifiConnected(false),
    hasPublishedFirstPacket(false) {
  // Pure in-memory initialization only: NO hardware I/O (WiFi, SPI) before setup()!
  topicInput[0] = '\0';
  topicEvent[0] = '\0';
  clientMqttId[0] = '\0';
}

void NetMqttClient::begin(GameEventCallback callback) {
  onEventCallback = callback;
  setPlayerId(1);

  // Set MQTT client credentials, keep-alive, and connection timeout
  mqttClient.setId(clientMqttId);
  mqttClient.setKeepAliveInterval(15 * 1000); // 15 seconds keep-alive
  mqttClient.setConnectionTimeout(4000);       // 4s timeout (prevents 30s freeze if firewall blocks port 1883)

  #if defined(SECRET_MQTT_USER) && defined(SECRET_MQTT_PASS)
  if (strlen(SECRET_MQTT_USER) > 0) {
    mqttClient.setUsernamePassword(SECRET_MQTT_USER, SECRET_MQTT_PASS);
  }
  #endif
}

void NetMqttClient::setPlayerId(int id) {
  int newId = (id == 2) ? 2 : 1;
  if (playerId != newId && mqttClient.connected()) {
    Serial.print(F("[MQTT] Skifter aktiv rolle: Spiller "));
    Serial.print(playerId);
    Serial.print(F(" -> Spiller "));
    Serial.println(newId);
    disconnect();
  }
  playerId = newId;
  snprintf(topicInput, sizeof(topicInput), "%s/opla-%d/input", MQTT_TOPIC_PREFIX, playerId);
  snprintf(topicEvent, sizeof(topicEvent), "%s/opla-%d/event", MQTT_TOPIC_PREFIX, playerId);
  snprintf(clientMqttId, sizeof(clientMqttId), "tamapal-opla-%d", playerId);
}

int NetMqttClient::getPlayerId() const {
  return playerId;
}

bool NetMqttClient::isConnected() {
  return (WiFi.status() == WL_CONNECTED) && mqttClient.connected();
}

bool NetMqttClient::isConnecting() const {
  return connectingState;
}

void NetMqttClient::disconnect() {
  if (mqttClient.connected()) {
    Serial.println(F("[MQTT] Lukker forbindelse til broker..."));
    mqttClient.stop();
  }
  connectingState = false;
  lastReportedWifiConnected = false;
  hasPublishedFirstPacket = false;
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
    #if defined(SECRET_WIFI_SSID) && defined(SECRET_WIFI_PASS)
    if (strlen(SECRET_WIFI_SSID) == 0) {
      Serial.println(F("[WiFi] FEJL: SECRET_WIFI_SSID er tom! Udfyld arduino_secrets.h."));
      return;
    }

    Serial.print(F("[WiFi] Forbinder til '"));
    Serial.print(SECRET_WIFI_SSID);
    Serial.println(F("'..."));

    wifiStatus = WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
    Serial.print(F("[WiFi] Status efter forsoeg: "));
    Serial.println(wifiStatusToString(wifiStatus));

    if (wifiStatus != WL_CONNECTED) {
      return;
    }
    #else
    Serial.println(F("[WiFi] FEJL: Ingen arduino_secrets.h fundet!"));
    return;
    #endif
  }

  // Wi-Fi is connected!
  if (!lastReportedWifiConnected) {
    lastReportedWifiConnected = true;
    Serial.println(F("--------------------------------------------------"));
    Serial.print(F("[WiFi] FORBUNDET! Lokal IP: "));
    Serial.print(WiFi.localIP());
    Serial.print(F(" | Signal: "));
    Serial.print(WiFi.RSSI());
    Serial.println(F(" dBm"));
    Serial.println(F("--------------------------------------------------"));

    // Update client ID now that hardware MAC is guaranteed readable
    byte mac[6] = {0};
    WiFi.macAddress(mac);
    if (mac[4] != 0 || mac[5] != 0) {
      snprintf(clientMqttId, sizeof(clientMqttId), "tamapal-opla-%d-%02X%02X", playerId, mac[4], mac[5]);
    } else {
      snprintf(clientMqttId, sizeof(clientMqttId), "tamapal-opla-%d", playerId);
    }
  }

  // 2. Connect to MQTT broker
  if (!mqttClient.connected()) {
    #if defined(SECRET_MQTT_HOST) && defined(SECRET_MQTT_PORT)
    Serial.println(F("--- [MQTT] Forbindelsesforsoeg ---"));
    Serial.print(F("[MQTT] Broker: "));
    Serial.print(SECRET_MQTT_HOST);
    Serial.print(F(":"));
    Serial.println(SECRET_MQTT_PORT);
    Serial.print(F("[MQTT] Client ID: "));
    Serial.println(clientMqttId);
    Serial.print(F("[MQTT] Input topic (TX): "));
    Serial.println(topicInput);
    Serial.print(F("[MQTT] Event topic (RX): "));
    Serial.println(topicEvent);

    #if defined(SECRET_MQTT_USER) && defined(SECRET_MQTT_PASS)
    if (strlen(SECRET_MQTT_USER) > 0) {
      mqttClient.setUsernamePassword(SECRET_MQTT_USER, SECRET_MQTT_PASS);
      Serial.print(F("[MQTT] Bruger: "));
      Serial.println(SECRET_MQTT_USER);
    } else {
      Serial.println(F("[MQTT] Tilstand: Anonym (uden brugernavn/adgangskode)"));
    }
    #endif

    mqttClient.setId(clientMqttId);

    // Bypass DNS if host is an IPv4 string (avoids WiFiNINA hostByName failure on numerical IPs)
    IPAddress brokerIp;
    bool isNumericIp = brokerIp.fromString(SECRET_MQTT_HOST);
    int connResult = 0;

    if (isNumericIp) {
      Serial.print(F("[MQTT] Forbinder direkte via IP: "));
      Serial.println(brokerIp);
      connResult = mqttClient.connect(brokerIp, SECRET_MQTT_PORT);
    } else {
      Serial.print(F("[MQTT] Forbinder via hostname: "));
      Serial.println(SECRET_MQTT_HOST);
      connResult = mqttClient.connect(SECRET_MQTT_HOST, SECRET_MQTT_PORT);
    }

    if (connResult != 0) {
      Serial.println(F("[MQTT] SUCCES! Forbundet til broker."));
      mqttClient.subscribe(topicEvent, 1);
      Serial.print(F("[MQTT] Abonnerer paa event-topic: "));
      Serial.println(topicEvent);
      connectingState = false;
      hasPublishedFirstPacket = false;
    } else {
      int err = mqttClient.connectError();
      Serial.print(F("[MQTT] FEJL: Kunne ikke forbinde! Kode: "));
      Serial.print(err);
      Serial.print(F(" ("));
      Serial.print(mqttErrorToString(err));
      Serial.println(F(")"));

      if (err == MQTT_NOT_AUTHORIZED || err == MQTT_BAD_USER_NAME_OR_PASSWORD) {
        Serial.println(F("[MQTT] TIP: Brokeren kraever gyldig adgangskode! Tjek login fra Mathias i Teams."));
      } else if (err == MQTT_CONNECTION_TIMEOUT || err == MQTT_CONNECTION_REFUSED) {
        Serial.println(F("[MQTT] TIP: Timeout / Afvist. Tjek om skolens Wi-Fi blokerer port 1883, eller proev mobil hotspot (2.4 GHz)."));
      }
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

  Serial.print(F("[MQTT RX] Modtaget besked: "));
  Serial.println(buffer);

  // Parse JSON using battle-tested ArduinoJson v7
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buffer);
  if (error) {
    Serial.print(F("[MQTT RX] JSON parsing fejlede: "));
    Serial.println(error.f_str());
    return;
  }

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

bool NetMqttClient::publishInput(const char* jsonPayload, unsigned long now, bool forceImmediate) {
  if (jsonPayload == nullptr || !isConnected()) {
    return false;
  }

  // Rate-limit publishing to ~20 Hz (every 50 ms), UNLESS a button state changed immediately!
  if (!forceImmediate && (now - lastPublishTime < MQTT_PUBLISH_INTERVAL_MS)) {
    return false;
  }

  lastPublishTime = now;

  // Publish to dynamic input topic (tamapal/opla-1/input or opla-2/input) with QoS 0
  mqttClient.beginMessage(topicInput, false, 0, false);
  mqttClient.print(jsonPayload);
  mqttClient.endMessage();

  if (!hasPublishedFirstPacket) {
    hasPublishedFirstPacket = true;
    Serial.println(F("[MQTT] Foerste input-pakke sendt til broker!"));
    Serial.print(F("[MQTT TX] "));
    Serial.print(topicInput);
    Serial.print(F(" -> "));
    Serial.println(jsonPayload);
    Serial.println(F("[MQTT] Sender nu kontinuerligt input ved 20 Hz..."));
  } else if (forceImmediate) {
    Serial.print(F("[MQTT TX Edge] "));
    Serial.println(jsonPayload);
  }

  return true;
}
