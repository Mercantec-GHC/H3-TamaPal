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

  // 1. Clear display
  carrier->display.fillScreen(nativeBg);

  // 2. Court walls
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

  // 3. Bottom controls hint
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(hintColor, nativeBg);
  const char* hint = "Haeld/T0,T1 | T3:Exit";
  int hintW = strlen(hint) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - hintW) / 2, 214);
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

  // 1. Lives (Left)
  if (force || pong.getLives() != lastLives) {
    drawLives(pong.getLives());
    lastLives = pong.getLives();
  }

  // 2. Score (Center)
  if (force || pong.getScore() != lastScore) {
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(scoreCyan, nativeBg);
    carrier->display.setCursor(95, 20);
    char buf[16];
    snprintf(buf, sizeof(buf), "SCORE:%02u", pong.getScore());
    carrier->display.print(buf);
    lastScore = pong.getScore();
  }

  // 3. High Score (Right)
  if (force || pong.getHighScore() != lastHighScore) {
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(highGold, nativeBg);
    carrier->display.setCursor(168, 20);
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

  // Clear lives area
  carrier->display.fillRect(32, 18, 36, 12, nativeBg);

  // Draw 3 life dots (radius 3)
  for (int i = 0; i < 3; i++) {
    int cx = 36 + (i * 10);
    int cy = 24;
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

  carrier->display.fillRoundRect(50, 110, 140, 22, 5, cardBg);
  carrier->display.drawRoundRect(50, 110, 140, 22, 5, cyanText);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(cyanText, cardBg);
  const char* txt = "TRYK OK FOR SERVE";
  int txtW = strlen(txt) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - txtW) / 2, 117);
  carrier->display.print(txt);
}

void UiPongRenderer::clearReadyPrompt() {
  if (!carrier) return;
  uint16_t nativeBg = color565(8, 12, 26);
  carrier->display.fillRect(48, 108, 144, 26, nativeBg);
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

  // 4. Dismiss Prompt
  carrier->display.setTextColor(cyanHint, modalBg);
  const char* sub = "Tryk OK for menu";
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

  // State transitions
  PongState currentState = pong.getState();
  if (currentState != lastState) {
    if (lastState == PONG_STATE_READY) {
      clearReadyPrompt();
    }
    if (currentState == PONG_STATE_READY) {
      drawReadyPrompt();
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

  // 2. Paddle delta-rendering
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

  // 3. Ball delta-rendering
  int16_t newBallX = (int16_t)pong.getBallX();
  int16_t newBallY = (int16_t)pong.getBallY();
  if (newBallX != lastBallX || newBallY != lastBallY) {
    if (lastBallX != -1 && lastBallY != -1) {
      carrier->display.fillRect(lastBallX, lastBallY,
                                PongLogic::BALL_SIZE, PongLogic::BALL_SIZE, nativeBg);
    }

    carrier->display.fillRoundRect(newBallX, newBallY,
                                  PongLogic::BALL_SIZE, PongLogic::BALL_SIZE, 1, ballYellow);
    carrier->display.drawPixel(newBallX + 1, newBallY + 1, 0xFFFF);
    lastBallX = newBallX;
    lastBallY = newBallY;
  }
}

