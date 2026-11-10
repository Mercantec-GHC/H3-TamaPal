// WHAT: Implementation of non-blocking buzzer tones and 5 DotStar RGB LEDs
// WHY:  Standard Arduino delay() halts everything; this state machine keeps 60 FPS smooth
// USED BY: TamaPal.ino

#include "sound_led.h"

SoundLedController::SoundLedController()
  : carrier(nullptr),
    isBuzzerPlaying(false),
    soundMuted(START_WITH_SOUND_MUTED),
    toneEndTimeMs(0),
    ledResetTimeMs(0),
    baseRed(0),
    baseGreen(150),
    baseBlue(220),
    chaserActive(false),
    chaserEndTimeMs(0),
    chaserStepMs(60),
    chaserR(0),
    chaserG(0),
    chaserB(0),
    breathingActive(false),
    breathingEndTimeMs(0),
    breathR(0),
    breathG(0),
    breathB(0) {
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
  if (carrier == nullptr) return;

  unsigned long now = millis();

  // Trigger LED flashes for game events even if audio buzzer is muted
  if (sound == SOUND_HIT) {
    applyLedColor(0, 255, 80);       // Green flash
    ledResetTimeMs = now + 200;
  } else if (sound == SOUND_MISS) {
    applyLedColor(255, 30, 30);      // Red flash
    ledResetTimeMs = now + 250;
  } else if (sound == SOUND_GAMEOVER) {
    applyLedColor(255, 120, 0);      // Amber flash
    ledResetTimeMs = now + 400;
  } else if (sound == SOUND_DEATH) {
    applyLedColor(80, 20, 140);       // Somber violet fade
    ledResetTimeMs = now + 600;
  } else if (sound == SOUND_REBIRTH) {
    applyLedColor(0, 255, 180);       // Brilliant emerald-gold flash
    ledResetTimeMs = now + 500;
  } else if (sound == SOUND_PURR) {
    applyLedColor(255, 105, 180);     // Warm pink cuddle glow
    ledResetTimeMs = now + 450;
  } else if (sound == SOUND_CRACK) {
    applyLedColor(255, 220, 80);      // Golden crack sparkle
    ledResetTimeMs = now + 150;
  } else if (sound == SOUND_BUBBLES) {
    applyLedColor(0, 220, 255);       // Aqua-cyan soap foam glow
    ledResetTimeMs = now + 400;
  } else if (sound == SOUND_HEAL) {
    applyLedColor(80, 255, 120);      // Emerald healing brilliance
    ledResetTimeMs = now + 500;
  } else if (sound == SOUND_HEARTBEAT) {
    applyLedColor(220, 15, 20);       // Ruby pulse
    ledResetTimeMs = now + 120;
  } else if (sound == SOUND_FANFARE) {
    applyLedColor(255, 215, 0);       // Brilliant gold victory
    ledResetTimeMs = now + 350;
  }

  if (soundMuted) return;

  switch (sound) {
    case SOUND_CLICK:
      carrier->Buzzer.sound(520);    // Warm, gentle tactile click (was 1200 Hz shrill beep)
      toneEndTimeMs = now + 8;       // Ultra-short 8 ms tap for quiet Switch/iOS haptic feel
      isBuzzerPlaying = true;
      break;

    case SOUND_CONFIRM:
      carrier->Buzzer.sound(659);    // Soft melodic E5 blip (was 1760 Hz piercing alarm siren)
      toneEndTimeMs = now + 20;      // Gentle 20 ms chirp
      isBuzzerPlaying = true;
      break;

    case SOUND_BACK:
      carrier->Buzzer.sound(370);    // Soft low pop
      toneEndTimeMs = now + 15;      // Quiet 15 ms
      isBuzzerPlaying = true;
      break;

    case SOUND_HIT:
      carrier->Buzzer.sound(784);    // Mellow G5 paddle pop (was 1400 Hz)
      toneEndTimeMs = now + 25;      // Crisp 25 ms
      isBuzzerPlaying = true;
      break;

    case SOUND_MISS:
      carrier->Buzzer.sound(220);    // Low buzz for miss
      toneEndTimeMs = now + 100;
      isBuzzerPlaying = true;
      break;

    case SOUND_GAMEOVER:
      carrier->Buzzer.sound(294);    // Warm D4 tone for game over
      toneEndTimeMs = now + 180;
      isBuzzerPlaying = true;
      break;

    case SOUND_DEATH:
      carrier->Buzzer.sound(180);    // Low somber requiem drone
      toneEndTimeMs = now + 300;
      isBuzzerPlaying = true;
      break;

    case SOUND_REBIRTH:
      carrier->Buzzer.sound(988);    // Warm B5 chime (was 1975 Hz piercing whistle)
      toneEndTimeMs = now + 120;
      isBuzzerPlaying = true;
      break;

    case SOUND_PURR:
      carrier->Buzzer.sound(523);    // Warm C5 cuddle purr (was 1046 Hz)
      toneEndTimeMs = now + 40;
      isBuzzerPlaying = true;
      break;

    case SOUND_CRACK:
      carrier->Buzzer.sound(587);    // Soft snap (was 1600 Hz)
      toneEndTimeMs = now + 12;
      isBuzzerPlaying = true;
      break;

    case SOUND_BUBBLES:
      carrier->Buzzer.sound(740);    // Soft bubbly blip (was 1350 Hz)
      toneEndTimeMs = now + 25;
      isBuzzerPlaying = true;
      break;

    case SOUND_HEAL:
      carrier->Buzzer.sound(1046);   // Sweet C6 bell (was 2093 Hz shrill C7)
      toneEndTimeMs = now + 70;
      isBuzzerPlaying = true;
      break;

    case SOUND_HEARTBEAT:
      carrier->Buzzer.sound(110);    // Low organic heart thump
      toneEndTimeMs = now + 45;
      isBuzzerPlaying = true;
      break;

    case SOUND_FANFARE:
      carrier->Buzzer.sound(880);    // Warm A5 fanfare (was 1760 Hz piercing tone)
      toneEndTimeMs = now + 150;
      isBuzzerPlaying = true;
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

void SoundLedController::startChaser(uint8_t r, uint8_t g, uint8_t b, unsigned long durationMs, uint16_t stepMs) {
  chaserActive = true;
  chaserEndTimeMs = millis() + durationMs;
  chaserStepMs = (stepMs > 10) ? stepMs : 60;
  chaserR = r; chaserG = g; chaserB = b;
  breathingActive = false;
  ledResetTimeMs = 0;
}

void SoundLedController::startBreathing(uint8_t r, uint8_t g, uint8_t b, unsigned long durationMs) {
  breathingActive = true;
  breathingEndTimeMs = millis() + durationMs;
  breathR = r; breathG = g; breathB = b;
  chaserActive = false;
  ledResetTimeMs = 0;
}

// Periodic check: turn off buzzer automatically when tone duration has passed
void SoundLedController::update(unsigned long currentTimeMs) {
  if (carrier == nullptr) return;

  // Buzzer non-blocking cutoff
  if (isBuzzerPlaying && currentTimeMs >= toneEndTimeMs) {
    carrier->Buzzer.noSound();
    isBuzzerPlaying = false;
  }

  // Active circular chaser animation
  if (chaserActive) {
    if (currentTimeMs >= chaserEndTimeMs) {
      chaserActive = false;
      applyLedColor(baseRed, baseGreen, baseBlue);
    } else {
      uint8_t activeLed = (uint8_t)((currentTimeMs / chaserStepMs) % 5);
      uint32_t activeColor = carrier->leds.Color(chaserR, chaserG, chaserB);
      uint32_t dimColor = carrier->leds.Color(chaserR / 5, chaserG / 5, chaserB / 5);
      for (int i = 0; i < 5; i++) {
        carrier->leds.setPixelColor(i, (i == activeLed) ? activeColor : dimColor);
      }
      carrier->leds.show();
      return;
    }
  }

  // Active pulsating breathing animation
  if (breathingActive) {
    if (currentTimeMs >= breathingEndTimeMs) {
      breathingActive = false;
      applyLedColor(baseRed, baseGreen, baseBlue);
    } else {
      uint16_t cycle = (uint16_t)(currentTimeMs % 2400); // 2.4s calm breathing period
      uint8_t wave = (cycle < 1200) ? (cycle * 255 / 1200) : ((2400 - cycle) * 255 / 1200);
      uint8_t factor = 40 + (wave * 215 / 255);
      uint8_t r = (uint16_t)breathR * factor / 255;
      uint8_t g = (uint16_t)breathG * factor / 255;
      uint8_t b = (uint16_t)breathB * factor / 255;
      applyLedColor(r, g, b);
      return;
    }
  }

  // Restore saved base mood LED color after event flash
  if (ledResetTimeMs > 0 && currentTimeMs >= ledResetTimeMs) {
    ledResetTimeMs = 0;
    applyLedColor(baseRed, baseGreen, baseBlue);
  }
}

// Set persistent base RGB color across all 5 DotStar LEDs
void SoundLedController::setMoodColor(uint8_t red, uint8_t green, uint8_t blue) {
  baseRed = red;
  baseGreen = green;
  baseBlue = blue;
  ledResetTimeMs = 0;
  applyLedColor(red, green, blue);
}

// Temporary flash that reverts to base color after durationMs
void SoundLedController::flashColor(uint8_t red, uint8_t green, uint8_t blue, unsigned long durationMs) {
  if (carrier == nullptr) return;
  applyLedColor(red, green, blue);
  ledResetTimeMs = millis() + durationMs;
}

// Internal hardware helper to write RGB values to the 5 LEDs
void SoundLedController::applyLedColor(uint8_t red, uint8_t green, uint8_t blue) {
  if (carrier == nullptr) return;

  uint32_t color = carrier->leds.Color(red, green, blue);
  for (int i = 0; i < 5; i++) {
    carrier->leds.setPixelColor(i, color);
  }
  carrier->leds.show();
}

