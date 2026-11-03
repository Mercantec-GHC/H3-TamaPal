// WHAT: Implementation of the Pong / minigame controller screen on ST7789 display
// WHY:  Real-time non-flickering visual feedback for paddle slider, tilt, button, and MQTT state
// USED BY: TamaPal.ino

#include "ui_controller.h"
#include <string.h>

UiControllerRenderer::UiControllerRenderer()
  : carrier(nullptr),
    lastPaddleY(-1),
    lastNetworkStatus(false),
    lastButtonVisual(false),
    isInitialized(false) {
}

void UiControllerRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
}

void UiControllerRenderer::drawInitialScreen() {
  if (carrier == nullptr) return;

  uint16_t bgColor    = color565(8, 12, 26);
  uint16_t ringColor  = color565(25, 45, 80);
  uint16_t titleColor = color565(0, 230, 255);
  uint16_t railColor  = color565(20, 35, 65);
  uint16_t tickColor  = color565(40, 70, 120);
  uint16_t hintColor  = color565(110, 140, 190);

  // 1. Dark navy background
  carrier->display.fillScreen(bgColor);

  // 2. Subtle outer circular guideline
  carrier->display.drawCircle(120, 120, 118, ringColor);

  // 3. Header title (centered at Y=30)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(titleColor, bgColor);
  carrier->display.setCursor(72, 30);
  carrier->display.print("PONG KONTROL");

  // 4. Slider track for vertical paddle movement (Height: 104 px, Center: Y=120)
  carrier->display.fillRoundRect(116, 68, 8, 104, 4, railColor);
  carrier->display.drawRoundRect(115, 67, 10, 106, 5, color565(30, 55, 95));

  // Center neutral tick mark
  carrier->display.drawFastHLine(110, 120, 20, tickColor);
  carrier->display.drawFastHLine(112, 80, 16, color565(25, 45, 75));  // Top guide
  carrier->display.drawFastHLine(112, 160, 16, color565(25, 45, 75)); // Bottom guide

  // 5. Bottom touch button hints (safe inside round bezel)
  carrier->display.setTextColor(hintColor, bgColor);
  carrier->display.setCursor(76, 204);
  carrier->display.print("T2: SERVE");
  carrier->display.setCursor(68, 218);
  carrier->display.print("T3: AFSLUT");

  lastPaddleY = -1;
  isInitialized = true;
}

void UiControllerRenderer::updateDisplay(const TiltLogic& tilt, bool isMqttConnected, bool forceFullRedraw) {
  if (carrier == nullptr) return;

  if (forceFullRedraw || !isInitialized) {
    drawInitialScreen();
  }

  uint16_t bgColor     = color565(8, 12, 26);
  uint16_t railColor   = color565(20, 35, 65);
  uint16_t paddleColor = color565(0, 230, 255);
  uint16_t paddleGloss = color565(180, 255, 255);
  uint16_t paddleShadow= color565(15, 80, 140);

  // 1. Calculate paddle Y position from normalized tilt (-1.0 to 1.0)
  // Travel range: Y=70 to Y=152 (span: 82 px, center at 111)
  float tiltY = tilt.getTiltY();
  int paddleY = 111 + (int)(tiltY * 41.0f);
  if (paddleY < 70) paddleY = 70;
  if (paddleY > 152) paddleY = 152;

  // 2. Redraw paddle ONLY if position changed (prevents flicker!)
  if (paddleY != lastPaddleY || forceFullRedraw) {
    if (lastPaddleY >= 0) {
      // Erase previous paddle area with background and repair the track slot
      carrier->display.fillRect(112, lastPaddleY, 16, 18, bgColor);
      // Restore rail slot in the erased segment
      int restoreY = lastPaddleY;
      int restoreH = 18;
      if (restoreY < 68) { restoreH -= (68 - restoreY); restoreY = 68; }
      if (restoreY + restoreH > 172) { restoreH = 172 - restoreY; }
      if (restoreH > 0) {
        carrier->display.fillRect(116, restoreY, 8, restoreH, railColor);
      }
      // Re-draw guide ticks if old paddle covered them
      if (lastPaddleY <= 80 && lastPaddleY + 18 >= 80) {
        carrier->display.drawFastHLine(112, 80, 16, color565(25, 45, 75));
      }
      if (lastPaddleY <= 120 && lastPaddleY + 18 >= 120) {
        carrier->display.drawFastHLine(110, 120, 20, color565(40, 70, 120));
      }
      if (lastPaddleY <= 160 && lastPaddleY + 18 >= 160) {
        carrier->display.drawFastHLine(112, 160, 16, color565(25, 45, 75));
      }
    }

    // Draw new paddle slider thumb (14 px wide x 18 px high with 2.5D shine)
    carrier->display.fillRoundRect(113, paddleY, 14, 18, 3, paddleColor);
    carrier->display.drawFastHLine(115, paddleY + 1, 10, paddleGloss);  // Top highlight
    carrier->display.drawFastHLine(115, paddleY + 16, 10, paddleShadow); // Bottom shadow
    carrier->display.drawRoundRect(113, paddleY, 14, 18, 3, color565(255, 255, 255));

    lastPaddleY = paddleY;
  }

  // 3. Network status badge (ONLINE / FORBINDER...) at Y=46
  if (isMqttConnected != lastNetworkStatus || forceFullRedraw) {
    carrier->display.fillRect(60, 44, 120, 12, bgColor);
    carrier->display.setTextSize(1);
    if (isMqttConnected) {
      carrier->display.fillCircle(82, 49, 3, color565(50, 255, 120)); // Green dot
      carrier->display.setTextColor(color565(50, 255, 120), bgColor);
      carrier->display.setCursor(92, 46);
      carrier->display.print("ONLINE");
    } else {
      carrier->display.fillCircle(72, 49, 3, color565(255, 180, 50)); // Amber dot
      carrier->display.setTextColor(color565(255, 180, 50), bgColor);
      carrier->display.setCursor(82, 46);
      carrier->display.print("FORBINDER");
    }
    lastNetworkStatus = isMqttConnected;
  }

  // 4. Button press flash indicator at Y=180
  bool buttonPressed = tilt.isButtonPressed();
  if (buttonPressed != lastButtonVisual || forceFullRedraw) {
    carrier->display.fillRect(70, 178, 100, 14, bgColor);
    if (buttonPressed) {
      // Glow pill when serving ball
      carrier->display.fillRoundRect(80, 178, 80, 14, 4, color565(255, 60, 80));
      carrier->display.setTextColor(color565(255, 255, 255), color565(255, 60, 80));
      carrier->display.setCursor(95, 181);
      carrier->display.print("! SERVE !");
    }
    lastButtonVisual = buttonPressed;
  }
}

