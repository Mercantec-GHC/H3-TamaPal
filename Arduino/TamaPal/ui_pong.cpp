// WHAT: Implementation of Solo Wall-Pong rendering on the circular ST7789 display
// WHY:  Zero-flicker delta-rendering maintains 50+ FPS with minimal CPU overhead
// USED BY: TamaPal.ino

#include "ui_pong.h"
#include <stdio.h>
#include <string.h>

UiPongRenderer::UiPongRenderer()
  : carrier(nullptr),
    lastPaddleY(-1),
    lastBallX(-1),
    lastBallY(-1),
    lastScore(9999),
    lastHighScore(9999),
    lastLives(255),
    lastState(PONG_STATE_READY),
    isInitialized(false) {}

void UiPongRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
  isInitialized = false;
}

void UiPongRenderer::drawInitialScreen(const PongLogic& pong) {
  if (!carrier) return;

  uint16_t nativeBg     = color565(8, 12, 26);     // Dark seamless midnight background
  uint16_t borderCyan   = color565(0, 200, 255);   // Neon cyan boundary walls
  uint16_t bumperGold   = color565(255, 180, 50);  // Neon gold right bumper wall
  uint16_t hintColor    = color565(95, 130, 175);  // Subtle navigation text

  // 1. Clear display and draw circular stadium bezel ring (R = 112..114)
  carrier->display.fillScreen(nativeBg);
  carrier->display.drawCircle(120, 120, 114, color565(20, 42, 78));
  carrier->display.drawCircle(120, 120, 112, color565(35, 80, 135));

  // 2. Court walls (calibrated inside circular viewport: X=42..198, Y=52..188)
  // Top neon rail
  carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_TOP,
                                PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderCyan);
  carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_TOP - 1,
                                PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, color565(0, 90, 140));

  // Bottom neon rail
  carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_BOTTOM,
                                PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderCyan);
  carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_BOTTOM + 1,
                                PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, color565(0, 90, 140));

  // Right solid neon bounce wall
  carrier->display.fillRect(PongLogic::FIELD_RIGHT, PongLogic::FIELD_TOP,
                            3, PongLogic::FIELD_BOTTOM - PongLogic::FIELD_TOP + 1, bumperGold);
  carrier->display.drawFastVLine(PongLogic::FIELD_RIGHT + 1, PongLogic::FIELD_TOP + 2,
                                PongLogic::FIELD_BOTTOM - PongLogic::FIELD_TOP - 3, 0xFFFF);

  // Left danger / goal line (subtle dotted red line)
  for (int y = PongLogic::FIELD_TOP + 2; y < PongLogic::FIELD_BOTTOM; y += 4) {
    carrier->display.drawPixel(PongLogic::FIELD_LEFT, y, color565(180, 50, 70));
  }

  // 3. Bottom controls hint (calibrated at Y=196 inside circular bottom arc)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(hintColor, nativeBg);
  const char* hint = "T0:Op T1:Ned T2:OK T3:Ud";
  int hintW = strlen(hint) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - hintW) / 2, 196);
  carrier->display.print(hint);

  // 4. Force redraw of HUD, paddle, and ball
  lastPaddleY   = -1;
  lastBallX     = -1;
  lastBallY     = -1;
  lastScore     = 9999;
  lastHighScore = 9999;
  lastLives     = 255;
  lastState     = pong.getState();
  isInitialized = true;

  drawHud(pong, true);

  if (pong.getState() == PONG_STATE_READY) {
    drawReadyPrompt();
  }
}

void UiPongRenderer::drawHud(const PongLogic& pong, bool force) {
  if (!carrier) return;

  uint16_t nativeBg  = color565(8, 12, 26);
  uint16_t scoreCyan = color565(210, 245, 255);
  uint16_t highGold  = color565(255, 215, 60);

  // 1. Lives (Left, calibrated at X=58..78, Y=35 inside top circular arc)
  if (force || pong.getLives() != lastLives) {
    drawLives(pong.getLives());
    lastLives = pong.getLives();
  }

  // 2. Score (Center, Y=32)
  if (force || pong.getScore() != lastScore) {
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(scoreCyan, nativeBg);
    carrier->display.setCursor(92, 32);
    char buf[16];
    snprintf(buf, sizeof(buf), "SCORE:%02u", pong.getScore());
    carrier->display.print(buf);
    lastScore = pong.getScore();
  }

  // 3. High Score (Right, X=148, Y=32 -> ends at X=178, safely inside circular bezel)
  if (force || pong.getHighScore() != lastHighScore) {
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(highGold, nativeBg);
    carrier->display.setCursor(148, 32);
    char buf[16];
    snprintf(buf, sizeof(buf), "HI:%02u", pong.getHighScore());
    carrier->display.print(buf);
    lastHighScore = pong.getHighScore();
  }
}

void UiPongRenderer::drawLives(uint8_t lives) {
  if (!carrier) return;

  uint16_t nativeBg  = color565(8, 12, 26);
  uint16_t activeRed = color565(255, 65, 95);   // Glowing red heart/life dot
  uint16_t lostGray  = color565(55, 65, 85);    // Muted lost life

  // Clear lives area (calibrated inside top circle chord at Y=30..41)
  carrier->display.fillRect(54, 30, 32, 12, nativeBg);

  // Draw 3 life dots (radius 3 at X=59, 68, 77, Y=35)
  for (int i = 0; i < 3; i++) {
    int cx = 59 + (i * 9);
    int cy = 35;
    uint16_t col = (i < lives) ? activeRed : lostGray;
    carrier->display.fillCircle(cx, cy, 3, col);
    if (i < lives) {
      carrier->display.drawPixel(cx - 1, cy - 1, 0xFFFF); // Highlight gleam
    }
  }
}

void UiPongRenderer::drawReadyPrompt() {
  if (!carrier) return;

  uint16_t cardBg   = color565(16, 26, 52);
  uint16_t cyanText = color565(80, 230, 255);

  carrier->display.fillRoundRect(41, 110, 158, 22, 5, cardBg);
  carrier->display.drawRoundRect(41, 110, 158, 22, 5, cyanText);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(cyanText, cardBg);
  const char* txt = "TRYK T2 (OK) FOR START";
  int txtW = strlen(txt) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - txtW) / 2, 117);
  carrier->display.print(txt);
}

void UiPongRenderer::clearReadyPrompt() {
  if (!carrier) return;
  uint16_t nativeBg = color565(8, 12, 26);
  carrier->display.fillRect(39, 108, 162, 26, nativeBg);
}

void UiPongRenderer::drawGameOverScreen(const PongLogic& pong) {
  if (!carrier) return;

  uint16_t modalBg   = color565(14, 20, 42);
  uint16_t redBorder = color565(255, 65, 75);
  uint16_t textWhite = 0xFFFF;
  uint16_t textGold  = color565(255, 215, 60);
  uint16_t cyanHint  = color565(80, 210, 255);

  // Central modal box
  carrier->display.fillRoundRect(36, 66, 168, 106, 8, modalBg);
  carrier->display.drawRoundRect(36, 66, 168, 106, 8, redBorder);

  // 1. GAME OVER Header
  carrier->display.setTextSize(2);
  carrier->display.setTextColor(redBorder, modalBg);
  const char* header = "GAME OVER";
  int headerW = strlen(header) * 12;
  carrier->display.setCursor((SCREEN_WIDTH - headerW) / 2, 76);
  carrier->display.print(header);

  // 2. Final Score
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(textWhite, modalBg);
  char buf[32];
  snprintf(buf, sizeof(buf), "Score: %u", pong.getScore());
  int scoreW = strlen(buf) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - scoreW) / 2, 100);
  carrier->display.print(buf);

  // 3. High Score & Record Announcement
  if (pong.isNewHighScore()) {
    carrier->display.setTextColor(textGold, modalBg);
    const char* rec = "* NY REKORD! *";
    int recW = strlen(rec) * 6;
    carrier->display.setCursor((SCREEN_WIDTH - recW) / 2, 116);
    carrier->display.print(rec);
  } else {
    carrier->display.setTextColor(textGold, modalBg);
    snprintf(buf, sizeof(buf), "Rekord: %u", pong.getHighScore());
    int hiW = strlen(buf) * 6;
    carrier->display.setCursor((SCREEN_WIDTH - hiW) / 2, 116);
    carrier->display.print(buf);
  }

  // 4. Replay / Dismiss Prompt
  carrier->display.setTextColor(cyanHint, modalBg);
  const char* sub = "T2:Spil igen | T3:Menu";
  int subW = strlen(sub) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - subW) / 2, 144);
  carrier->display.print(sub);
}

void UiPongRenderer::updateDisplay(const PongLogic& pong, bool forceRedraw) {
  if (!carrier) return;

  if (forceRedraw || !isInitialized) {
    drawInitialScreen(pong);
  }

  uint16_t nativeBg     = color565(8, 12, 26);
  uint16_t paddleCyan   = color565(0, 220, 255);
  uint16_t ballYellow   = color565(255, 230, 80);
  uint16_t wallRedDotted = color565(180, 50, 70);

  // State transitions
  PongState currentState = pong.getState();
  if (currentState != lastState) {
    if (lastState == PONG_STATE_READY && currentState == PONG_STATE_PLAYING) {
      clearReadyPrompt();
      lastBallX = -1;
      lastBallY = -1;
      lastPaddleY = -1;
    } else if (lastState == PONG_STATE_READY) {
      clearReadyPrompt();
    }

    if (currentState == PONG_STATE_READY) {
      drawReadyPrompt();
    } else if (currentState == PONG_STATE_BALL_LOST) {
      if (lastBallX >= 0) {
        carrier->display.fillRect(lastBallX, lastBallY,
                                  PongLogic::BALL_SIZE, PongLogic::BALL_SIZE, nativeBg);
        lastBallX = -1;
        lastBallY = -1;
      }
    } else if (currentState == PONG_STATE_GAMEOVER) {
      drawGameOverScreen(pong);
      lastState = currentState;
      return;
    }
    lastState = currentState;
  }

  if (currentState == PONG_STATE_GAMEOVER) {
    return; // Don't redraw court while modal is up
  }

  // 1. HUD updates
  drawHud(pong, false);

  // 2. Ball delta-erasure & rendering (visible during READY, BALL_LOST, and PLAYING)
  {
    int16_t newBallX = (int16_t)pong.getBallX();
    int16_t newBallY = (int16_t)pong.getBallY();
    if (newBallX != lastBallX || newBallY != lastBallY) {
      if (lastBallX != -1 && lastBallY != -1) {
        carrier->display.fillRect(lastBallX, lastBallY,
                                  PongLogic::BALL_SIZE, PongLogic::BALL_SIZE, nativeBg);
        if (lastBallX <= PongLogic::PADDLE_X + PongLogic::PADDLE_WIDTH + 1) {
          lastPaddleY = -1; // Force paddle redraw if erased ball touched paddle
        }
      }

      carrier->display.fillRoundRect(newBallX, newBallY,
                                    PongLogic::BALL_SIZE, PongLogic::BALL_SIZE, 1, ballYellow);
      carrier->display.drawPixel(newBallX + 1, newBallY + 1, 0xFFFF);
      lastBallX = newBallX;
      lastBallY = newBallY;
    }
  }

  // 3. Paddle delta-rendering (rendered after ball erase so paddle is never clipped by ball)
  int16_t newPaddleY = (int16_t)pong.getPaddleY();
  if (newPaddleY != lastPaddleY) {
    if (lastPaddleY != -1) {
      carrier->display.fillRect(PongLogic::PADDLE_X, lastPaddleY,
                                PongLogic::PADDLE_WIDTH, PongLogic::PADDLE_HEIGHT, nativeBg);
    }

    carrier->display.fillRoundRect(PongLogic::PADDLE_X, newPaddleY,
                                  PongLogic::PADDLE_WIDTH, PongLogic::PADDLE_HEIGHT, 2, paddleCyan);
    carrier->display.drawFastVLine(PongLogic::PADDLE_X + 2, newPaddleY + 2,
                                  PongLogic::PADDLE_HEIGHT - 4, 0xFFFF);
    lastPaddleY = newPaddleY;
  }

  // Restore left danger dotted line if ball was near left boundary
  if (currentState == PONG_STATE_PLAYING || currentState == PONG_STATE_BALL_LOST) {
    if (lastBallX <= PongLogic::FIELD_LEFT + 6 || pong.getBallX() <= (float)(PongLogic::FIELD_LEFT + 6)) {
      for (int16_t y = PongLogic::FIELD_TOP + 2; y < PongLogic::FIELD_BOTTOM; y += 4) {
        carrier->display.drawPixel(PongLogic::FIELD_LEFT, y, wallRedDotted);
      }
    }
  }
}

