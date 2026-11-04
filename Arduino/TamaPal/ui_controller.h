// WHAT: Renders the universal controller screen (Pong buttons + Labyrinth 2D tilt) on ST7789 display
// WHY:  Provides instant visual feedback for buttons, 2D tilt ball, and MQTT network state with zero flicker
// USED BY: TamaPal.ino

#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "tilt_logic.h"

class UiControllerRenderer {
public:
  UiControllerRenderer();

  void begin(MKRIoTCarrier* hardwareCarrier);

  // Draw the initial static controller UI (arena, button boxes, header, hints)
  void drawInitialScreen(int playerId = 1);

  // Update real-time 2D tilt ball, button highlights, and network badge without screen flickering
  void updateDisplay(const TiltLogic& tilt, bool isMqttConnected, bool forceFullRedraw = false);

private:
  MKRIoTCarrier* carrier;
  int lastBallX;
  int lastBallY;
  bool lastNetworkStatus;
  bool lastBtnUp;
  bool lastBtnDown;
  bool lastBtnAction;
  bool isInitialized;

  void drawButtonUp(bool pressed);
  void drawButtonDown(bool pressed);
  void drawButtonAction(bool pressed);
  void redrawArenaCrosshair(int x, int y);

  // Helper to convert 24-bit RGB to 16-bit 565 format
  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_CONTROLLER_H
