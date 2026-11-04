// WHAT: Graphical renderer for the 5 Pet Vitals dashboard screen (SCREEN_STATS)
// WHY:  Draws 5 colorful horizontal progress bars and mood status with zero flicker
// USED BY: TamaPal.ino

#ifndef UI_STATS_H
#define UI_STATS_H

#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "pet_logic.h"

class UiStatsRenderer {
public:
  UiStatsRenderer();

  // Attach hardware display pointer
  void begin(MKRIoTCarrier* hardwareCarrier);

  // Full initial screen draw (circular guide, header, bar frames, labels, footer)
  void drawInitialScreen();

  // Flicker-free dirty updates (redraws only changed bar fills, percentages, and mood)
  void updateDisplay(const PetLogic& pet, bool forceRedraw = false);

private:
  MKRIoTCarrier* carrier;

  int lastHunger;
  int lastWarmth;
  int lastEnergy;
  int lastHappiness;
  int lastDiscipline;
  char lastMood[32];
  bool isInitialized;

  void drawSingleBar(int y, const char* label, int val, int& lastVal, uint16_t barColor, bool force);

  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_STATS_H

