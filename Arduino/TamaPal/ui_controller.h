// WHAT: Renders the Pong / minigame controller screen on the round ST7789 display
// WHY:  Provides instant visual feedback for paddle position, tilt, button, and network status
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

  // Draw the initial static controller UI (borders, header, paddle slider track, button hints)
  void drawInitialScreen();

  // Update real-time paddle position and network status badge without screen flickering
  void updateDisplay(const TiltLogic& tilt, bool isMqttConnected, bool forceFullRedraw = false);

private:
  MKRIoTCarrier* carrier;
  int lastPaddleY;
  bool lastNetworkStatus;
  bool lastButtonVisual;
  bool isInitialized;

  // Helper to convert 24-bit RGB to 16-bit 565 format
  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_CONTROLLER_H

