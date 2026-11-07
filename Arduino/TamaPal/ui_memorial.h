// WHAT: Graphical renderer for the Hall of Fame / Ancestral Memorial gallery (SCREEN_MEMORIAL)
// WHY:  Renders ancestors from grave.txt with cenotaph stele, stats, and Danish epitaphs
// USED BY: TamaPal.ino

#ifndef UI_MEMORIAL_H
#define UI_MEMORIAL_H

#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "pet_logic.h"
#include "storage_pet.h"

class UiMemorialRenderer {
public:
  UiMemorialRenderer();

  // Attach hardware carrier display pointer
  void begin(MKRIoTCarrier* hardwareCarrier);

  // Draws the sacred cemetery forest clearing with trees, meadow, graves and hovering selector wisp
  void drawCemeteryGrove(uint16_t selectedIndex, uint16_t totalCount, StoragePet& storage, uint16_t currentGen);

  // Draws the detailed life cycle and chronicle card for a specific ancestor
  void drawLifeCycleDetail(uint16_t index, uint16_t totalCount, const GraveRecord& record, uint16_t currentGen);

  // Draws full record view for ancestor at selectedIndex (backwards compatibility)
  void drawRecord(uint16_t index, uint16_t totalCount, const GraveRecord& record);

  // Draws when no ancestral graves exist yet (Gen 1 is still alive)
  void drawEmpty(uint16_t currentGen);

private:
  MKRIoTCarrier* carrier;

  void drawMysticalSkyAndMoon();
  void drawPineTree(int x, int baseY, int scale);
  void drawGraveStone(int cx, int cy, const char* name, uint16_t gen, bool isSelected, uint8_t style);
  void drawHoveringWisp(int cx, int cy);
  void drawBackgroundStars();
  void drawAncestralStele(int cx, int cy);
  void drawHoveringSpirit(int cx, int cy);

  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_MEMORIAL_H

