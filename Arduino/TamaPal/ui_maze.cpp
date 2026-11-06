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

  // 1. Draw static maze walls and floor tiles
  drawAllWalls(maze);
  drawCellItems(maze);

  // 2. Bottom controls hint
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(color565(95, 130, 175), nativeBg);
  const char* hint = "Haeld for at rulle | T3:Exit";
  int hintW = strlen(hint) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - hintW) / 2, 214);
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

  uint16_t wallCyan = color565(0, 190, 220);
  uint16_t wallCore = color565(120, 240, 255);

  int left = MazeLogic::ORIGIN_X + c * MazeLogic::CELL_SIZE;
  int top  = MazeLogic::ORIGIN_Y + r * MazeLogic::CELL_SIZE;
  uint8_t cell = maze.getCell(c, r);

  // North wall (2px thick)
  if (cell & MAZE_WALL_NORTH) {
    carrier->display.drawFastHLine(left, top, MazeLogic::CELL_SIZE + 1, wallCyan);
    carrier->display.drawFastHLine(left, top + 1, MazeLogic::CELL_SIZE + 1, wallCore);
  }
  // West wall (2px thick)
  if (cell & MAZE_WALL_WEST) {
    carrier->display.drawFastVLine(left, top, MazeLogic::CELL_SIZE + 1, wallCyan);
    carrier->display.drawFastVLine(left + 1, top, MazeLogic::CELL_SIZE + 1, wallCore);
  }
  // East wall
  if (cell & MAZE_WALL_EAST) {
    carrier->display.drawFastVLine(left + MazeLogic::CELL_SIZE, top, MazeLogic::CELL_SIZE + 1, wallCyan);
  }
  // South wall
  if (cell & MAZE_WALL_SOUTH) {
    carrier->display.drawFastHLine(left, top + MazeLogic::CELL_SIZE, MazeLogic::CELL_SIZE + 1, wallCyan);
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

  uint16_t iceGlint     = color565(180, 225, 255);
  uint16_t berryRed     = color565(255, 60, 90);
  uint16_t berryYellow  = color565(255, 220, 60);
  uint16_t berryGreen   = color565(60, 220, 90);
  uint16_t pitfallRing  = color565(60, 15, 80);
  uint16_t pitfallHole  = color565(5, 5, 12);
  uint16_t portalGold   = color565(255, 215, 50);

  int left = MazeLogic::ORIGIN_X + c * MazeLogic::CELL_SIZE;
  int top  = MazeLogic::ORIGIN_Y + r * MazeLogic::CELL_SIZE;
  int cx   = left + MazeLogic::CELL_SIZE / 2;
  int cy   = top  + MazeLogic::CELL_SIZE / 2;
  uint8_t cell = maze.getCell(c, r);

  // 1. Ice Patch floor
  if (cell & MAZE_ITEM_ICE) {
    carrier->display.drawPixel(left + 6, top + 6, iceGlint);
    carrier->display.drawPixel(left + 16, top + 15, iceGlint);
  }

  // 2. Deku Berry item
  if (cell & MAZE_ITEM_BERRY) {
    carrier->display.fillCircle(cx, cy + 1, 4, berryRed);
    carrier->display.drawPixel(cx - 1, cy, berryYellow);
    carrier->display.drawPixel(cx, cy - 3, berryGreen);
    carrier->display.drawPixel(cx + 1, cy - 4, berryGreen);
  }

  // 3. Pitfall trap
  if (cell & MAZE_ITEM_PITFALL) {
    carrier->display.drawCircle(cx, cy, 6, pitfallRing);
    carrier->display.fillCircle(cx, cy, 4, pitfallHole);
    carrier->display.drawPixel(cx - 1, cy - 1, color565(140, 60, 190));
  }

  // 4. Start marker at (0, 0)
  if (c == 0 && r == 0) {
    carrier->display.drawCircle(cx, cy, 5, color565(30, 80, 50));
  }

  // 5. Exit Portal at (6, 6)
  if (c == MAZE_COLS - 1 && r == MAZE_ROWS - 1) {
    carrier->display.drawCircle(cx, cy, 8, color565(160, 130, 20));
    carrier->display.fillCircle(cx, cy, 5, portalGold);
    carrier->display.drawPixel(cx, cy, 0xFFFF);
    carrier->display.drawPixel(cx - 1, cy - 1, 0xFFFF);
  }
}

void UiMazeRenderer::drawCellItems(const MazeLogic& maze) {
  uint16_t iceBg = color565(18, 44, 82);
  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    for (uint8_t c = 0; c < MAZE_COLS; c++) {
      uint8_t cell = maze.getCell(c, r);
      if (cell & MAZE_ITEM_ICE) {
        int left = MazeLogic::ORIGIN_X + c * MazeLogic::CELL_SIZE;
        int top  = MazeLogic::ORIGIN_Y + r * MazeLogic::CELL_SIZE;
        carrier->display.fillRect(left + 2, top + 2, MazeLogic::CELL_SIZE - 3, MazeLogic::CELL_SIZE - 3, iceBg);
      }
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

  uint16_t iceBg = color565(18, 44, 82);

  for (int r = minR; r <= maxR; r++) {
    for (int c = minC; c <= maxC; c++) {
      int left = MazeLogic::ORIGIN_X + c * MazeLogic::CELL_SIZE;
      int top  = MazeLogic::ORIGIN_Y + r * MazeLogic::CELL_SIZE;

      if (maze.getCell(c, r) & MAZE_ITEM_ICE) {
        int iceLeft   = left + 2;
        int iceTop    = top + 2;
        int iceRight  = left + MazeLogic::CELL_SIZE - 2;
        int iceBottom = top + MazeLogic::CELL_SIZE - 2;

        int clipL = (centerPxX - 5 > iceLeft) ? (centerPxX - 5) : iceLeft;
        int clipT = (centerPxY - 8 > iceTop) ? (centerPxY - 8) : iceTop;
        int clipR = (centerPxX + 5 < iceRight) ? (centerPxX + 5) : iceRight;
        int clipB = (centerPxY + 5 < iceBottom) ? (centerPxY + 5) : iceBottom;

        if (clipR >= clipL && clipB >= clipT) {
          carrier->display.fillRect(clipL, clipT, clipR - clipL + 1, clipB - clipT + 1, iceBg);
        }
      }

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

  // 1. Berries counter (Left, Y = 18)
  if (force || maze.getBerriesCollected() != lastBerries) {
    carrier->display.fillRect(28, 16, 56, 12, nativeBg);
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(berryPink, nativeBg);
    carrier->display.setCursor(28, 18);
    char buf[16];
    snprintf(buf, sizeof(buf), "* %u/%u", maze.getBerriesCollected(), maze.getTotalBerries());
    carrier->display.print(buf);
    lastBerries = maze.getBerriesCollected();
  }

  // 2. Elapsed Timer (Center, Y = 18)
  uint16_t currentSec = maze.getElapsedSeconds();
  if (force || currentSec != lastSeconds) {
    carrier->display.fillRect(94, 16, 52, 12, nativeBg);
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(timeCyan, nativeBg);
    carrier->display.setCursor(96, 18);
    char buf[16];
    uint16_t m = currentSec / 60;
    uint16_t s = currentSec % 60;
    snprintf(buf, sizeof(buf), "%02u:%02u", m, s);
    carrier->display.print(buf);
    lastSeconds = currentSec;
  }

  // 3. Best record (Right, Y = 18)
  if (force) {
    carrier->display.fillRect(158, 16, 54, 12, nativeBg);
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(goldRecord, nativeBg);
    carrier->display.setCursor(158, 18);
    char buf[16];
    if (maze.getBestSeconds() > 0) {
      snprintf(buf, sizeof(buf), "%us", maze.getBestSeconds());
    } else {
      snprintf(buf, sizeof(buf), "--s");
    }
    carrier->display.print(buf);
  }
}

void UiMazeRenderer::drawMiniLumi(int x, int y, float vx, float vy) {
  if (!carrier) return;

  uint16_t lumiMint   = color565(120, 245, 150);
  uint16_t lumiBorder = color565(45, 175, 95);
  uint16_t sproutGold = color565(255, 220, 60);

  // Mint-green mochi body (radius 4)
  carrier->display.fillCircle(x, y, 4, lumiMint);
  carrier->display.drawCircle(x, y, 4, lumiBorder);

  // Golden Sprout on top
  carrier->display.drawFastVLine(x, y - 6, 2, sproutGold);
  carrier->display.drawPixel(x - 1, y - 6, sproutGold);
  carrier->display.drawPixel(x + 1, y - 7, color565(255, 240, 120));

  // Eyes looking in motion direction
  int eyeDx = (vx > 0.35f) ? 1 : ((vx < -0.35f) ? -1 : 0);
  int eyeDy = (vy > 0.35f) ? 1 : ((vy < -0.35f) ? -1 : 0);

  carrier->display.drawPixel(x - 2 + eyeDx, y - 1 + eyeDy, 0x0000);
  carrier->display.drawPixel(x + 2 + eyeDx, y - 1 + eyeDy, 0x0000);
  carrier->display.drawPixel(x - 2 + eyeDx, y - 2 + eyeDy, 0xFFFF); // gleam
  carrier->display.drawPixel(x + 2 + eyeDx, y - 2 + eyeDy, 0xFFFF);
}

void UiMazeRenderer::eraseMiniLumi(int x, int y, const MazeLogic& maze) {
  (void)maze;
  if (!carrier) return;

  uint16_t nativeBg = color565(8, 12, 26);
  // Clear 11x14 box covering ball and sprout with base floor color
  carrier->display.fillRect(x - 5, y - 8, 11, 14, nativeBg);
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

  // 4. Dismiss Prompt
  carrier->display.setTextColor(cyanHint, modalBg);
  const char* sub = "Tryk OK / T3: Menu";
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
    if (currentState == MAZE_STATE_PITFALL_FALL) {
      drawPitfallSplash(maze.getBallX(), maze.getBallY());
    }
    if (lastState == MAZE_STATE_PITFALL_FALL && currentState != MAZE_STATE_PITFALL_FALL) {
      if (lastBallX != -1 && lastBallY != -1) {
        carrier->display.fillRect(lastBallX - 10, lastBallY - 10, 21, 21, nativeBg);
        int pitC = (lastBallX - MazeLogic::ORIGIN_X) / MazeLogic::CELL_SIZE;
        int pitR = (lastBallY - MazeLogic::ORIGIN_Y) / MazeLogic::CELL_SIZE;
        uint16_t iceBg = color565(18, 44, 82);
        for (int pr = pitR - 1; pr <= pitR + 1; pr++) {
          if (pr < 0 || pr >= MAZE_ROWS) continue;
          for (int pc = pitC - 1; pc <= pitC + 1; pc++) {
            if (pc < 0 || pc >= MAZE_COLS) continue;
            if (maze.getCell((uint8_t)pc, (uint8_t)pr) & MAZE_ITEM_ICE) {
              int iLeft = MazeLogic::ORIGIN_X + pc * MazeLogic::CELL_SIZE;
              int iTop  = MazeLogic::ORIGIN_Y + pr * MazeLogic::CELL_SIZE;
              carrier->display.fillRect(iLeft + 2, iTop + 2, MazeLogic::CELL_SIZE - 3, MazeLogic::CELL_SIZE - 3, iceBg);
            }
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

  // 2. HUD Updates
  drawHud(maze, false);

  // 3. Ball Delta-Rendering
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

