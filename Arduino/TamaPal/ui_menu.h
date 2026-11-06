// WHAT: Graphical renderer for the top menu bar (5 vector icons, sliding frame, Danish text)
// WHY:  Draws only the necessary screen regions (Dirty Rectangles) to eliminate screen flicker
// USED BY: TamaPal.ino

#ifndef UI_MENU_H
#define UI_MENU_H

#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "menu_logic.h"

class UiMenuRenderer {
public:
  UiMenuRenderer();

  // Attach hardware display pointer
  void begin(MKRIoTCarrier* hardwareCarrier);

  // Initial draw of static screen background
  void drawInitialScreen();

  // Draw or update the top menu bar (flicker-free dirty updates)
  void drawMenuBar(const MenuLogic& menu, bool forceRedrawAll);

  // Games selection menu
  void drawGamesMenu(uint8_t selectedGame, uint16_t pongHighScore, uint16_t mazeHighScore);

  // Helper screens
  void drawPlaceholderScreen(const char* title, const char* hint);

private:
  MKRIoTCarrier* carrier;

  float lastDrawnCursorX;
  MenuIcon lastDrawnIcon;
  ScreenType lastDrawnScreen;
  bool isMenuInitialized;

  // Vector drawing helpers for the 5 icons
  void drawIconFood(int centerX, int centerY, bool isSelected);
  void drawIconPlay(int centerX, int centerY, bool isSelected);
  void drawIconScold(int centerX, int centerY, bool isSelected);
  void drawIconStats(int centerX, int centerY, bool isSelected);
  void drawIconConnect(int centerX, int centerY, bool isSelected);

  // Pack 24-bit RGB into 16-bit RGB565 for ST7789 display
  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_MENU_H

