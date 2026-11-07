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

  // Trigger wall ricochet sparks (top/bottom walls or bot paddle)
  void triggerWallSparks(float ballX, float ballY, float ballVx, float ballVy);

  // Trigger electric cyan sparks on player paddle hit
  void triggerPaddleSparks(float ballX, float ballY);

  // Trigger Lumi cheerleader celebration jump when player hits the ball
  void triggerCheer();

  // Draw Game Over overlay with final score, high score, and exit prompt
  void drawGameOverScreen(const PongLogic& pong);

private:
  struct PongSpark {
    int16_t x;
    int16_t y;
    int16_t prevX;
    int16_t prevY;
    int8_t vx;
    int8_t vy;
    uint8_t life;
    uint16_t color;
  };

  static constexpr uint8_t MAX_SPARKS = 6;
  static constexpr uint8_t TRAIL_LEN  = 3;

  MKRIoTCarrier* carrier;

  int16_t lastPaddleY;
  int16_t lastBotY;
  int16_t lastBallX;
  int16_t lastBallY;
  int16_t trailX[TRAIL_LEN];
  int16_t trailY[TRAIL_LEN];
  PongSpark sparks[MAX_SPARKS];
  uint8_t cheerTimer;
  uint8_t lastCheerMood;
  uint8_t lastCheerAnimTick;
  uint16_t lastScore;
  uint16_t lastHighScore;
  uint8_t lastLives;
  uint8_t scoreFlashTimer;
  uint8_t missFlashTimer;
  PongState lastState;
  bool isInitialized;

  void drawHud(const PongLogic& pong, bool force);
  void drawLives(uint8_t lives);
  void drawReadyPrompt();
  void clearReadyPrompt();
  void drawCheerleaderLumi(const PongLogic& pong, bool force);
  void updateSparks(bool& mascotOverlapped);
  void clearFireballAndTrail();

  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_PONG_H

