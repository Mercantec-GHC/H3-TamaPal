// WHAT: Renders the Solo Wall-Pong minigame on the circular ST7789 display
// WHY:  Delta-rendering (erasing only moving objects) ensures 50+ FPS without screen flicker
// USED BY: TamaPal.ino

#ifndef UI_PONG_H
#define UI_PONG_H

#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "pong_logic.h"

class UiPongRenderer {
public:
  UiPongRenderer();

  void begin(MKRIoTCarrier* hardwareCarrier);

  // Draw initial static court, boundaries, header HUD, and hints
  void drawInitialScreen(const PongLogic& pong);

  // High-performance flicker-free frame update
  void updateDisplay(const PongLogic& pong, bool forceRedraw = false);

  // Draw Game Over overlay with final score, high score, and exit prompt
  void drawGameOverScreen(const PongLogic& pong);

private:
  MKRIoTCarrier* carrier;

  int16_t lastPaddleY;
  int16_t lastBallX;
  int16_t lastBallY;
  uint16_t lastScore;
  uint16_t lastHighScore;
  uint8_t lastLives;
  PongState lastState;
  bool isInitialized;

  void drawHud(const PongLogic& pong, bool force);
  void drawLives(uint8_t lives);
  void drawReadyPrompt();
  void clearReadyPrompt();

  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_PONG_H

