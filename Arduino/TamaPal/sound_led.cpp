// WHAT: Implementation of non-blocking buzzer tones and 5 DotStar RGB LEDs
// WHY:  Standard Arduino delay() halts everything; this state machine keeps 60 FPS smooth
// USED BY: TamaPal.ino

#include "sound_led.h"

SoundLedController::SoundLedController()
  : carrier(nullptr),
    isBuzzerPlaying(false),
    soundMuted(START_WITH_SOUND_MUTED),
    toneEndTimeMs(0),
    ledResetTimeMs(0) {
}

// Store hardware carrier reference and configure initial LED brightness
void SoundLedController::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
  if (carrier != nullptr) {
    carrier->leds.setBrightness(25); // Gentle brightness
    setMoodColor(0, 150, 220);       // Calm cyan startup glow
  }
}

// Toggle sound mute ON/OFF
void SoundLedController::toggleMute() {
  soundMuted = !soundMuted;
  if (soundMuted && carrier != nullptr) {
    carrier->Buzzer.noSound();
    isBuzzerPlaying = false;
  }
}

// Return true if sound is currently muted
bool SoundLedController::isMuted() const {
  return soundMuted;
}

// Trigger requested sound effect without any delay() calls
void SoundLedController::playSound(SoundType sound) {
  if (carrier == nullptr || soundMuted) return;

  unsigned long now = millis();

  switch (sound) {
    case SOUND_CLICK:
      carrier->Buzzer.sound(1200);   // Crisp click tone
      toneEndTimeMs = now + 18;      // Turn off after 18 ms
      isBuzzerPlaying = true;
      break;

    case SOUND_CONFIRM:
      carrier->Buzzer.sound(1760);   // Bright A6 chirp
      toneEndTimeMs = now + 40;      // Turn off after 40 ms
      isBuzzerPlaying = true;
      break;

    case SOUND_BACK:
      carrier->Buzzer.sound(440);    // Warm A4 low tone
      toneEndTimeMs = now + 35;
      isBuzzerPlaying = true;
      break;

    case SOUND_HIT:
      carrier->Buzzer.sound(1400);   // High ping for paddle hit
      toneEndTimeMs = now + 65;
      isBuzzerPlaying = true;
      setMoodColor(0, 255, 80);      // Green flash
      ledResetTimeMs = now + 200;
      break;

    case SOUND_MISS:
      carrier->Buzzer.sound(220);    // Low buzz for miss
      toneEndTimeMs = now + 150;
      isBuzzerPlaying = true;
      setMoodColor(255, 30, 30);     // Red flash
      ledResetTimeMs = now + 250;
      break;

    case SOUND_GAMEOVER:
      carrier->Buzzer.sound(330);    // Low tone for game over
      toneEndTimeMs = now + 250;
      isBuzzerPlaying = true;
      setMoodColor(255, 120, 0);     // Amber flash
      ledResetTimeMs = now + 400;
      break;

    default:
      break;
  }
}

// Trigger audio and LED reaction based on MQTT game event
void SoundLedController::playEventFeedback(GameEvent event) {
  switch (event) {
    case EVENT_HIT:      playSound(SOUND_HIT); break;
    case EVENT_MISS:     playSound(SOUND_MISS); break;
    case EVENT_GAMEOVER: playSound(SOUND_GAMEOVER); break;
    default: break;
  }
}

// Periodic check: turn off buzzer automatically when tone duration has passed
void SoundLedController::update(unsigned long currentTimeMs) {
  if (carrier == nullptr) return;

  // Buzzer non-blocking cutoff
  if (isBuzzerPlaying && currentTimeMs >= toneEndTimeMs) {
    carrier->Buzzer.noSound();
    isBuzzerPlaying = false;
  }

  // Restore calm mood LED color after event flash
  if (ledResetTimeMs > 0 && currentTimeMs >= ledResetTimeMs) {
    setMoodColor(0, 150, 220); // Restore calm cyan glow
    ledResetTimeMs = 0;
  }
}

// Set RGB color across all 5 DotStar LEDs around the circular case
void SoundLedController::setMoodColor(uint8_t red, uint8_t green, uint8_t blue) {
  if (carrier == nullptr) return;

  uint32_t color = carrier->leds.Color(red, green, blue);
  for (int i = 0; i < 5; i++) {
    carrier->leds.setPixelColor(i, color);
  }
  carrier->leds.show();
}

