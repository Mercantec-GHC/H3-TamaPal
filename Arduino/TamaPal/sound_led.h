// WHAT: Non-blocking sound generator (Buzzer) and RGB LED controller
// WHY:  Eliminates delay() so screen animation and MQTT never stutter
// USED BY: TamaPal.ino, menu_logic.cpp

#ifndef SOUND_LED_H
#define SOUND_LED_H

#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"

class SoundLedController {
public:
  SoundLedController();

  // Attach reference to hardware carrier
  void begin(MKRIoTCarrier* hardwareCarrier);

  // Trigger non-blocking sound effect
  void playSound(SoundType sound);

  // Play audio/LED feedback for MQTT game event (hit, miss, gameover)
  void playEventFeedback(GameEvent event);

  // Periodic non-blocking update (checks if tone duration expired)
  void update(unsigned long currentTimeMs);

  // Set color pattern for all 5 RGB LEDs
  void setMoodColor(uint8_t red, uint8_t green, uint8_t blue);

  // Temporary LED flash that automatically restores the base mood color
  void flashColor(uint8_t red, uint8_t green, uint8_t blue, unsigned long durationMs = 300);

  // Start circular running LED chaser effect around the 5 LEDs
  void startChaser(uint8_t r, uint8_t g, uint8_t b, unsigned long durationMs, uint16_t stepMs = 60);

  // Start gentle pulsating breathing effect across all LEDs
  void startBreathing(uint8_t r, uint8_t g, uint8_t b, unsigned long durationMs);

  // Toggle sound mute ON/OFF (triggered by TOUCH0 + TOUCH4 combo)
  void toggleMute();
  bool isMuted() const;

private:
  MKRIoTCarrier* carrier;

  bool isBuzzerPlaying;
  bool soundMuted;
  unsigned long toneEndTimeMs;
  unsigned long ledResetTimeMs;
  uint8_t baseRed;
  uint8_t baseGreen;
  uint8_t baseBlue;

  // Non-blocking Chaser & Breathing state
  bool chaserActive;
  unsigned long chaserEndTimeMs;
  uint16_t chaserStepMs;
  uint8_t chaserR, chaserG, chaserB;

  bool breathingActive;
  unsigned long breathingEndTimeMs;
  uint8_t breathR, breathG, breathB;

  void applyLedColor(uint8_t red, uint8_t green, uint8_t blue);
};

#endif // SOUND_LED_H

