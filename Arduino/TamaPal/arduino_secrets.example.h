// WHAT: Example template for Wi-Fi and MQTT network credentials
// WHY:  Keep sensitive passwords out of Git (arduino_secrets.h is git-ignored)
// USED BY: net_mqtt.cpp (once copied to arduino_secrets.h)

#ifndef ARDUINO_SECRETS_H
#define ARDUINO_SECRETS_H

// Copy this file to "arduino_secrets.h" in the same folder and fill in your real credentials:
#define SECRET_WIFI_SSID "Your_WiFi_Name"
#define SECRET_WIFI_PASS "Your_WiFi_Password"

#define SECRET_MQTT_HOST "138.199.155.78" // IP address of team MQTT broker
#define SECRET_MQTT_PORT 1883
#define SECRET_MQTT_USER ""
#define SECRET_MQTT_PASS ""

#endif // ARDUINO_SECRETS_H

