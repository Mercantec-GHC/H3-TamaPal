// WHAT: Implementation of the universal controller screen (Pong buttons + Labyrinth 2D tilt) on ST7789 display
// WHY:  Provides instant visual feedback for buttons, 2D tilt ball, and MQTT network state with zero flicker
// USED BY: TamaPal.ino

#include "ui_controller.h"
#include <stdlib.h>

namespace {
  // Arena geometry constants
  const int ARENA_CX = 120;
  const int ARENA_CY = 118;
  const int ARENA_RADIUS = 34;
  const int BALL_RADIUS = 5;
  const int MAX_BALL_OFFSET = 26;

  // Button geometry constants
  const int BTN_UP_X = 14;
  const int BTN_UP_Y = 106;
  const int BTN_DN_X = 166;
  const int BTN_DN_Y = 106;
  const int BTN_W    = 60;
  const int BTN_H    = 25;

  const int BTN_ACT_X = 74;
  const int BTN_ACT_Y = 166;
  const int BTN_ACT_W = 92;
  const int BTN_ACT_H = 24;
}

UiControllerRenderer::UiControllerRenderer()
  : carrier(nullptr),
    lastBallX(-1),
    lastBallY(-1),
    lastNetworkStatus(false),
    lastBtnUp(false),
    lastBtnDown(false),
    lastBtnAction(false),
    isInitialized(false) {
}

void UiControllerRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
}

void UiControllerRenderer::drawInitialScreen() {
  if (carrier == nullptr) return;

  uint16_t bgColor       = color565(8, 12, 26);
  uint16_t ringColor     = color565(25, 45, 80);
  uint16_t titleColor    = color565(0, 230, 255);
  uint16_t arenaBgColor  = color565(14, 22, 44);
  uint16_t arenaBorder   = color565(40, 75, 130);
  uint16_t hintColor     = color565(100, 130, 175);

  // 1. Dark navy background
  carrier->display.fillScreen(bgColor);

  // 2. Subtle outer circular guideline
  carrier->display.drawCircle(120, 120, 118, ringColor);

  // 3. Header title (centered at Y=24)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(titleColor, bgColor);
  carrier->display.setCursor(99, 24);
  carrier->display.print("KONTROL");

  // 4. Center 2D circular tilt arena for Labyrinth
  carrier->display.fillCircle(ARENA_CX, ARENA_CY, ARENA_RADIUS, arenaBgColor);
  carrier->display.drawCircle(ARENA_CX, ARENA_CY, ARENA_RADIUS, arenaBorder);
  carrier->display.drawCircle(ARENA_CX, ARENA_CY, ARENA_RADIUS + 1, color565(20, 35, 65));

  // Arena crosshairs
  carrier->display.drawFastHLine(ARENA_CX - 28, ARENA_CY, 57, color565(28, 46, 80));
  carrier->display.drawFastVLine(ARENA_CX, ARENA_CY - 28, 57, color565(28, 46, 80));
  carrier->display.drawCircle(ARENA_CX, ARENA_CY, 13, color565(22, 36, 65));

  // 5. Draw button indicators in initial unpressed state
  drawButtonUp(false);
  drawButtonDown(false);
  drawButtonAction(false);

  // 6. Bottom touch button hint (T3 = Back)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(hintColor, bgColor);
  carrier->display.setCursor(86, 214);
  carrier->display.print("T3: TILBAGE");

  lastBallX = -1;
  lastBallY = -1;
  lastBtnUp = false;
  lastBtnDown = false;
  lastBtnAction = false;
  lastNetworkStatus = false;
  isInitialized = true;
}

void UiControllerRenderer::drawButtonUp(bool pressed) {
  if (carrier == nullptr) return;

  uint16_t fillCol   = pressed ? color565(0, 180, 235)   : color565(16, 26, 48);
  uint16_t borderCol = pressed ? color565(220, 255, 255) : color565(35, 60, 100);
  uint16_t textCol   = pressed ? color565(0, 20, 45)     : color565(120, 160, 210);

  carrier->display.fillRoundRect(BTN_UP_X, BTN_UP_Y, BTN_W, BTN_H, 4, fillCol);
  carrier->display.drawRoundRect(BTN_UP_X, BTN_UP_Y, BTN_W, BTN_H, 4, borderCol);

  // Triangle arrow pointing UP
  carrier->display.fillTriangle(BTN_UP_X + 9,  BTN_UP_Y + 16,
                                BTN_UP_X + 15, BTN_UP_Y + 8,
                                BTN_UP_X + 21, BTN_UP_Y + 16, textCol);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(textCol, fillCol);
  carrier->display.setCursor(BTN_UP_X + 26, BTN_UP_Y + 9);
  carrier->display.print("OP");
}

void UiControllerRenderer::drawButtonDown(bool pressed) {
  if (carrier == nullptr) return;

  uint16_t fillCol   = pressed ? color565(0, 180, 235)   : color565(16, 26, 48);
  uint16_t borderCol = pressed ? color565(220, 255, 255) : color565(35, 60, 100);
  uint16_t textCol   = pressed ? color565(0, 20, 45)     : color565(120, 160, 210);

  carrier->display.fillRoundRect(BTN_DN_X, BTN_DN_Y, BTN_W, BTN_H, 4, fillCol);
  carrier->display.drawRoundRect(BTN_DN_X, BTN_DN_Y, BTN_W, BTN_H, 4, borderCol);

  // Triangle arrow pointing DOWN
  carrier->display.fillTriangle(BTN_DN_X + 9,  BTN_DN_Y + 8,
                                BTN_DN_X + 15, BTN_DN_Y + 16,
                                BTN_DN_X + 21, BTN_DN_Y + 8, textCol);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(textCol, fillCol);
  carrier->display.setCursor(BTN_DN_X + 26, BTN_DN_Y + 9);
  carrier->display.print("NED");
}

void UiControllerRenderer::drawButtonAction(bool pressed) {
  if (carrier == nullptr) return;

  uint16_t fillCol   = pressed ? color565(255, 60, 80)   : color565(16, 26, 48);
  uint16_t borderCol = pressed ? color565(255, 200, 210) : color565(35, 60, 100);
  uint16_t textCol   = pressed ? color565(255, 255, 255) : color565(120, 160, 210);

  carrier->display.fillRoundRect(BTN_ACT_X, BTN_ACT_Y, BTN_ACT_W, BTN_ACT_H, 4, fillCol);
  carrier->display.drawRoundRect(BTN_ACT_X, BTN_ACT_Y, BTN_ACT_W, BTN_ACT_H, 4, borderCol);

  // Action bullet dot
  carrier->display.fillCircle(BTN_ACT_X + 14, BTN_ACT_Y + 12, 3, textCol);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(textCol, fillCol);
  carrier->display.setCursor(BTN_ACT_X + 24, BTN_ACT_Y + 8);
  carrier->display.print("SERV T2");
}

void UiControllerRenderer::redrawArenaCrosshair(int x, int y) {
  if (carrier == nullptr) return;

  uint16_t crossColor = color565(28, 46, 80);
  uint16_t circleColor= color565(22, 36, 65);

  // Horizontal line segment restore
  if (abs(y - ARENA_CY) <= BALL_RADIUS + 1) {
    int startX = x - (BALL_RADIUS + 1);
    int endX   = x + (BALL_RADIUS + 1);
    if (startX < ARENA_CX - 28) startX = ARENA_CX - 28;
    if (endX > ARENA_CX + 28) endX = ARENA_CX + 28;
    if (endX >= startX) {
      carrier->display.drawFastHLine(startX, ARENA_CY, endX - startX + 1, crossColor);
    }
  }

  // Vertical line segment restore
  if (abs(x - ARENA_CX) <= BALL_RADIUS + 1) {
    int startY = y - (BALL_RADIUS + 1);
    int endY   = y + (BALL_RADIUS + 1);
    if (startY < ARENA_CY - 28) startY = ARENA_CY - 28;
    if (endY > ARENA_CY + 28) endY = ARENA_CY + 28;
    if (endY >= startY) {
      carrier->display.drawFastVLine(ARENA_CX, startY, endY - startY + 1, crossColor);
    }
  }

  // Center ring segment restore if near radius 13
  carrier->display.drawCircle(ARENA_CX, ARENA_CY, 13, circleColor);
}

void UiControllerRenderer::updateDisplay(const TiltLogic& tilt, bool isMqttConnected, bool forceFullRedraw) {
  if (carrier == nullptr) return;

  if (forceFullRedraw || !isInitialized) {
    drawInitialScreen();
  }

  uint16_t bgColor      = color565(8, 12, 26);
  uint16_t arenaBgColor = color565(14, 22, 44);
  uint16_t ballColor    = color565(0, 230, 255);
  uint16_t ballRim      = color565(255, 255, 255);

  // 1. Calculate 2D ball position inside arena
  float tiltX = tilt.getTiltX();
  float tiltY = tilt.getTiltY();
  int ballX = ARENA_CX + (int)(tiltX * MAX_BALL_OFFSET);
  int ballY = ARENA_CY + (int)(tiltY * MAX_BALL_OFFSET);

  // Clamp within circular radius boundary
  int dx = ballX - ARENA_CX;
  int dy = ballY - ARENA_CY;
  int distSq = dx * dx + dy * dy;
  if (distSq > MAX_BALL_OFFSET * MAX_BALL_OFFSET) {
    // Keep ball within boundary
    ballX = ARENA_CX + (int)((float)dx * (MAX_BALL_OFFSET / 27.0f));
    ballY = ARENA_CY + (int)((float)dy * (MAX_BALL_OFFSET / 27.0f));
  }

  // 2. Redraw 2D ball only when moved (zero-flicker localized update)
  if (ballX != lastBallX || ballY != lastBallY || forceFullRedraw) {
    if (lastBallX >= 0) {
      // Erase old ball
      carrier->display.fillCircle(lastBallX, lastBallY, BALL_RADIUS + 1, arenaBgColor);
      redrawArenaCrosshair(lastBallX, lastBallY);
    }

    // Draw new 2D ball with shine
    carrier->display.fillCircle(ballX, ballY, BALL_RADIUS, ballColor);
    carrier->display.drawCircle(ballX, ballY, BALL_RADIUS, ballRim);
    carrier->display.drawPixel(ballX - 1, ballY - 1, color565(255, 255, 255));

    lastBallX = ballX;
    lastBallY = ballY;
  }

  // 3. Network status badge at Y=38
  if (isMqttConnected != lastNetworkStatus || forceFullRedraw) {
    carrier->display.fillRect(60, 36, 120, 14, bgColor);
    carrier->display.setTextSize(1);
    if (isMqttConnected) {
      carrier->display.fillCircle(88, 42, 3, color565(50, 255, 120)); // Green dot
      carrier->display.setTextColor(color565(50, 255, 120), bgColor);
      carrier->display.setCursor(96, 39);
      carrier->display.print("ONLINE");
    } else {
      carrier->display.fillCircle(78, 42, 3, color565(255, 180, 50)); // Amber dot
      carrier->display.setTextColor(color565(255, 180, 50), bgColor);
      carrier->display.setCursor(86, 39);
      carrier->display.print("FORBINDER");
    }
    lastNetworkStatus = isMqttConnected;
  }

  // 4. Update Button Up (Pong)
  bool curUp = tilt.isUpPressed();
  if (curUp != lastBtnUp || forceFullRedraw) {
    drawButtonUp(curUp);
    lastBtnUp = curUp;
  }

  // 5. Update Button Down (Pong)
  bool curDown = tilt.isDownPressed();
  if (curDown != lastBtnDown || forceFullRedraw) {
    drawButtonDown(curDown);
    lastBtnDown = curDown;
  }

  // 6. Update Button Action (Pong & Labyrinth)
  bool curAction = tilt.isButtonPressed();
  if (curAction != lastBtnAction || forceFullRedraw) {
    drawButtonAction(curAction);
    lastBtnAction = curAction;
  }
}
