// WHAT: Mathematical calculation of IMU tilts, deadzone, smoothing, and JSON formatting
// WHY:  Decouples accelerometer math and JSON formatting from hardware for unit-testing
// USED BY: TamaPal.ino, net_mqtt.cpp, tests/test_main.cpp

#ifndef TILT_LOGIC_H
#define TILT_LOGIC_H

#include <stddef.h>
#include "settings.h"

class TiltLogic {
public:
  TiltLogic();

  // Reset filter history to center (0.0, 0.0)
  void reset();

  // Update raw accelerometer inputs (values in G: e.g. from LSM6DS3 on carrier)
  // ax: left/right acceleration (maps to horizontal tiltX for maze / steering)
  // ay: forward/backward acceleration (maps to vertical tiltY for Pong paddle)
  // az: vertical / gravity reference vector
  void updateAcceleration(float ax, float ay, float az);

  // Set button states for Pong (Up=TOUCH0, Down=TOUCH1, Action=TOUCH2)
  void setButtons(bool up, bool down, bool action);
  void setButtonPressed(bool pressed); // Compatibility helper for action button

  // Get smoothed normalized tilt values in range [-1.0 .. 1.0]
  // -1.0 = tilted forward/up, +1.0 = tilted backward/down
  float getTiltY() const;

  // -1.0 = tilted left, +1.0 = tilted right
  float getTiltX() const;

  // Get raw unsmoothed tilt values (useful for diagnostics)
  float getRawTiltY() const;
  float getRawTiltX() const;

  // Current button states
  bool isUpPressed() const;
  bool isDownPressed() const;
  bool isButtonPressed() const;

  // Format MQTT input JSON packet: {"up":false,"down":false,"button":false,"tiltX":0.10,"tiltY":0.35,"tilt":0.35}
  // Safe on SAMD21: formatted using integer arithmetic, no '%f'
  size_t formatInputJson(char* buffer, size_t maxLen) const;

  // Parse incoming MQTT event type from backend ("hit", "miss", "gameover")
  static GameEvent parseEventType(const char* typeStr);

private:
  float rawTiltY;
  float rawTiltX;
  float smoothedTiltY;
  float smoothedTiltX;
  bool upState;
  bool downState;
  bool buttonState;

  // Convert raw acceleration and gravity vector to a clamped [-1.0 .. 1.0] tilt value
  static float calculateNormalizedTilt(float axisAccel, float gravityAccel);
};

#endif // TILT_LOGIC_H

