// WHAT: Graphical renderer for the Hall of Fame / Ancestral Memorial gallery (SCREEN_MEMORIAL)
// WHY:  Renders ancestors from grave.txt with cenotaph stele, stats, and Danish epitaphs
// USED BY: TamaPal.ino

#ifndef UI_MEMORIAL_H
#define UI_MEMORIAL_H

#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "pet_logic.h"

class UiMemorialRenderer {
public:
  UiMemorialRenderer();

  // Attach hardware carrier display pointer
  void begin(MKRIoTCarrier* hardwareCarrier);

  // Draws full record view for ancestor at selectedIndex
  void drawRecord(uint16_t index, uint16_t totalCount, const GraveRecord& record);

  // Draws when no ancestral graves exist yet (Gen 1 is still alive)
  void drawEmpty(uint16_t currentGen);

private:
  MKRIoTCarrier* carrier;

  void drawBackgroundStars();
  void drawAncestralStele(int cx, int cy);
  void drawHoveringSpirit(int cx, int cy);

  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_MEMORIAL_H

