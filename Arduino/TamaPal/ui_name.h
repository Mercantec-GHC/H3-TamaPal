// WHAT: Arcade 4-letter Name Entry screen renderer for Oplà display
// WHY:  Allows player to give Lumi a personalized retro name before birth
// USED BY: TamaPal.ino

#ifndef UI_NAME_H
#define UI_NAME_H

#include <stdint.h>
#include <Arduino_MKRIoTCarrier.h>
#include "name_logic.h"

class UiNameRenderer {
public:
  UiNameRenderer();

  void begin(MKRIoTCarrier* hardwareCarrier);

  // Render or update name entry screen (delta or full redraw)
  void drawNameEntry(const NameEntryLogic& nameLogic, unsigned long nowMs, bool forceRedraw = false);

private:
  MKRIoTCarrier* carrier;
  uint8_t lastCursorSlot;
  char lastSlots[NameEntryLogic::MAX_NAME_LENGTH];
  bool lastArrowBlink;
  bool screenInitialized;

  void drawBox(uint8_t slot, char letter, bool isSelected, bool blinkArrow);

  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_NAME_H

