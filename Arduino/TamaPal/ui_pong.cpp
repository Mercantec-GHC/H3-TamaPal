// WHAT: Implementation of Solo Wall-Pong rendering on the circular ST7789 display
// WHY:  Zero-flicker delta-rendering with blazing fireball, wall ricochet sparks, and Lumi cheerleader mascot
// USED BY: TamaPal.ino

#include "ui_pong.h"
#include <stdio.h>
#include <string.h>

UiPongRenderer::UiPongRenderer()
  : carrier(nullptr),
    lastPaddleY(-1),
    lastBotY(-1),
    lastBallX(-1),
    lastBallY(-1),
    cheerTimer(0),
    lastCheerMood(255),
    lastCheerAnimTick(255),
    lastScore(9999),
    lastHighScore(9999),
    lastLives(255),
    scoreFlashTimer(0),
    missFlashTimer(0),
    paddleFlashTimer(0),
    wallRippleX(-1),
    wallRippleY(-1),
    wallRippleTimer(0),
    lastState(PONG_STATE_READY),
    isInitialized(false) {
  for (uint8_t i = 0; i < TRAIL_LEN; i++) {
    trailX[i] = -1;
    trailY[i] = -1;
  }
  for (uint8_t i = 0; i < MAX_SPARKS; i++) {
    sparks[i].life = 0;
  }
}

void UiPongRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
  isInitialized = false;
  scoreFlashTimer = 0;
  missFlashTimer = 0;
  paddleFlashTimer = 0;
  wallRippleTimer = 0;
}

void UiPongRenderer::triggerCheer() {
  cheerTimer = 18; // ~360 ms cheering jump animation
}

void UiPongRenderer::triggerPaddleSparks(float ballX, float ballY) {
  int16_t cx = (int16_t)ballX + PongLogic::BALL_SIZE;
  int16_t cy = (int16_t)ballY + PongLogic::BALL_SIZE / 2;

  // Impact Flash: paddle glows white-hot for 4 frames
  paddleFlashTimer = 4;

  // Electric cyan and pure white shockwave sparks spraying rightward
  uint16_t sparkColors[3] = {
    color565(80, 245, 255),   // Electric neon cyan
    0xFFFF,                   // White-hot shock spark
    color565(0, 210, 255)     // Brilliant cyan
  };

  int8_t spreadX[6] = {  2,  3,  4,  4,  3,  2 };
  int8_t spreadY[6] = { -3, -2, -1,  1,  2,  3 };

  for (uint8_t i = 0; i < MAX_SPARKS; i++) {
    sparks[i].x     = cx;
    sparks[i].y     = cy;
    sparks[i].prevX = cx;
    sparks[i].prevY = cy;
    sparks[i].vx    = spreadX[i];
    sparks[i].vy    = spreadY[i];
    sparks[i].life  = 5 + (i & 1);
    sparks[i].color = sparkColors[i % 3];
  }
}

void UiPongRenderer::triggerWallSparks(float ballX, float ballY, float ballVx, float ballVy) {
  int16_t cx = (int16_t)ballX + PongLogic::BALL_SIZE / 2;
  int16_t cy = (int16_t)ballY + PongLogic::BALL_SIZE / 2;

  bool hitTop    = (ballY <= (float)(PongLogic::FIELD_TOP + 4));
  bool hitBottom = (ballY >= (float)(PongLogic::FIELD_BOTTOM - PongLogic::BALL_SIZE - 4));

  if (hitTop || hitBottom) {
    wallRippleX = cx;
    wallRippleY = cy;
    wallRippleTimer = 4;
  }

  uint16_t sparkColors[3];
  if (hitTop || hitBottom) {
    sparkColors[0] = color565(255, 240, 120); // Solar white-gold
    sparkColors[1] = color565(80, 220, 255);  // Electric blue
    sparkColors[2] = 0xFFFF;                  // White ricochet spark
  } else {
    // Right wall / Bot hit: Solar flame sparks
    sparkColors[0] = color565(255, 215, 40);  // Pure solar gold
    sparkColors[1] = color565(255, 120, 20);  // Blazing orange
    sparkColors[2] = color565(255, 60, 20);   // Crimson ember
  }

  for (uint8_t i = 0; i < MAX_SPARKS; i++) {
    sparks[i].x     = cx;
    sparks[i].y     = cy;
    sparks[i].prevX = cx;
    sparks[i].prevY = cy;
    sparks[i].life  = 5 + (i & 1);
    sparks[i].color = sparkColors[i % 3];

    if (hitTop) {
      // Spray downward and fan horizontally
      int8_t spreadX[6] = { -3, -2, -1, 1, 2, 3 };
      int8_t spreadY[6] = {  1,  2,  3, 3, 2, 1 };
      sparks[i].vx = spreadX[i];
      sparks[i].vy = spreadY[i];
    } else if (hitBottom) {
      // Spray upward and fan horizontally
      int8_t spreadX[6] = { -3, -2, -1, 1, 2, 3 };
      int8_t spreadY[6] = { -1, -2, -3, -3, -2, -1 };
      sparks[i].vx = spreadX[i];
      sparks[i].vy = spreadY[i];
    } else {
      // Right wall / Bot ricochet: spray leftward and fan vertically
      int8_t spreadX[6] = { -2, -3, -4, -3, -2, -4 };
      int8_t spreadY[6] = { -3, -2, -1,  1,  2,  3 };
      sparks[i].vx = (ballVx < 0.0f) ? spreadX[i] : -spreadX[i];
      sparks[i].vy = spreadY[i];
      (void)ballVy;
    }
  }
}

void UiPongRenderer::drawInitialScreen(const PongLogic& pong) {
  if (!carrier) return;

  uint16_t nativeBg     = 0x0000;                  // Deep true black for maximum IPS contrast
  uint16_t borderCyan   = color565(0, 240, 255);   // Electric cyber-cyan boundary rails
  uint16_t borderDark   = color565(0, 85, 150);    // Deep rail under-edge
  uint16_t hintColor    = color565(120, 185, 230); // High-contrast navigation text

  // 1. Clear display and draw circular stadium bezel rings (R = 112..114)
  carrier->display.fillScreen(nativeBg);
  carrier->display.drawCircle(120, 120, 114, color565(16, 42, 85));
  carrier->display.drawCircle(120, 120, 112, color565(32, 95, 175));

  // 2. High-energy court rails (calibrated inside circular viewport: X=42..198, Y=52..188)
  // Top neon rail
  carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_TOP,
                                PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderCyan);
  carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_TOP - 1,
                                PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderDark);

  // Bottom neon rail
  carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_BOTTOM,
                                PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderCyan);
  carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_BOTTOM + 1,
                                PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderDark);

  // Center court dotted net (X = 120) - skips mascot stadium circle (Y=60..94)
  for (int y = PongLogic::FIELD_TOP + 4; y < PongLogic::FIELD_BOTTOM - 2; y += 6) {
    if (y < 60 || y > 94) {
      carrier->display.drawFastVLine(120, y, 3, color565(30, 65, 115));
    }
  }

  // Left & Right goal lines
  for (int y = PongLogic::FIELD_TOP + 2; y < PongLogic::FIELD_BOTTOM; y += 4) {
    carrier->display.drawPixel(PongLogic::FIELD_LEFT, y, color565(225, 45, 65));
    carrier->display.drawPixel(PongLogic::FIELD_RIGHT + 3, y, color565(180, 110, 25));
  }

  // 3. Bottom controls hint (calibrated at Y=196 inside circular bottom arc)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(hintColor, nativeBg);
  const char* hint = "T1:Op T0:Ned T2:OK T3:Ud";
  int hintW = strlen(hint) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - hintW) / 2, 196);
  carrier->display.print(hint);

  // 4. Reset cached state
  lastPaddleY   = -1;
  lastBotY      = -1;
  lastBallX     = -1;
  lastBallY     = -1;
  for (uint8_t i = 0; i < TRAIL_LEN; i++) {
    trailX[i] = -1;
    trailY[i] = -1;
  }
  for (uint8_t i = 0; i < MAX_SPARKS; i++) {
    sparks[i].life = 0;
  }
  cheerTimer        = 0;
  lastCheerMood     = 255;
  lastCheerAnimTick = 255;
  lastScore         = 9999;
  lastHighScore     = 9999;
  lastLives         = 255;
  paddleFlashTimer  = 0;
  wallRippleTimer   = 0;
  lastState         = pong.getState();
  isInitialized     = true;

  drawHud(pong, true);
  drawCheerleaderLumi(pong, true);

  if (pong.getState() == PONG_STATE_READY) {
    drawReadyPrompt();
  }
}

void UiPongRenderer::drawCheerleaderLumi(const PongLogic& pong, bool force) {
  if (!carrier) return;

  // Mascot stadium box centered at (120, 76) - safely above the Ready prompt (Y=110) and below Top Wall (Y=52)
  const int16_t cx = 120;
  const int16_t cy = 76;

  // Determine mood: 0 = watching, 1 = cheering jump, 2 = shocked/missed, 3 = gazing left, 4 = gazing right
  uint8_t mood = 0;
  if (pong.getState() == PONG_STATE_BALL_LOST) {
    mood = 2; // Shocked O_O
  } else if (cheerTimer > 0) {
    mood = 1; // Cheering ^_^ with pom-poms raised
  } else if (pong.getBallX() < 105.0f) {
    mood = 3; // Watching left
  } else if (pong.getBallX() > 135.0f) {
    mood = 4; // Watching right
  }

  uint8_t animTick = (uint8_t)((millis() / 320) & 1);
  if (!force && mood == lastCheerMood && animTick == lastCheerAnimTick) {
    return;
  }
  lastCheerMood     = mood;
  lastCheerAnimTick = animTick;

  uint16_t nativeBg    = 0x0000;
  uint16_t ringCol     = (mood == 1) ? color565(40, 140, 210) : color565(20, 55, 105);
  uint16_t lumiBody    = color565(85, 255, 140);  // Vivid candy lime body
  uint16_t lumiOutline = color565(20, 125, 60);   // Deep 3D emerald contour
  uint16_t pomGold     = color565(255, 225, 45);
  uint16_t pomOrange   = color565(255, 130, 20);
  uint16_t blushPink   = color565(255, 105, 155);

  // Clear mascot zone (X=102..138, Y=58..94)
  carrier->display.fillRect(cx - 17, cy - 17, 35, 35, nativeBg);
  for (int y = PongLogic::FIELD_TOP + 4; y < PongLogic::FIELD_BOTTOM - 2; y += 6) {
    if ((y < 60 || y > 94) && y >= cy - 18 && y <= cy + 17) {
      carrier->display.drawFastVLine(120, y, 3, color565(30, 65, 115));
    }
  }

  // Stadium mascot circle on court floor
  carrier->display.drawCircle(cx, cy, 14, ringCol);

  int16_t bodyY = (mood == 1) ? (cy - 2) : (cy + (animTick ? 0 : 1));

  // Lumi rounded mochi body with specular cut
  carrier->display.fillCircle(cx, bodyY, 8, lumiBody);
  carrier->display.drawCircle(cx, bodyY, 8, lumiOutline);
  carrier->display.drawPixel(cx - 4, bodyY - 4, 0xFFFF);

  // Cute spirit ears on top-left and top-right
  carrier->display.fillTriangle(cx - 7, bodyY - 4, cx - 4, bodyY - 10, cx - 2, bodyY - 6, lumiBody);
  carrier->display.fillTriangle(cx + 7, bodyY - 4, cx + 4, bodyY - 10, cx + 2, bodyY - 6, lumiBody);

  // Golden leaf sprout on top of head
  int16_t sproutDx = (animTick && mood != 2) ? 1 : 0;
  carrier->display.drawFastVLine(cx, bodyY - 11, 3, pomGold);
  carrier->display.drawPixel(cx - 1 + sproutDx, bodyY - 12, pomGold);
  carrier->display.drawPixel(cx + 1 + sproutDx, bodyY - 11, color565(140, 255, 120));

  // Facial expressions & Pom-poms
  if (mood == 1) {
    // CHEERING! Happy ^ _ ^ eyes, rosy cheeks, open smile, pom-poms raised high!
    carrier->display.drawLine(cx - 4, bodyY - 1, cx - 3, bodyY - 2, 0x0000);
    carrier->display.drawLine(cx - 3, bodyY - 2, cx - 2, bodyY - 1, 0x0000);
    carrier->display.drawLine(cx + 2, bodyY - 1, cx + 3, bodyY - 2, 0x0000);
    carrier->display.drawLine(cx + 3, bodyY - 2, cx + 4, bodyY - 1, 0x0000);
    carrier->display.drawPixel(cx - 5, bodyY + 1, blushPink);
    carrier->display.drawPixel(cx + 5, bodyY + 1, blushPink);
    carrier->display.drawFastHLine(cx - 2, bodyY + 2, 5, 0x0000);
    carrier->display.drawFastHLine(cx - 1, bodyY + 3, 3, color565(255, 60, 95));

    // Raised pom-poms!
    carrier->display.fillCircle(cx - 11, bodyY - 6, 3, pomGold);
    carrier->display.drawPixel(cx - 11, bodyY - 6, pomOrange);
    carrier->display.fillCircle(cx + 11, bodyY - 6, 3, pomGold);
    carrier->display.drawPixel(cx + 11, bodyY - 6, pomOrange);
  } else if (mood == 2) {
    // SHOCKED (Missed ball!): Wide O _ O eyes and gasping O mouth!
    carrier->display.fillCircle(cx - 3, bodyY - 1, 2, 0xFFFF);
    carrier->display.drawPixel(cx - 3, bodyY - 1, 0x0000);
    carrier->display.fillCircle(cx + 3, bodyY - 1, 2, 0xFFFF);
    carrier->display.drawPixel(cx + 3, bodyY - 1, 0x0000);
    carrier->display.drawCircle(cx, bodyY + 3, 2, 0x0000);
    // Paws held to cheeks
    carrier->display.fillCircle(cx - 7, bodyY + 2, 2, lumiOutline);
    carrier->display.fillCircle(cx + 7, bodyY + 2, 2, lumiOutline);
  } else {
    // WATCHING THE RALLY: Eyes follow the flaming ball left or right
    int16_t lookX = (mood == 3) ? -1 : ((mood == 4) ? 1 : 0);
    carrier->display.fillRect(cx - 4 + lookX, bodyY - 2, 2, 2, 0x0000);
    carrier->display.fillRect(cx + 3 + lookX, bodyY - 2, 2, 2, 0x0000);
    carrier->display.drawPixel(cx - 4 + lookX, bodyY - 2, 0xFFFF);
    carrier->display.drawPixel(cx + 3 + lookX, bodyY - 2, 0xFFFF);
    carrier->display.drawPixel(cx - 5, bodyY + 1, blushPink);
    carrier->display.drawPixel(cx + 5, bodyY + 1, blushPink);
    carrier->display.drawFastHLine(cx - 1, bodyY + 2, 3, 0x0000);

    // Pom-poms bobbing at sides
    int16_t pomY = bodyY + (animTick ? 0 : 2);
    carrier->display.fillCircle(cx - 10, pomY, 2, pomGold);
    carrier->display.fillCircle(cx + 10, pomY, 2, pomGold);
  }
}

void UiPongRenderer::drawHud(const PongLogic& pong, bool force) {
  if (!carrier) return;

  uint16_t nativeBg  = 0x0000;
  uint16_t scoreCyan = color565(90, 240, 255);
  uint16_t scoreGold = color565(255, 235, 80);
  uint16_t highGold  = color565(255, 215, 60);

  // 1. Lives (Left, calibrated at X=52..86, Y=32 inside top circular arc)
  if (force || pong.getLives() != lastLives) {
    drawLives(pong.getLives());
    lastLives = pong.getLives();
  }

  // 2. Score (Center, Y=32) - flashes radiant gold when a point is scored!
  if (force || pong.getScore() != lastScore || scoreFlashTimer > 0) {
    carrier->display.setTextSize(1);
    uint16_t sColor = (scoreFlashTimer > 0) ? scoreGold : scoreCyan;
    carrier->display.setTextColor(sColor, nativeBg);
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

  uint16_t nativeBg   = 0x0000;
  uint16_t activeRed  = color565(255, 35, 70);   // Glowing ruby heart
  uint16_t activeDark = color565(160, 15, 40);   // Shaded lower edge
  uint16_t lostGray   = color565(50, 60, 80);    // Muted lost life outline

  // Clear lives area
  carrier->display.fillRect(52, 29, 36, 12, nativeBg);

  // Draw 3 pixel-art hearts (7x6 px each at X=56, 67, 78, Y=31)
  for (int i = 0; i < 3; i++) {
    int hx = 56 + (i * 11);
    int hy = 32;

    if (i < lives) {
      // Row 0: . X X . X X .
      carrier->display.drawFastHLine(hx + 1, hy, 2, activeRed);
      carrier->display.drawFastHLine(hx + 4, hy, 2, activeRed);
      // Row 1: X X X X X X X
      carrier->display.drawFastHLine(hx, hy + 1, 7, activeRed);
      carrier->display.drawPixel(hx + 1, hy + 1, 0xFFFF); // White sheen gleam
      // Row 2: X X X X X X X
      carrier->display.drawFastHLine(hx, hy + 2, 7, activeRed);
      // Row 3: . X X X X X .
      carrier->display.drawFastHLine(hx + 1, hy + 3, 5, activeRed);
      // Row 4: . . X X X . .
      carrier->display.drawFastHLine(hx + 2, hy + 4, 3, activeDark);
      // Row 5: . . . X . . .
      carrier->display.drawPixel(hx + 3, hy + 5, activeDark);
    } else {
      // Broken / depleted heart outline with diagonal crack
      carrier->display.drawPixel(hx + 1, hy, lostGray);
      carrier->display.drawPixel(hx + 2, hy, lostGray);
      carrier->display.drawPixel(hx + 4, hy, lostGray);
      carrier->display.drawPixel(hx + 5, hy, lostGray);
      carrier->display.drawPixel(hx, hy + 1, lostGray);
      carrier->display.drawPixel(hx + 6, hy + 1, lostGray);
      carrier->display.drawPixel(hx, hy + 2, lostGray);
      carrier->display.drawPixel(hx + 6, hy + 2, lostGray);
      carrier->display.drawPixel(hx + 1, hy + 3, lostGray);
      carrier->display.drawPixel(hx + 5, hy + 3, lostGray);
      carrier->display.drawPixel(hx + 2, hy + 4, lostGray);
      carrier->display.drawPixel(hx + 4, hy + 4, lostGray);
      carrier->display.drawPixel(hx + 3, hy + 5, lostGray);
      // Crack through center
      carrier->display.drawPixel(hx + 3, hy + 2, lostGray);
    }
  }
}

void UiPongRenderer::drawReadyPrompt() {
  if (!carrier) return;

  uint16_t cardBg   = color565(12, 24, 52);
  uint16_t cyanText = color565(90, 240, 255);

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
  uint16_t nativeBg = 0x0000;
  carrier->display.fillRect(39, 108, 162, 26, nativeBg);
  // Restore center dotted net where ready prompt was
  for (int y = PongLogic::FIELD_TOP + 4; y < PongLogic::FIELD_BOTTOM - 2; y += 6) {
    if (y >= 106 && y <= 136) {
      carrier->display.drawFastVLine(120, y, 3, color565(30, 65, 115));
    }
  }
}

void UiPongRenderer::drawGameOverScreen(const PongLogic& pong) {
  if (!carrier) return;

  uint16_t modalBg   = color565(14, 20, 42);
  uint16_t redBorder = color565(255, 65, 75);
  uint16_t redInner  = color565(130, 25, 40);
  uint16_t textWhite = 0xFFFF;
  uint16_t textGold  = color565(255, 215, 60);
  uint16_t cyanHint  = color565(90, 225, 255);

  // Central modal box with double neon rim
  carrier->display.fillRoundRect(36, 66, 168, 106, 8, modalBg);
  carrier->display.drawRoundRect(36, 66, 168, 106, 8, redBorder);
  carrier->display.drawRoundRect(38, 68, 164, 102, 6, redInner);

  // 1. GAME OVER Header
  carrier->display.setTextSize(2);
  carrier->display.setTextColor(redBorder, modalBg);
  const char* header = "GAME OVER";
  int headerW = strlen(header) * 12;
  carrier->display.setCursor((SCREEN_WIDTH - headerW) / 2, 76);
  carrier->display.print(header);

  // 2. Final Score with broken heart indicator
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(textWhite, modalBg);
  char buf[32];
  snprintf(buf, sizeof(buf), "Score: %u  [ 0 LIV ]", pong.getScore());
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

void UiPongRenderer::clearFireballAndTrail() {
  if (!carrier) return;
  uint16_t nativeBg = 0x0000;

  if (lastBallX != -1 && lastBallY != -1) {
    carrier->display.fillRect(lastBallX - 2, lastBallY - 2,
                              PongLogic::BALL_SIZE + 4, PongLogic::BALL_SIZE + 4, nativeBg);
    lastBallX = -1;
    lastBallY = -1;
  }
  for (uint8_t i = 0; i < TRAIL_LEN; i++) {
    if (trailX[i] != -1 && trailY[i] != -1) {
      carrier->display.fillRect(trailX[i] - 2, trailY[i] - 2,
                                PongLogic::BALL_SIZE + 4, PongLogic::BALL_SIZE + 4, nativeBg);
      trailX[i] = -1;
      trailY[i] = -1;
    }
  }
}

void UiPongRenderer::updateSparks(bool& mascotOverlapped) {
  if (!carrier) return;
  uint16_t nativeBg   = 0x0000;
  uint16_t borderCyan = color565(0, 240, 255);

  for (uint8_t i = 0; i < MAX_SPARKS; i++) {
    if (sparks[i].life == 0) continue;

    // Erase previous spark pixel (2x2)
    int16_t px = sparks[i].prevX;
    int16_t py = sparks[i].prevY;
    carrier->display.fillRect(px, py, 2, 2, nativeBg);

    if (px >= 100 && px <= 140 && py >= 56 && py <= 95) {
      mascotOverlapped = true;
    }
    if (py <= PongLogic::FIELD_TOP + 2) {
      carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_TOP,
                                     PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderCyan);
    }
    if (py >= PongLogic::FIELD_BOTTOM - 2) {
      carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_BOTTOM,
                                     PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderCyan);
    }

    sparks[i].life--;
    if (sparks[i].life == 0) continue;

    // Advance spark
    sparks[i].x += sparks[i].vx;
    sparks[i].y += sparks[i].vy;
    sparks[i].prevX = sparks[i].x;
    sparks[i].prevY = sparks[i].y;

    // Draw only inside court bounds
    if (sparks[i].x > PongLogic::FIELD_LEFT + 6 &&
        sparks[i].x < PongLogic::FIELD_RIGHT - 4 &&
        sparks[i].y > PongLogic::FIELD_TOP + 1 &&
        sparks[i].y < PongLogic::FIELD_BOTTOM - 2) {
      carrier->display.fillRect(sparks[i].x, sparks[i].y, 2, 2, sparks[i].color);
    } else {
      sparks[i].life = 0;
    }
  }
}

void UiPongRenderer::updateDisplay(const PongLogic& pong, bool forceRedraw) {
  if (!carrier) return;

  if (forceRedraw || !isInitialized) {
    drawInitialScreen(pong);
  }

  uint16_t nativeBg      = 0x0000;
  uint16_t borderCyan    = color565(0, 240, 255);
  uint16_t wallRedDotted = color565(255, 60, 80);

  if (cheerTimer > 0) {
    cheerTimer--;
  }

  bool paddleNeedsRedraw = false;
  bool botNeedsRedraw    = false;
  bool mascotOverlapped  = false;

  // Trigger paddle redraw during flash animation
  if (paddleFlashTimer > 0) {
    paddleNeedsRedraw = true;
  }

  // State transitions
  PongState currentState = pong.getState();
  if (currentState != lastState) {
    if (lastState == PONG_STATE_READY && currentState == PONG_STATE_PLAYING) {
      clearReadyPrompt();
      clearFireballAndTrail();
      paddleNeedsRedraw = true;
      botNeedsRedraw    = true;
      mascotOverlapped  = true;
    } else if (lastState == PONG_STATE_READY) {
      clearReadyPrompt();
      paddleNeedsRedraw = true;
      botNeedsRedraw    = true;
    }

    if (currentState == PONG_STATE_READY) {
      drawReadyPrompt();
    } else if (currentState == PONG_STATE_BALL_LOST) {
      clearFireballAndTrail();
      paddleNeedsRedraw = true;
      botNeedsRedraw    = true;
      mascotOverlapped  = true;
      // Red stadium alert ring & MISS! badge
      carrier->display.drawCircle(120, 120, 114, color565(255, 45, 60));
      carrier->display.drawCircle(120, 120, 112, color565(180, 20, 40));
      carrier->display.fillRoundRect(92, 110, 56, 20, 4, color565(180, 25, 45));
      carrier->display.drawRoundRect(92, 110, 56, 20, 4, 0xFFFF);
      carrier->display.setTextSize(1);
      carrier->display.setTextColor(0xFFFF, color565(180, 25, 45));
      carrier->display.setCursor(104, 116);
      carrier->display.print("MISS!");
    } else if (currentState == PONG_STATE_GAMEOVER) {
      drawGameOverScreen(pong);
      lastState = currentState;
      return;
    }

    if (lastState == PONG_STATE_BALL_LOST && currentState != PONG_STATE_BALL_LOST) {
      // Restore stadium guide rings to calm neon cyan/blue
      carrier->display.drawCircle(120, 120, 114, color565(0, 70, 110));
      carrier->display.drawCircle(120, 120, 112, color565(0, 140, 200));
      // Erase MISS! prompt
      carrier->display.fillRect(90, 108, 60, 24, nativeBg);
      // Restore center net
      for (int y = 106; y <= 134; y += 6) {
        carrier->display.drawFastVLine(120, y, 3, color565(30, 65, 115));
      }
      paddleNeedsRedraw = true;
      botNeedsRedraw    = true;
    }
    lastState = currentState;
  }

  if (currentState == PONG_STATE_GAMEOVER) {
    return; // Don't redraw court while modal is up
  }

  // Detect score increment to trigger gold flash
  if (pong.getScore() != lastScore && lastScore != 9999) {
    scoreFlashTimer = 8;
  }
  if (scoreFlashTimer > 0) {
    scoreFlashTimer--;
    if (scoreFlashTimer == 0) {
      drawHud(pong, true);
    }
  }

  // 1. HUD updates
  drawHud(pong, false);

  // 2. Wall Ricochet Sparks update
  updateSparks(mascotOverlapped);

  // 2b. Wall shockwave ripple along the laser rail
  if (wallRippleTimer > 0) {
    int16_t rw = (5 - wallRippleTimer) * 8; // expands: 8, 16, 24, 32
    int16_t rx1 = (wallRippleX - rw < PongLogic::FIELD_LEFT) ? (int16_t)PongLogic::FIELD_LEFT : (int16_t)(wallRippleX - rw);
    int16_t rx2 = (wallRippleX + rw > PongLogic::FIELD_RIGHT) ? (int16_t)PongLogic::FIELD_RIGHT : (int16_t)(wallRippleX + rw);
    uint16_t rippleColor = (wallRippleTimer >= 3) ? 0xFFFF : color565(120, 240, 255);
    carrier->display.drawFastHLine(rx1, wallRippleY, rx2 - rx1 + 1, rippleColor);

    wallRippleTimer--;
    if (wallRippleTimer == 0) {
      // Restore affected rail back to standard cyan neon
      carrier->display.drawFastHLine(rx1, wallRippleY, rx2 - rx1 + 1, borderCyan);
    }
  }

  // 3. Blazing Fireball & Flame Trail delta-erasure & rendering
  {
    int16_t newBallX = (int16_t)pong.getBallX();
    int16_t newBallY = (int16_t)pong.getBallY();
    if (newBallX != lastBallX || newBallY != lastBallY) {
      // Erase oldest tail segment and previous fireball aura
      int16_t erasePtsX[2] = { trailX[TRAIL_LEN - 1], lastBallX };
      int16_t erasePtsY[2] = { trailY[TRAIL_LEN - 1], lastBallY };

      for (uint8_t e = 0; e < 2; e++) {
        int16_t ex = erasePtsX[e];
        int16_t ey = erasePtsY[e];
        if (ex == -1 || ey == -1) continue;

        carrier->display.fillRect(ex - 2, ey - 2,
                                  PongLogic::BALL_SIZE + 4, PongLogic::BALL_SIZE + 4, nativeBg);

        if (ex <= PongLogic::PADDLE_X + PongLogic::PADDLE_WIDTH + 3) {
          paddleNeedsRedraw = true;
        }
        if (ex + PongLogic::BALL_SIZE + 3 >= PongLogic::FIELD_RIGHT - 4) {
          botNeedsRedraw = true;
        }
        if (ey <= PongLogic::FIELD_TOP + 3) {
          carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_TOP,
                                         PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderCyan);
        }
        if (ey + PongLogic::BALL_SIZE + 3 >= PongLogic::FIELD_BOTTOM - 1) {
          carrier->display.drawFastHLine(PongLogic::FIELD_LEFT, PongLogic::FIELD_BOTTOM,
                                         PongLogic::FIELD_RIGHT - PongLogic::FIELD_LEFT + 1, borderCyan);
        }
        if (ex >= 98 && ex <= 142 && ey >= 54 && ey <= 96) {
          mascotOverlapped = true;
        }
        if (ex <= 122 && ex + PongLogic::BALL_SIZE + 2 >= 118) {
          for (int y = PongLogic::FIELD_TOP + 4; y < PongLogic::FIELD_BOTTOM - 2; y += 6) {
            if ((y < 60 || y > 94) && y >= ey - 4 && y <= ey + PongLogic::BALL_SIZE + 4) {
              carrier->display.drawFastVLine(120, y, 3, color565(30, 65, 115));
            }
          }
        }
      }

      // Shift trail history
      if (currentState == PONG_STATE_PLAYING && lastBallX != -1) {
        for (int i = TRAIL_LEN - 1; i > 0; i--) {
          trailX[i] = trailX[i - 1];
          trailY[i] = trailY[i - 1];
        }
        trailX[0] = lastBallX;
        trailY[0] = lastBallY;
      } else {
        for (uint8_t i = 0; i < TRAIL_LEN; i++) {
          trailX[i] = -1;
          trailY[i] = -1;
        }
      }

      // Check if new ball overlaps Lumi mascot
      if (newBallX >= 98 && newBallX <= 142 && newBallY >= 54 && newBallY <= 96) {
        mascotOverlapped = true;
      }

      lastBallX = newBallX;
      lastBallY = newBallY;
    }
  }

  // 4. Redraw Lumi Cheerleader in background if mood changed or ball/sparks crossed over him
  drawCheerleaderLumi(pong, mascotOverlapped);

  // 5. Render Flame Trail & Blazing Fireball on top of court background
  if (lastBallX != -1 && lastBallY != -1) {
    if (currentState == PONG_STATE_PLAYING) {
      // Tail segment 2 (oldest ember: deep crimson-violet)
      if (trailX[2] != -1) {
        carrier->display.fillRect(trailX[2] + 1, trailY[2] + 1, 2, 2, color565(190, 25, 30));
      }
      // Tail segment 1 (mid flame: bright orange-red)
      if (trailX[1] != -1) {
        carrier->display.fillRect(trailX[1] + 1, trailY[1] + 1, 3, 3, color565(255, 95, 15));
      }
      // Tail segment 0 (inner flame tongue: blazing gold)
      if (trailX[0] != -1) {
        carrier->display.fillCircle(trailX[0] + 2, trailY[0] + 2, 2, color565(255, 195, 30));
      }
    }

    // Main Fireball: outer crimson-orange flame halo + solar yellow core + diamond white center
    int16_t bcx = lastBallX + 2;
    int16_t bcy = lastBallY + 2;
    carrier->display.fillCircle(bcx, bcy, 3, color565(255, 65, 15));
    carrier->display.fillRect(lastBallX + 1, lastBallY + 1, 3, 3, color565(255, 235, 70));
    carrier->display.drawPixel(bcx, bcy, 0xFFFF);
  }

  // 6. Player Paddle (Left, electric plasma) delta-rendering
  int16_t newPaddleY = (int16_t)pong.getPaddleY();
  if (newPaddleY != lastPaddleY || paddleNeedsRedraw) {
    if (newPaddleY != lastPaddleY && lastPaddleY != -1) {
      carrier->display.fillRect(PongLogic::PADDLE_X - 1, lastPaddleY - 1,
                                PongLogic::PADDLE_WIDTH + 2, PongLogic::PADDLE_HEIGHT + 2, nativeBg);
    }

    if (paddleFlashTimer > 0) {
      // Blinding white-hot impact flash!
      carrier->display.drawRoundRect(PongLogic::PADDLE_X - 1, newPaddleY - 1,
                                    PongLogic::PADDLE_WIDTH + 2, PongLogic::PADDLE_HEIGHT + 2, 2, 0xFFFF);
      carrier->display.fillRoundRect(PongLogic::PADDLE_X, newPaddleY,
                                    PongLogic::PADDLE_WIDTH, PongLogic::PADDLE_HEIGHT, 2, 0xFFFF);
      carrier->display.fillRect(PongLogic::PADDLE_X + 1, newPaddleY + 1,
                                PongLogic::PADDLE_WIDTH - 2, 2, color565(255, 230, 80));
      carrier->display.fillRect(PongLogic::PADDLE_X + 1, newPaddleY + PongLogic::PADDLE_HEIGHT - 3,
                                PongLogic::PADDLE_WIDTH - 2, 2, color565(255, 230, 80));
      paddleFlashTimer--;
      if (paddleFlashTimer == 0) {
        paddleNeedsRedraw = true; // Restore normal cartridge next frame
      }
    } else {
      // High-Tech Next-Gen Plasma Cartridge (segmented cyan)
      carrier->display.drawRoundRect(PongLogic::PADDLE_X - 1, newPaddleY - 1,
                                    PongLogic::PADDLE_WIDTH + 2, PongLogic::PADDLE_HEIGHT + 2, 2, color565(0, 100, 180));
      carrier->display.fillRoundRect(PongLogic::PADDLE_X, newPaddleY,
                                    PongLogic::PADDLE_WIDTH, PongLogic::PADDLE_HEIGHT, 2, color565(0, 240, 255));
      // Top and bottom electrode caps (ice cyan)
      carrier->display.fillRect(PongLogic::PADDLE_X + 1, newPaddleY,
                                PongLogic::PADDLE_WIDTH - 2, 2, color565(180, 255, 255));
      carrier->display.fillRect(PongLogic::PADDLE_X + 1, newPaddleY + PongLogic::PADDLE_HEIGHT - 2,
                                PongLogic::PADDLE_WIDTH - 2, 2, color565(180, 255, 255));
      // Central crystalline lightguide spine
      carrier->display.drawFastVLine(PongLogic::PADDLE_X + 2, newPaddleY + 3,
                                    PongLogic::PADDLE_HEIGHT - 6, 0xFFFF);
      // Cartridge micro-slits
      int16_t slit1 = newPaddleY + PongLogic::PADDLE_HEIGHT / 3;
      int16_t slit2 = newPaddleY + (PongLogic::PADDLE_HEIGHT * 2) / 3;
      carrier->display.drawFastHLine(PongLogic::PADDLE_X + 1, slit1, PongLogic::PADDLE_WIDTH - 2, color565(0, 80, 140));
      carrier->display.drawFastHLine(PongLogic::PADDLE_X + 1, slit2, PongLogic::PADDLE_WIDTH - 2, color565(0, 80, 140));
    }

    lastPaddleY = newPaddleY;
  }

  // 7. AI Bot Opponent Paddle (Right, solar flame cartridge) delta-rendering
  int16_t targetBotY = (int16_t)(pong.getBallY() + (PongLogic::BALL_SIZE - PongLogic::PADDLE_HEIGHT) * 0.5f);
  const int16_t minBotY = PongLogic::FIELD_TOP + 2;
  const int16_t maxBotY = PongLogic::FIELD_BOTTOM - PongLogic::PADDLE_HEIGHT - 2;
  if (targetBotY < minBotY) targetBotY = minBotY;
  if (targetBotY > maxBotY) targetBotY = maxBotY;

  if (targetBotY != lastBotY || botNeedsRedraw) {
    const int16_t botX = PongLogic::FIELD_RIGHT - 2;
    if (targetBotY != lastBotY && lastBotY != -1) {
      carrier->display.fillRect(botX - 1, lastBotY - 1,
                                PongLogic::PADDLE_WIDTH + 2, PongLogic::PADDLE_HEIGHT + 2, nativeBg);
    }
    // High-Tech Solar Flame Cartridge
    carrier->display.drawRoundRect(botX - 1, targetBotY - 1,
                                  PongLogic::PADDLE_WIDTH + 2, PongLogic::PADDLE_HEIGHT + 2, 2, color565(180, 60, 10));
    carrier->display.fillRoundRect(botX, targetBotY,
                                  PongLogic::PADDLE_WIDTH, PongLogic::PADDLE_HEIGHT, 2, color565(255, 140, 20));
    // Solar gold electrode caps
    carrier->display.fillRect(botX + 1, targetBotY,
                              PongLogic::PADDLE_WIDTH - 2, 2, color565(255, 230, 80));
    carrier->display.fillRect(botX + 1, targetBotY + PongLogic::PADDLE_HEIGHT - 2,
                              PongLogic::PADDLE_WIDTH - 2, 2, color565(255, 230, 80));
    // Core lightguide
    carrier->display.drawFastVLine(botX + 2, targetBotY + 3,
                                  PongLogic::PADDLE_HEIGHT - 6, 0xFFFF);
    // Cartridge micro-slits
    int16_t slit1 = targetBotY + PongLogic::PADDLE_HEIGHT / 3;
    int16_t slit2 = targetBotY + (PongLogic::PADDLE_HEIGHT * 2) / 3;
    carrier->display.drawFastHLine(botX + 1, slit1, PongLogic::PADDLE_WIDTH - 2, color565(140, 40, 0));
    carrier->display.drawFastHLine(botX + 1, slit2, PongLogic::PADDLE_WIDTH - 2, color565(140, 40, 0));

    lastBotY = targetBotY;
  }

  // Restore left danger dotted line if ball was near left boundary
  if (currentState == PONG_STATE_PLAYING || currentState == PONG_STATE_BALL_LOST) {
    if (lastBallX <= PongLogic::FIELD_LEFT + 8 || pong.getBallX() <= (float)(PongLogic::FIELD_LEFT + 8)) {
      for (int16_t y = PongLogic::FIELD_TOP + 2; y < PongLogic::FIELD_BOTTOM; y += 4) {
        carrier->display.drawPixel(PongLogic::FIELD_LEFT, y, wallRedDotted);
      }
    }
  }
}
