// WHAT: Mathematical calculations for Oplà IMU tilt angles and MQTT payload formatting
// WHY:  Pure C++ implementation without Arduino dependencies so it can be 100% unit tested
// USED BY: TamaPal.ino, net_mqtt.cpp, tests/test_main.cpp

#include "tilt_logic.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

TiltLogic::TiltLogic()
  : rawTiltY(0.0f),
    rawTiltX(0.0f),
    smoothedTiltY(0.0f),
    smoothedTiltX(0.0f),
    buttonState(false) {
}

void TiltLogic::reset() {
  rawTiltY = 0.0f;
  rawTiltX = 0.0f;
  smoothedTiltY = 0.0f;
  smoothedTiltX = 0.0f;
  buttonState = false;
}

// Convert an acceleration component and gravity vector to a normalized [-1.0 .. 1.0] tilt
float TiltLogic::calculateNormalizedTilt(float axisAccel, float gravityAccel) {
  // Use atan2 to calculate angle in radians, then convert to degrees
  float angleRad = atan2f(axisAccel, gravityAccel);
  float angleDeg = angleRad * (180.0f / (float)M_PI);

  // Normalize by MAX_TILT_DEG (e.g., 30 degrees = 1.0)
  float tilt = angleDeg / MAX_TILT_DEG;

  // Clamp strictly between -1.0 and 1.0
  if (std::isnan(tilt)) tilt = 0.0f;
  if (tilt > 1.0f) tilt = 1.0f;
  if (tilt < -1.0f) tilt = -1.0f;

  // Apply deadzone around zero to prevent idle paddle jitter
  if (fabsf(tilt) < TILT_DEADZONE) {
    tilt = 0.0f;
  }

  return tilt;
}

void TiltLogic::updateAcceleration(float ax, float ay, float az) {
  // The gravity reference vector is az when holding the carrier face-up
  // If az is close to 0 (board completely vertical), fallback to 0.1 to avoid division instability
  float refZ = (fabsf(az) < 0.05f) ? 0.05f : az;

  // Calculate raw normalized tilts for Y (forward/backward) and X (left/right)
  rawTiltY = calculateNormalizedTilt(ay, refZ);
  rawTiltX = calculateNormalizedTilt(ax, refZ);

  // Apply exponential smoothing filter: (smoothed * 0.70) + (raw * 0.30)
  // This removes sensor noise while maintaining immediate responsiveness
  smoothedTiltY = (smoothedTiltY * (1.0f - TILT_SMOOTH_FACTOR)) + (rawTiltY * TILT_SMOOTH_FACTOR);
  smoothedTiltX = (smoothedTiltX * (1.0f - TILT_SMOOTH_FACTOR)) + (rawTiltX * TILT_SMOOTH_FACTOR);

  // Clean snap to exact zero when near zero
  if (fabsf(smoothedTiltY) < 0.005f) smoothedTiltY = 0.0f;
  if (fabsf(smoothedTiltX) < 0.005f) smoothedTiltX = 0.0f;
}

void TiltLogic::setButtonPressed(bool pressed) {
  buttonState = pressed;
}

float TiltLogic::getTiltY() const {
  return smoothedTiltY;
}

float TiltLogic::getTiltX() const {
  return smoothedTiltX;
}

float TiltLogic::getRawTiltY() const {
  return rawTiltY;
}

float TiltLogic::getRawTiltX() const {
  return rawTiltX;
}

bool TiltLogic::isButtonPressed() const {
  return buttonState;
}

// Format the JSON payload safely without '%f' (avoiding SAMD21 float printf bug)
size_t TiltLogic::formatInputJson(char* buffer, size_t maxLen) const {
  if (buffer == nullptr || maxLen == 0) return 0;

  // Convert float -1.00..1.00 to integer cents (-100..100)
  int vy = (int)lroundf(smoothedTiltY * 100.0f);
  const char* signY = (vy < 0) ? "-" : "";
  if (vy < 0) vy = -vy;
  if (vy > 100) vy = 100;

  int vx = (int)lroundf(smoothedTiltX * 100.0f);
  const char* signX = (vx < 0) ? "-" : "";
  if (vx < 0) vx = -vx;
  if (vx > 100) vx = 100;

  // "tilt" (Pong compatibility) + "tiltX" (Maze / 2D steering) + "button"
  int written = snprintf(buffer, maxLen,
                         "{\"tilt\":%s%d.%02d,\"tiltX\":%s%d.%02d,\"button\":%s}",
                         signY, vy / 100, vy % 100,
                         signX, vx / 100, vx % 100,
                         buttonState ? "true" : "false");

  if (written < 0 || (size_t)written >= maxLen) {
    buffer[maxLen - 1] = '\0';
    return maxLen - 1;
  }

  return (size_t)written;
}

// Parse MQTT event string into GameEvent enum
GameEvent TiltLogic::parseEventType(const char* typeStr) {
  if (typeStr == nullptr) return EVENT_NONE;
  if (strcmp(typeStr, "hit") == 0) return EVENT_HIT;
  if (strcmp(typeStr, "miss") == 0) return EVENT_MISS;
  if (strcmp(typeStr, "gameover") == 0) return EVENT_GAMEOVER;
  return EVENT_NONE;
}

