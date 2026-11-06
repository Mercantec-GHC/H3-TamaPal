// WHAT: Renders the procedural Marble Maze minigame on the circular ST7789 display
// WHY:  Delta-rendering with animated mini-Lumi ball and interactive items runs at 50+ FPS with 0 flicker
// USED BY: TamaPal.ino

#ifndef UI_MAZE_H
#define UI_MAZE_H

#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "maze_logic.h"

class UiMazeRenderer {
public:
  UiMazeRenderer();

  void begin(MKRIoTCarrier* hardwareCarrier);

  // Draw initial static maze walls, items, portal, and HUD
  void drawInitialScreen(const MazeLogic& maze);

  // Flicker-free delta-update for rolling mini-Lumi ball and timer
  void updateDisplay(const MazeLogic& maze, bool forceRedraw = false);

  // Victory celebration modal
  void drawVictoryScreen(const MazeLogic& maze);

  // Pitfall splash effect
  void drawPitfallSplash(float ballX, float ballY);

private:
  MKRIoTCarrier* carrier;

  int16_t lastBallX;
  int16_t lastBallY;
  uint8_t lastBerries;
  uint16_t lastSeconds;
  MazeState lastState;
  bool isInitialized;

  void drawAllWalls(const MazeLogic& maze);
  void drawCellItems(const MazeLogic& maze);
  void drawHud(const MazeLogic& maze, bool force);
  void drawMiniLumi(int x, int y, float vx, float vy);
  void eraseMiniLumi(int x, int y, const MazeLogic& maze);

  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_MAZE_H

