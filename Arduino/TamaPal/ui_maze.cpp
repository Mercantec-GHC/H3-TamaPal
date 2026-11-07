// WHAT: Implementation of procedural Marble Maze rendering on circular ST7789 display
// WHY:  Delta-rendering with animated mini-Lumi ball and interactive items runs at 50+ FPS with 0 flicker
// USED BY: TamaPal.ino

#include "ui_maze.h"
#include <stdio.h>
#include <string.h>

UiMazeRenderer::UiMazeRenderer()
  : carrier(nullptr),
    lastBallX(-1),
    lastBallY(-1),
    lastBerries(255),
    lastCollectedBerries(0),
    lastSeconds(9999),
    lastAnimPhase(255),
    lastState(MAZE_STATE_READY),
    isInitialized(false) {}

void UiMazeRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
  isInitialized = false;
  lastCollectedBerries = 0;
}

void UiMazeRenderer::drawInitialScreen(const MazeLogic& maze) {
  if (!carrier) return;

  uint16_t nativeBg = color565(8, 12, 26);
  carrier->display.fillScreen(nativeBg);

  // Circular stadium bezel guide rings (R = 112..114) matching the round Oplà screen
  carrier->display.drawCircle(120, 120, 114, color565(20, 42, 78));
  carrier->display.drawCircle(120, 120, 112, color565(35, 80, 135));

  // 1. Draw static maze walls and floor tiles
  drawAllWalls(maze);
  drawCellItems(maze);

  // 2. Bottom controls hint (calibrated at Y=196 inside circular bottom arc)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(color565(95, 130, 175), nativeBg);
  const char* hint = "Vip:Rul | T2:Pause";
  int hintW = strlen(hint) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - hintW) / 2, 196);
  carrier->display.print(hint);

  // 3. Reset cached state and draw HUD
  lastBallX   = -1;
  lastBallY   = -1;
  lastBerries = 255;
  lastCollectedBerries = maze.getBerriesCollected();
  lastSeconds = 9999;
  lastState   = maze.getState();
  isInitialized = true;

  drawHud(maze, true);

  // 4. Initial draw of mini-Lumi ball
  drawMiniLumi((int)maze.getBallX(), (int)maze.getBallY(), 0.0f, 0.0f);
  lastBallX = (int16_t)maze.getBallX();
  lastBallY = (int16_t)maze.getBallY();
}

void UiMazeRenderer::drawSingleCellWalls(const MazeLogic& maze, uint8_t c, uint8_t r) {
  if (!carrier || c >= MAZE_COLS || r >= MAZE_ROWS) return;

  uint16_t wallCyan   = color565(0, 190, 220);
  uint16_t wallCore   = color565(120, 240, 255);
  uint16_t wallShadow = color565(4, 18, 38);   // 2.5D ambient drop shadow under walls

  int left = MazeLogic::ORIGIN_X + c * MazeLogic::CELL_SIZE;
  int top  = MazeLogic::ORIGIN_Y + r * MazeLogic::CELL_SIZE;
  uint8_t cell = maze.getCell(c, r);

  // North wall (2px thick + 1px drop shadow)
  if (cell & MAZE_WALL_NORTH) {
    carrier->display.drawFastHLine(left, top, MazeLogic::CELL_SIZE + 1, wallCyan);
    carrier->display.drawFastHLine(left, top + 1, MazeLogic::CELL_SIZE + 1, wallCore);
    carrier->display.drawFastHLine(left + 1, top + 2, MazeLogic::CELL_SIZE - 1, wallShadow);
  }
  // West wall (2px thick + 1px drop shadow)
  if (cell & MAZE_WALL_WEST) {
    carrier->display.drawFastVLine(left, top, MazeLogic::CELL_SIZE + 1, wallCyan);
    carrier->display.drawFastVLine(left + 1, top, MazeLogic::CELL_SIZE + 1, wallCore);
    carrier->display.drawFastVLine(left + 2, top + 1, MazeLogic::CELL_SIZE - 1, wallShadow);
  }
  // East wall (2px thick)
  if (cell & MAZE_WALL_EAST) {
    carrier->display.drawFastVLine(left + MazeLogic::CELL_SIZE, top, MazeLogic::CELL_SIZE + 1, wallCyan);
    carrier->display.drawFastVLine(left + MazeLogic::CELL_SIZE - 1, top, MazeLogic::CELL_SIZE + 1, wallCore);
  }
  // South wall (2px thick)
  if (cell & MAZE_WALL_SOUTH) {
    carrier->display.drawFastHLine(left, top + MazeLogic::CELL_SIZE, MazeLogic::CELL_SIZE + 1, wallCyan);
    carrier->display.drawFastHLine(left, top + MazeLogic::CELL_SIZE - 1, MazeLogic::CELL_SIZE + 1, wallCore);
  }

  // Narrow Pinch Passage (MAZE_ITEM_ICE / MAZE_ITEM_NARROW): inward-protruding stone ledges (3px thick on each side)
  if (cell & MAZE_ITEM_ICE) {
    uint16_t ledgeFill = color565(22, 105, 145);
    uint16_t ledgeEdge = color565(90, 225, 250);
    if ((cell & MAZE_WALL_NORTH) && (cell & MAZE_WALL_SOUTH)) {
      carrier->display.fillRect(left + 2, top + 2, MazeLogic::CELL_SIZE - 3, 2, ledgeFill);
      carrier->display.drawFastHLine(left + 2, top + 4, MazeLogic::CELL_SIZE - 3, ledgeEdge);
      carrier->display.fillRect(left + 2, top + MazeLogic::CELL_SIZE - 3, MazeLogic::CELL_SIZE - 3, 2, ledgeFill);
      carrier->display.drawFastHLine(left + 2, top + MazeLogic::CELL_SIZE - 4, MazeLogic::CELL_SIZE - 3, ledgeEdge);
    } else if ((cell & MAZE_WALL_WEST) && (cell & MAZE_WALL_EAST)) {
      carrier->display.fillRect(left + 2, top + 2, 2, MazeLogic::CELL_SIZE - 3, ledgeFill);
      carrier->display.drawFastVLine(left + 4, top + 2, MazeLogic::CELL_SIZE - 3, ledgeEdge);
      carrier->display.fillRect(left + MazeLogic::CELL_SIZE - 3, top + 2, 2, MazeLogic::CELL_SIZE - 3, ledgeFill);
      carrier->display.drawFastVLine(left + MazeLogic::CELL_SIZE - 4, top + 2, MazeLogic::CELL_SIZE - 3, ledgeEdge);
    }
  }
}

void UiMazeRenderer::drawAllWalls(const MazeLogic& maze) {
  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    for (uint8_t c = 0; c < MAZE_COLS; c++) {
      drawSingleCellWalls(maze, c, r);
    }
  }
}

void UiMazeRenderer::drawSingleCellItems(const MazeLogic& maze, uint8_t c, uint8_t r) {
  if (!carrier || c >= MAZE_COLS || r >= MAZE_ROWS) return;

  uint16_t berryRed     = color565(255, 60, 90);
  uint16_t berryYellow  = color565(255, 220, 60);
  uint16_t berryGreen   = color565(60, 220, 90);
  uint16_t bhHorizon    = color565(195, 60, 255);
  uint16_t bhCore       = color565(2, 2, 6);
  uint16_t portalGold   = color565(255, 215, 50);

  int left = MazeLogic::ORIGIN_X + c * MazeLogic::CELL_SIZE;
  int top  = MazeLogic::ORIGIN_Y + r * MazeLogic::CELL_SIZE;
  int cx   = left + MazeLogic::CELL_SIZE / 2;
  int cy   = top  + MazeLogic::CELL_SIZE / 2;
  uint8_t cell = maze.getCell(c, r);

  // 1. Deku Berry item
  if (cell & MAZE_ITEM_BERRY) {
    carrier->display.fillCircle(cx, cy + 1, 4, berryRed);
    carrier->display.drawPixel(cx - 1, cy, berryYellow);
    carrier->display.drawPixel(cx, cy - 3, berryGreen);
    carrier->display.drawPixel(cx + 1, cy - 4, berryGreen);
  }

  // 2. Cosmic Black Hole (MAZE_ITEM_PITFALL) with pulsating gravitational accretion vortex
  if (cell & MAZE_ITEM_PITFALL) {
    uint8_t pulse = (uint8_t)((millis() / 180) % 3);
    uint16_t outerRing = (pulse == 0) ? color565(110, 30, 175) : ((pulse == 1) ? color565(80, 20, 135) : color565(140, 45, 215));
    carrier->display.drawCircle(cx, cy, 7, outerRing);
    carrier->display.drawCircle(cx, cy, 5, bhHorizon);
    carrier->display.fillCircle(cx, cy, 4, bhCore);
    // Swirling accretion disk particles that rotate with pulse!
    int8_t sOffset[3][4] = {
      { -6, -3,  6,  3 },
      { -3, -6,  3,  6 },
      { -5,  4,  5, -4 }
    };
    carrier->display.drawPixel(cx + sOffset[pulse][0], cy + sOffset[pulse][1], bhHorizon);
    carrier->display.drawPixel(cx + sOffset[pulse][2], cy + sOffset[pulse][3], color565(245, 175, 255));
  }

  // 3. Start marker at (0, 0)
  if (c == 0 && r == 0) {
    carrier->display.drawCircle(cx, cy, 5, color565(30, 80, 50));
  }

  // 4. Magic Exit Portal at (6, 6)
  if (c == MAZE_COLS - 1 && r == MAZE_ROWS - 1) {
    uint8_t pSpin = (uint8_t)((millis() / 200) % 4);
    uint16_t portalRing = (pSpin & 1) ? color565(255, 220, 60) : color565(120, 245, 255);
    carrier->display.drawCircle(cx, cy, 8, portalRing);
    carrier->display.drawCircle(cx, cy, 6, color565(210, 160, 30));
    carrier->display.fillCircle(cx, cy, 4, portalGold);
    carrier->display.drawPixel(cx, cy, 0xFFFF);
    // Twinkling stars at 4 quadrants
    int8_t starX[4] = { 0, 3, 0, -3 };
    int8_t starY[4] = { -3, 0, 3, 0 };
    carrier->display.drawPixel(cx + starX[pSpin], cy + starY[pSpin], 0xFFFF);
  }
}

void UiMazeRenderer::drawCellItems(const MazeLogic& maze) {
  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    for (uint8_t c = 0; c < MAZE_COLS; c++) {
      drawSingleCellItems(maze, c, r);
    }
  }
}

void UiMazeRenderer::repairMazeRegion(const MazeLogic& maze, int16_t centerPxX, int16_t centerPxY) {
  if (!carrier) return;

  int minC = (centerPxX - 6 - MazeLogic::ORIGIN_X) / MazeLogic::CELL_SIZE;
  int maxC = (centerPxX + 6 - MazeLogic::ORIGIN_X) / MazeLogic::CELL_SIZE;
  int minR = (centerPxY - 9 - MazeLogic::ORIGIN_Y) / MazeLogic::CELL_SIZE;
  int maxR = (centerPxY + 6 - MazeLogic::ORIGIN_Y) / MazeLogic::CELL_SIZE;

  if (minC < 0) minC = 0;
  if (maxC >= MAZE_COLS) maxC = MAZE_COLS - 1;
  if (minR < 0) minR = 0;
  if (maxR >= MAZE_ROWS) maxR = MAZE_ROWS - 1;

  for (int r = minR; r <= maxR; r++) {
    for (int c = minC; c <= maxC; c++) {
      drawSingleCellWalls(maze, (uint8_t)c, (uint8_t)r);
      drawSingleCellItems(maze, (uint8_t)c, (uint8_t)r);
    }
  }
}

void UiMazeRenderer::drawHud(const MazeLogic& maze, bool force) {
  if (!carrier) return;

  uint16_t nativeBg   = color565(8, 12, 26);
  uint16_t berryPink  = color565(255, 110, 140);
  uint16_t timeCyan   = color565(210, 245, 255);
  uint16_t goldRecord = color565(255, 215, 60);

  // 1. Berries counter (Left, calibrated at X=58, Y=31 inside top circular arc)
  if (force || maze.getBerriesCollected() != lastBerries) {
    carrier->display.fillRect(56, 29, 40, 12, nativeBg);
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(berryPink, nativeBg);
    carrier->display.setCursor(58, 31);
    char buf[16];
    snprintf(buf, sizeof(buf), "*%u/%u", maze.getBerriesCollected(), maze.getTotalBerries());
    carrier->display.print(buf);
    lastBerries = maze.getBerriesCollected();
  }

  // 2. Elapsed Timer (Center, X=102, Y=31)
  uint16_t currentSec = maze.getElapsedSeconds();
  if (force || currentSec != lastSeconds) {
    carrier->display.fillRect(98, 29, 44, 12, nativeBg);
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(timeCyan, nativeBg);
    carrier->display.setCursor(102, 31);
    char buf[16];
    uint16_t m = currentSec / 60;
    uint16_t s = currentSec % 60;
    snprintf(buf, sizeof(buf), "%02u:%02u", m, s);
    carrier->display.print(buf);
    lastSeconds = currentSec;
  }

  // 3. Best record (Right, X=148, Y=31 -> ends at X=172, safely inside circular bezel)
  if (force) {
    carrier->display.fillRect(146, 29, 36, 12, nativeBg);
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(goldRecord, nativeBg);
    carrier->display.setCursor(148, 31);
    char buf[16];
    if (maze.getBestSeconds() > 0) {
      snprintf(buf, sizeof(buf), "%us", maze.getBestSeconds());
    } else {
      snprintf(buf, sizeof(buf), "--s");
    }
    carrier->display.print(buf);
  }
}

void UiMazeRenderer::animateHazardsAndPortal(const MazeLogic& maze, uint8_t phase) {
  if (!carrier) return;
  (void)phase;

  int ballC = ((int)maze.getBallX() - MazeLogic::ORIGIN_X) / MazeLogic::CELL_SIZE;
  int ballR = ((int)maze.getBallY() - MazeLogic::ORIGIN_Y) / MazeLogic::CELL_SIZE;

  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    for (uint8_t c = 0; c < MAZE_COLS; c++) {
      uint8_t cell = maze.getCell(c, r);
      bool isPitfall = (cell & MAZE_ITEM_PITFALL);
      bool isPortal  = (c == MAZE_COLS - 1 && r == MAZE_ROWS - 1);

      if (isPitfall || isPortal) {
        // Skip redraw if ball is currently inside this cell to prevent flickering
        if (c == ballC && r == ballR) continue;
        drawSingleCellItems(maze, c, r);
      }
    }
  }
}

void UiMazeRenderer::drawMiniLumi(int x, int y, float vx, float vy) {
  if (!carrier) return;

  uint16_t lumiMint   = color565(120, 245, 150);
  uint16_t lumiBorder = color565(35, 165, 85);
  uint16_t sproutGold = color565(255, 220, 60);
  uint16_t blushPink  = color565(255, 120, 160);

  // Mint-green mochi body (radius 4)
  carrier->display.fillCircle(x, y, 4, lumiMint);
  carrier->display.drawCircle(x, y, 4, lumiBorder);

  // Cute tiny spirit ears on sides
  carrier->display.drawPixel(x - 4, y - 3, lumiBorder);
  carrier->display.drawPixel(x + 4, y - 3, lumiBorder);

  // Golden Sprout on top: sweeps back against movement velocity
  int sproutDx = (vx > 0.4f) ? -1 : ((vx < -0.4f) ? 1 : 0);
  int sproutDy = (vy > 0.4f) ? -1 : 0;
  carrier->display.drawFastVLine(x, y - 6, 2, sproutGold);
  carrier->display.drawPixel(x + sproutDx, y - 7 + sproutDy, sproutGold);
  carrier->display.drawPixel(x + sproutDx * 2, y - 8 + sproutDy, color565(255, 245, 130));

  // Determine emotional state: speed > 0.45 or rolling fast = Panicked/Shocked expression!
  float speedSq = (vx * vx) + (vy * vy);
  bool isShocked = (speedSq > 0.35f);

  if (isShocked) {
    // SHOCKED / PANICKED: Wide white eyes with tiny dilated pupils & open mouth ":O"
    carrier->display.drawPixel(x - 2, y - 2, 0xFFFF);
    carrier->display.drawPixel(x - 2, y - 1, 0xFFFF);
    carrier->display.drawPixel(x + 2, y - 2, 0xFFFF);
    carrier->display.drawPixel(x + 2, y - 1, 0xFFFF);

    int eyeDx = (vx > 0.2f) ? 1 : ((vx < -0.2f) ? -1 : 0);
    int eyeDy = (vy > 0.2f) ? 1 : ((vy < -0.2f) ? -1 : 0);
    carrier->display.drawPixel(x - 2 + eyeDx, y - 1 + eyeDy, 0x0000);
    carrier->display.drawPixel(x + 2 + eyeDx, y - 1 + eyeDy, 0x0000);

    // Open gasping mouth in the center
    carrier->display.drawPixel(x, y + 1, 0x0000);
    carrier->display.drawPixel(x, y + 2, color565(255, 70, 90));

    // Rosy panic blush
    carrier->display.drawPixel(x - 3, y + 1, blushPink);
    carrier->display.drawPixel(x + 3, y + 1, blushPink);
  } else {
    // CALM / FOCUSED: Eyes glance in rolling direction with shiny gleam
    int eyeDx = (vx > 0.2f) ? 1 : ((vx < -0.2f) ? -1 : 0);
    int eyeDy = (vy > 0.2f) ? 1 : ((vy < -0.2f) ? -1 : 0);

    carrier->display.drawPixel(x - 2 + eyeDx, y - 1 + eyeDy, 0x0000);
    carrier->display.drawPixel(x + 2 + eyeDx, y - 1 + eyeDy, 0x0000);
    carrier->display.drawPixel(x - 2 + eyeDx, y - 2 + eyeDy, 0xFFFF); // gleam
    carrier->display.drawPixel(x + 2 + eyeDx, y - 2 + eyeDy, 0xFFFF);

    // Cute tiny smile
    carrier->display.drawPixel(x, y + 1, 0x0000);
    carrier->display.drawPixel(x - 3, y, blushPink);
    carrier->display.drawPixel(x + 3, y, blushPink);
  }
}

void UiMazeRenderer::eraseMiniLumi(int x, int y, const MazeLogic& maze) {
  (void)maze;
  if (!carrier) return;

  uint16_t nativeBg = color565(8, 12, 26);
  // Clear 13x16 box covering ball, ears, and sprout with base floor color
  carrier->display.fillRect(x - 6, y - 9, 13, 16, nativeBg);
}

void UiMazeRenderer::drawVictoryScreen(const MazeLogic& maze) {
  if (!carrier) return;

  uint16_t modalBg   = color565(14, 24, 48);
  uint16_t goldBorder= color565(255, 215, 60);
  uint16_t textWhite = 0xFFFF;
  uint16_t cyanHint  = color565(80, 220, 255);

  carrier->display.fillRoundRect(34, 62, 172, 112, 8, modalBg);
  carrier->display.drawRoundRect(34, 62, 172, 112, 8, goldBorder);

  // 1. Victory Header
  carrier->display.setTextSize(2);
  carrier->display.setTextColor(goldBorder, modalBg);
  const char* h = "KLARET!";
  int hw = strlen(h) * 12;
  carrier->display.setCursor((SCREEN_WIDTH - hw) / 2, 72);
  carrier->display.print(h);

  // 2. Time & Berries
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(textWhite, modalBg);
  char buf[32];
  snprintf(buf, sizeof(buf), "Tid: %u sek | Baer: %u/%u",
           maze.getElapsedSeconds(), maze.getBerriesCollected(), maze.getTotalBerries());
  int sw = strlen(buf) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - sw) / 2, 98);
  carrier->display.print(buf);

  // 3. Record Announcement
  if (maze.isNewBestRecord()) {
    carrier->display.setTextColor(goldBorder, modalBg);
    const char* rec = "* NY REKORD! *";
    int rw = strlen(rec) * 6;
    carrier->display.setCursor((SCREEN_WIDTH - rw) / 2, 118);
    carrier->display.print(rec);
  } else {
    carrier->display.setTextColor(goldBorder, modalBg);
    snprintf(buf, sizeof(buf), "Bedste: %u sek", maze.getBestSeconds());
    int bw = strlen(buf) * 6;
    carrier->display.setCursor((SCREEN_WIDTH - bw) / 2, 118);
    carrier->display.print(buf);
  }

  // 4. Replay / Dismiss Prompt
  carrier->display.setTextColor(cyanHint, modalBg);
  const char* sub = "T2:Ny bane | T3:Menu";
  int subW = strlen(sub) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - subW) / 2, 146);
  carrier->display.print(sub);
}

void UiMazeRenderer::drawPitfallSplash(float ballX, float ballY) {
  if (!carrier) return;
  uint16_t splashPurple = color565(180, 50, 220);
  int bx = (int)ballX;
  int by = (int)ballY;
  carrier->display.drawCircle(bx, by, 7, splashPurple);
  carrier->display.drawCircle(bx, by, 9, color565(120, 30, 160));
}

void UiMazeRenderer::updateDisplay(const MazeLogic& maze, bool forceRedraw) {
  if (!carrier) return;

  if (forceRedraw || !isInitialized) {
    drawInitialScreen(maze);
  }

  uint16_t nativeBg = color565(8, 12, 26);
  MazeState currentState = maze.getState();
  if (currentState != lastState) {
    if (currentState == MAZE_STATE_VICTORY) {
      drawVictoryScreen(maze);
      lastState = currentState;
      return;
    }
    if (currentState == MAZE_STATE_PAUSED) {
      carrier->display.fillRect(98, 29, 44, 12, nativeBg);
      carrier->display.setTextSize(1);
      carrier->display.setTextColor(color565(255, 215, 60), nativeBg);
      carrier->display.setCursor(102, 31);
      carrier->display.print("PAUSE");

      // Update bottom hint while paused: T2:Fortsaet | T3:Afslut
      carrier->display.fillRect(30, 194, 180, 14, nativeBg);
      carrier->display.setTextColor(color565(255, 215, 60), nativeBg);
      const char* pHint = "T2:Fortsaet | T3:Afslut";
      int pW = strlen(pHint) * 6;
      carrier->display.setCursor((SCREEN_WIDTH - pW) / 2, 196);
      carrier->display.print(pHint);
    } else if (lastState == MAZE_STATE_PAUSED) {
      lastSeconds = 9999; // Force timer redraw on resume
      drawHud(maze, false);

      // Restore bottom hint on resume: Vip:Rul | T2:Pause
      carrier->display.fillRect(30, 194, 180, 14, nativeBg);
      carrier->display.setTextColor(color565(95, 130, 175), nativeBg);
      const char* rHint = "Vip:Rul | T2:Pause";
      int rW = strlen(rHint) * 6;
      carrier->display.setCursor((SCREEN_WIDTH - rW) / 2, 196);
      carrier->display.print(rHint);
    }
    if (currentState == MAZE_STATE_PITFALL_FALL) {
      drawPitfallSplash(maze.getBallX(), maze.getBallY());
    }
    if (lastState == MAZE_STATE_PITFALL_FALL && currentState != MAZE_STATE_PITFALL_FALL) {
      if (lastBallX != -1 && lastBallY != -1) {
        carrier->display.fillRect(lastBallX - 10, lastBallY - 10, 21, 21, nativeBg);
        int pitC = (lastBallX - MazeLogic::ORIGIN_X) / MazeLogic::CELL_SIZE;
        int pitR = (lastBallY - MazeLogic::ORIGIN_Y) / MazeLogic::CELL_SIZE;
        for (int pr = pitR - 1; pr <= pitR + 1; pr++) {
          if (pr < 0 || pr >= MAZE_ROWS) continue;
          for (int pc = pitC - 1; pc <= pitC + 1; pc++) {
            if (pc < 0 || pc >= MAZE_COLS) continue;
            drawSingleCellWalls(maze, (uint8_t)pc, (uint8_t)pr);
            drawSingleCellItems(maze, (uint8_t)pc, (uint8_t)pr);
          }
        }
        lastBallX = -1;
        lastBallY = -1;
      }
    }
    lastState = currentState;
  }

  if (currentState == MAZE_STATE_VICTORY) {
    return; // Don't delta-render behind modal
  }

  // 1. Berry collection cleanup
  if (maze.getBerriesCollected() != lastCollectedBerries) {
    lastCollectedBerries = maze.getBerriesCollected();
    int bcX = (int)maze.getBallX();
    int bcY = (int)maze.getBallY();
    int curC = (bcX - MazeLogic::ORIGIN_X) / MazeLogic::CELL_SIZE;
    int curR = (bcY - MazeLogic::ORIGIN_Y) / MazeLogic::CELL_SIZE;
    if (curC >= 0 && curC < MAZE_COLS && curR >= 0 && curR < MAZE_ROWS) {
      int cellCx = MazeLogic::ORIGIN_X + curC * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE / 2;
      int cellCy = MazeLogic::ORIGIN_Y + curR * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE / 2;
      carrier->display.fillRect(cellCx - 5, cellCy - 5, 11, 11, nativeBg);
      repairMazeRegion(maze, cellCx, cellCy);
      lastBallX = -1;
      lastBallY = -1;
    }
  }

  // 2. HUD Updates (skip while paused so "PAUSE" badge stays visible)
  if (currentState != MAZE_STATE_PAUSED) {
    drawHud(maze, false);
  }

  // 3. Periodic pulsating animation of black hole vortex and exit portal
  if (currentState == MAZE_STATE_PLAYING || currentState == MAZE_STATE_READY) {
    uint8_t animPhase = (uint8_t)((millis() / 220) % 4);
    if (animPhase != lastAnimPhase) {
      lastAnimPhase = animPhase;
      animateHazardsAndPortal(maze, animPhase);
    }
  }

  // 4. Ball Delta-Rendering
  int16_t newX = (int16_t)maze.getBallX();
  int16_t newY = (int16_t)maze.getBallY();

  if (newX != lastBallX || newY != lastBallY) {
    if (lastBallX != -1 && lastBallY != -1) {
      eraseMiniLumi(lastBallX, lastBallY, maze);
      repairMazeRegion(maze, lastBallX, lastBallY);
    }
    drawMiniLumi(newX, newY, maze.getBallVx(), maze.getBallVy());
    lastBallX = newX;
    lastBallY = newY;
  }
}

