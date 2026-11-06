#include "ui_name.h"

UiNameRenderer::UiNameRenderer()
  : carrier(nullptr),
    lastCursorSlot(255),
    lastArrowBlink(false),
    screenInitialized(false) {
  for (uint8_t i = 0; i < NameEntryLogic::MAX_NAME_LENGTH; ++i) {
    lastSlots[i] = '\0';
  }
}

void UiNameRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
}

void UiNameRenderer::drawBox(uint8_t slot, char letter, bool isSelected, bool blinkArrow) {
  if (carrier == nullptr) return;

  int boxX = 46 + (slot * 38);
  int boxY = 82;
  int boxW = 34;
  int boxH = 44;

  uint16_t bgNative   = color565(8, 12, 26);
  uint16_t cardBg     = isSelected ? color565(22, 36, 62) : color565(14, 20, 36);
  uint16_t cardBorder = isSelected ? color565(60, 240, 150) : color565(40, 58, 90);
  uint16_t textColor  = isSelected ? 0xFFFF : color565(160, 195, 230);
  uint16_t arrowGold  = color565(255, 215, 60);

  // Clear arrow zones
  carrier->display.fillRect(boxX - 2, boxY - 14, boxW + 4, 12, bgNative);
  carrier->display.fillRect(boxX - 2, boxY + boxH + 2, boxW + 4, 12, bgNative);

  // Card box
  carrier->display.fillRoundRect(boxX, boxY, boxW, boxH, 6, cardBg);
  carrier->display.drawRoundRect(boxX, boxY, boxW, boxH, 6, cardBorder);

  // Letter inside box
  carrier->display.setTextSize(3);
  carrier->display.setTextColor(textColor);
  carrier->display.setCursor(boxX + 9, boxY + 12);
  if (letter == ' ') {
    carrier->display.setTextColor(color565(75, 110, 150));
    carrier->display.print('_');
  } else {
    carrier->display.print(letter);
  }

  // Blinking selector arrows above and below box
  if (isSelected && blinkArrow) {
    // Up arrow above box
    carrier->display.fillTriangle(boxX + 17, boxY - 11, boxX + 10, boxY - 3, boxX + 24, boxY - 3, arrowGold);
    // Down arrow below box
    carrier->display.fillTriangle(boxX + 17, boxY + boxH + 11, boxX + 10, boxY + boxH + 3, boxX + 24, boxY + boxH + 3, arrowGold);
  }
}

void UiNameRenderer::drawNameEntry(const NameEntryLogic& nameLogic, unsigned long nowMs, bool forceRedraw) {
  if (carrier == nullptr) return;

  uint16_t bgNative   = color565(8, 12, 26);
  uint16_t goldColor  = color565(255, 215, 60);
  uint16_t mintColor  = color565(155, 248, 220);
  uint16_t textDim    = color565(160, 190, 225);

  bool blinkArrow = ((nowMs / 320) % 2 == 0);
  uint8_t currentSlot = nameLogic.getCursorSlot();

  if (forceRedraw || !screenInitialized) {
    carrier->display.fillScreen(bgNative);

    // Header Title
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(color565(120, 160, 215));
    carrier->display.setCursor(55, 26);
    carrier->display.print("* GIV ET NAVN *");

    // Subtitle
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(goldColor);
    carrier->display.setCursor(68, 42);
    carrier->display.print("4 BOGSTAVER");

    // Gold divider accent line
    carrier->display.drawFastHLine(44, 58, 152, color565(55, 85, 125));

    // Draw all 4 boxes
    for (uint8_t i = 0; i < NameEntryLogic::MAX_NAME_LENGTH; ++i) {
      char c = nameLogic.getChar(i);
      drawBox(i, c, (i == currentSlot), blinkArrow);
      lastSlots[i] = c;
    }

    // Cute Baby Sprout Preview at Y = 146
    int prevX = 120;
    int prevY = 153;
    // Baby head preview
    carrier->display.fillCircle(prevX, prevY, 8, color565(35, 125, 110));
    carrier->display.fillCircle(prevX, prevY, 7, mintColor);
    // Baby sprout leaf
    carrier->display.fillRect(prevX - 1, prevY - 14, 2, 4, color565(45, 165, 75));
    carrier->display.fillCircle(prevX + 1, prevY - 14, 2, color565(65, 225, 105));
    // Blushing cheeks & eyes
    carrier->display.drawPixel(prevX - 3, prevY - 1, color565(20, 45, 70));
    carrier->display.drawPixel(prevX + 3, prevY - 1, color565(20, 45, 70));
    carrier->display.drawPixel(prevX - 4, prevY + 2, color565(255, 145, 170));
    carrier->display.drawPixel(prevX + 4, prevY + 2, color565(255, 145, 170));

    carrier->display.setTextSize(1);
    carrier->display.setTextColor(mintColor);
    carrier->display.setCursor(56, 168);
    carrier->display.print("KLAEKKES MED DETTE NAVN");

    // Bottom Controller Help Banner
    carrier->display.fillRoundRect(32, 190, 176, 24, 7, color565(16, 24, 44));
    carrier->display.drawRoundRect(32, 190, 176, 24, 7, color565(45, 70, 110));
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(textDim);
    carrier->display.setCursor(38, 198);
    carrier->display.print("T0/1:A-Z | OK:FELT | T4:GEM");

    lastCursorSlot = currentSlot;
    lastArrowBlink = blinkArrow;
    screenInitialized = true;
    return;
  }

  // Delta updates: only redraw boxes that changed character, cursor slot or arrow blink
  bool slotChanged = (currentSlot != lastCursorSlot);
  bool blinkChanged = (blinkArrow != lastArrowBlink);

  for (uint8_t i = 0; i < NameEntryLogic::MAX_NAME_LENGTH; ++i) {
    char c = nameLogic.getChar(i);
    bool charChanged = (c != lastSlots[i]);
    bool isCursor = (i == currentSlot);
    bool wasCursor = (i == lastCursorSlot);

    if (charChanged || (slotChanged && (isCursor || wasCursor)) || (isCursor && blinkChanged)) {
      drawBox(i, c, isCursor, blinkArrow);
      lastSlots[i] = c;
    }
  }

  lastCursorSlot = currentSlot;
  lastArrowBlink = blinkArrow;
}
