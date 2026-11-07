// WHAT: Implementation of the 2.5D procedural character renderer for Lumi
// WHY:  Renders Lumi (Tamagotchi Paradise + Zelda Kokiri + Nintendo style) with zero flicker
// USED BY: TamaPal.ino

#include "ui_pet.h"

// Stage viewport constants (fits inside 240x240 display between top menu Y=67 and bottom guide Y=200)
static const int STAGE_X = 38;
static const int STAGE_Y = 70;
static const int STAGE_W = 164;
static const int STAGE_H = 126;

static const int LUMI_BASE_CX = 120;
static const int LUMI_BASE_CY = 131;

UiPetRenderer::UiPetRenderer()
  : carrier(nullptr),
    lastMoodText(nullptr),
    lastAnimTickMs(0),
    lastDrawnCx(LUMI_BASE_CX),
    lastDrawnCy(LUMI_BASE_CY),
    lastNaviX(-1),
    lastNaviY(-1),
    lastSmallFairyY(-1),
    lastZzzPhase(-1),
    lastExpression(EXPR_CONTENT),
    lastStage(STAGE_BABY),
    lastOutfit(OUTFIT_DEFAULT),
    lastWeather(SKY_DAY_SUNNY),
    stageInitialized(false),
    lastBlinking(false),
    lastPromptBlink(false),
    lastHadPoop(false),
    lastWasBathing(false),
    lastBubbleStep(-1),
    bathEndTimeMs(0),
    screenShakeEndTimeMs(0),
    novaStartTimeMs(0),
    novaDurationMs(0),
    lastNovaRadius(0),
    morphStartTimeMs(0),
    morphDurationMs(0),
    lastMorphStep(-1),
    lastMoteStep(-1),
    celestialAuraActive(false),
    lastShowcaseAct(0),
    lastShowcaseTitle(nullptr) {}

void UiPetRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
  stageInitialized = false;
}

LumiExpression UiPetRenderer::determineExpression(const PetLogic& pet) {
  if (!pet.isAlive()) {
    return EXPR_DEAD;
  }
  if (pet.isSleeping()) {
    return EXPR_SLEEPING;
  }
  if (pet.isSick()) {
    return EXPR_SICK;
  }
  if (pet.isOverheating()) {
    return EXPR_OVERHEATING;
  }
  if (pet.getWarmth() < PET_CRITICAL_THRESHOLD) {
    return EXPR_FREEZING;
  }
  if (pet.getHunger() < PET_CRITICAL_THRESHOLD) {
    return EXPR_HUNGRY;
  }
  if (pet.getEnergy() < PET_CRITICAL_THRESHOLD) {
    return EXPR_EXHAUSTED;
  }
  if (pet.getHappiness() < PET_CRITICAL_THRESHOLD) {
    return EXPR_SAD;
  }
  if (pet.getOverallHealth() >= 80) {
    return EXPR_SUPER_HAPPY;
  }
  return EXPR_CONTENT;
}

SkyWeatherMode UiPetRenderer::determineSkyWeather(const PetLogic& pet, LumiExpression expr) {
  if (expr == EXPR_DEAD) {
    return SKY_NIGHT_STARRY;
  }
  if (expr == EXPR_SLEEPING || pet.isSleeping()) {
    return SKY_NIGHT_STARRY;
  }
  if (expr == EXPR_FREEZING || pet.getWarmth() < 45) {
    return SKY_WINTER_SNOW;
  }
  return SKY_DAY_SUNNY;
}

void UiPetRenderer::drawSnowflake(int x, int y, uint16_t color) {
  if (carrier == nullptr) return;
  // Crisp 5x5 pixel crystal snowflake (+ cross with bright center & diagonal tips)
  carrier->display.drawFastHLine(x - 2, y, 5, color);
  carrier->display.drawFastVLine(x, y - 2, 5, color);
  carrier->display.drawPixel(x - 1, y - 1, color);
  carrier->display.drawPixel(x + 1, y - 1, color);
  carrier->display.drawPixel(x - 1, y + 1, color);
  carrier->display.drawPixel(x + 1, y + 1, color);
  carrier->display.drawPixel(x, y, 0xFFFF);
}

void UiPetRenderer::updateSkyWeather(SkyWeatherMode weather, int phase, uint16_t bgColor, bool force) {
  if (carrier == nullptr) return;
  if (!force && phase == lastZzzPhase && weather == lastWeather) {
    return;
  }

  // Clear the 4 dedicated sky weather patches (all outside Lumi's & Navi's boxes)
  // Patch A: Upper-left sky (X=46..62, Y=72..86)
  carrier->display.fillRect(46, 72, 18, 15, bgColor);
  // Patch B: Upper-mid-left sky (X=86..100, Y=71..85)
  carrier->display.fillRect(86, 71, 15, 15, bgColor);
  // Patch C: Upper-mid-right sky (X=142..156, Y=71..85)
  carrier->display.fillRect(142, 71, 15, 15, bgColor);
  // Patch D: Upper-right celestial corner for Sun / Moon / Big Snowflake (X=164..190, Y=70..92)
  carrier->display.fillRect(164, 70, 26, 22, bgColor);

  if (weather == SKY_DAY_SUNNY) {
    // 1. Radiant Nintendo Sun at (176, 80): warm circle + ring of pulsing dot rays
    int sunX = 176;
    int sunY = 80;
    uint16_t sunCore   = color565(255, 245, 130);
    uint16_t sunRim    = color565(255, 190, 55);
    uint16_t rayBright = color565(255, 230, 95);
    uint16_t raySoft   = color565(210, 155, 45);

    carrier->display.fillCircle(sunX, sunY, 5, sunRim);
    carrier->display.fillCircle(sunX, sunY, 3, sunCore);

    // 8 surrounding ray dots that gently breathe outward/inward with phase
    int cardinalDist = (phase % 2 == 0) ? 8 : 9;
    int diagDist     = (phase % 2 == 0) ? 6 : 5;
    uint16_t cColor  = (phase % 2 == 0) ? rayBright : raySoft;
    uint16_t dColor  = (phase % 2 == 0) ? raySoft   : rayBright;

    // Cardinal ray dots (N, S, W, E)
    carrier->display.drawPixel(sunX, sunY - cardinalDist, cColor);
    carrier->display.drawPixel(sunX, sunY + cardinalDist, cColor);
    carrier->display.drawPixel(sunX - cardinalDist, sunY, cColor);
    carrier->display.drawPixel(sunX + cardinalDist, sunY, cColor);

    // Diagonal ray dots (NW, NE, SW, SE)
    carrier->display.drawPixel(sunX - diagDist, sunY - diagDist, dColor);
    carrier->display.drawPixel(sunX + diagDist, sunY - diagDist, dColor);
    carrier->display.drawPixel(sunX - diagDist, sunY + diagDist, dColor);
    carrier->display.drawPixel(sunX + diagDist, sunY + diagDist, dColor);

    // Subtle warm forest sunbeams / pollen motes in upper sky
    uint16_t moteGold = color565(190, 230, 110);
    carrier->display.drawPixel(52, 78 + (phase % 2), moteGold);
    carrier->display.drawPixel(92, 75 + ((phase + 1) % 2), raySoft);
    carrier->display.drawPixel(147, 76 + (phase % 2), moteGold);

  } else if (weather == SKY_NIGHT_STARRY) {
    // 2. Starry Night: Glowing Crescent Moon at (176, 80) + twinkling stars
    int moonX = 176;
    int moonY = 80;
    uint16_t moonGold = color565(255, 240, 150);
    carrier->display.fillCircle(moonX, moonY, 5, moonGold);
    carrier->display.fillCircle(moonX - 3, moonY - 2, 4, bgColor); // Cutout creates crescent moon!

    uint16_t starBright = 0xFFFF;
    uint16_t starCyan   = color565(130, 215, 255);
    uint16_t starDim    = color565(70, 115, 170);

    // Twinkling star 1 at (53, 79)
    if (phase % 2 == 0) {
      carrier->display.drawFastHLine(51, 79, 5, starCyan);
      carrier->display.drawFastVLine(53, 77, 5, starCyan);
      carrier->display.drawPixel(53, 79, starBright);
    } else {
      carrier->display.drawPixel(53, 79, starDim);
    }

    // Twinkling star 2 at (92, 76)
    if ((phase + 1) % 2 == 0) {
      carrier->display.drawFastHLine(90, 76, 5, starCyan);
      carrier->display.drawFastVLine(92, 74, 5, starCyan);
      carrier->display.drawPixel(92, 76, starBright);
    } else {
      carrier->display.drawPixel(92, 76, starDim);
    }

    // Twinkling star 3 at (148, 77)
    carrier->display.drawPixel(148, 77, (phase % 2 == 0) ? starBright : starDim);
    carrier->display.drawPixel(166, 86, ((phase + 1) % 2 == 0) ? starCyan : starDim);

  } else if (weather == SKY_WINTER_SNOW) {
    // 3. Winter Cold: Falling/drifting crystal snowflakes across the sky
    uint16_t snowCyan  = color565(150, 230, 255);
    uint16_t snowLight = color565(210, 245, 255);

    int driftA = (phase % 3) * 2;
    int driftB = ((phase + 1) % 3) * 2;
    int driftC = ((phase + 2) % 3) * 2;

    drawSnowflake(54,  76 + driftA, snowCyan);
    drawSnowflake(92,  75 + driftB, snowLight);
    drawSnowflake(148, 75 + driftC, snowCyan);
    drawSnowflake(176, 78 + driftA, snowLight);
  }
}

void UiPetRenderer::drawForestStage(SkyWeatherMode weather, bool isDead) {
  if (carrier == nullptr) return;

  // Use exact native screen background color (8, 12, 26) so there is NO square box behind Lumi!
  uint16_t nativeBg = color565(8, 12, 26);
  carrier->display.fillRect(STAGE_X, STAGE_Y, STAGE_W, STAGE_H, nativeBg);

  // 2.5D Mossy Kokiri Forest Glade Platform (Y = 153..169)
  // Frosty snow-covered platform in winter, lush emerald moss otherwise
  uint16_t mossShadow = color565(14, 48, 36);
  uint16_t mossBase   = color565(26, 86, 56);
  uint16_t mossTop    = (weather == SKY_WINTER_SNOW) ? color565(150, 215, 245) : color565(56, 148, 86);
  uint16_t mossEdge   = (weather == SKY_WINTER_SNOW) ? color565(230, 250, 255) : color565(110, 215, 125);

  carrier->display.fillRoundRect(52, 155, 136, 14, 7, mossShadow);
  carrier->display.fillRoundRect(54, 153, 132, 13, 6, mossBase);
  carrier->display.fillRoundRect(58, 153, 124, 6, 3, mossTop);
  carrier->display.drawFastHLine(66, 153, 108, mossEdge);

  if (isDead) {
    // Cenotaph / Memorial Gravestone in center of glade (X = 106..134, Y = 126..154)
    uint16_t stoneBase   = color565(75, 85, 100);
    uint16_t stoneShadow = color565(45, 52, 65);
    uint16_t stoneEdge   = color565(115, 130, 150);
    uint16_t leafCarve   = color565(50, 60, 75);
    uint16_t leafTint    = color565(85, 150, 115);

    // Stone stele with rounded arch top
    carrier->display.fillRoundRect(106, 126, 28, 28, 5, stoneBase);
    carrier->display.drawRoundRect(106, 126, 28, 28, 5, stoneEdge);
    carrier->display.drawFastHLine(104, 153, 32, stoneShadow);

    // Carved Kokiri leaf engraving on stone
    carrier->display.drawLine(120, 133, 120, 143, leafCarve);
    carrier->display.fillCircle(120, 137, 3, leafCarve);
    carrier->display.drawPixel(120, 137, leafTint);
    carrier->display.drawLine(118, 139, 120, 136, leafTint);
    carrier->display.drawLine(122, 139, 120, 136, leafTint);

    // Ground moss creeps onto stone base
    carrier->display.drawFastHLine(107, 152, 6, color565(56, 148, 86));
    carrier->display.drawFastHLine(125, 152, 7, color565(56, 148, 86));
  } else {
    // Soft ground shadow directly beneath Lumi (anchors 2.5D depth)
    uint16_t dropShadow = color565(12, 42, 30);
    carrier->display.fillRoundRect(96, 154, 48, 5, 2, dropShadow);
  }

  // Left flora: Glowing Kokiri Magic Mushroom (X = 63..75, Y = 143..154)
  uint16_t stemCream   = color565(235, 230, 210);
  uint16_t capCyan     = color565(40, 215, 200);
  uint16_t capSpot     = color565(210, 255, 245);
  carrier->display.fillRect(67, 149, 4, 5, stemCream);
  carrier->display.fillRoundRect(63, 144, 12, 6, 3, capCyan);
  carrier->display.drawPixel(66, 146, capSpot);
  carrier->display.drawPixel(71, 145, capSpot);

  // Right flora: Tiny Deku / Silent Princess Flower & grass blades (X = 166..179, Y = 145..154)
  uint16_t grassGreen  = color565(80, 190, 100);
  uint16_t petalBlue   = color565(150, 225, 255);
  uint16_t flowerCore  = color565(255, 230, 80);
  carrier->display.drawLine(171, 153, 169, 148, grassGreen);
  carrier->display.drawLine(172, 153, 172, 147, grassGreen);
  carrier->display.drawLine(173, 153, 176, 149, grassGreen);
  carrier->display.fillCircle(172, 146, 3, petalBlue);
  carrier->display.drawPixel(172, 146, flowerCore);
}

void UiPetRenderer::drawMoodBanner(const char* moodText, LumiExpression expr) {
  if (carrier == nullptr || moodText == nullptr) return;

  // Compact rounded status pill at Y = 173..193
  uint16_t pillBg     = color565(14, 24, 44);
  uint16_t pillBorder = color565(55, 130, 175);
  uint16_t textColor  = color565(225, 245, 255);

  if (strstr(moodText, "<3") != nullptr) {
    pillBorder = color565(255, 105, 180); // Warm romantic pink for petting
    textColor  = color565(255, 195, 225);
  } else if (expr == EXPR_OVERHEATING) {
    pillBorder = color565(255, 60, 60);
    textColor  = color565(255, 180, 180);
  } else if (expr == EXPR_FREEZING || expr == EXPR_HUNGRY || expr == EXPR_EXHAUSTED || expr == EXPR_SAD) {
    pillBorder = color565(255, 140, 60);
    textColor  = color565(255, 220, 150);
  } else if (expr == EXPR_SUPER_HAPPY || expr == EXPR_EATING || expr == EXPR_PLAYING) {
    pillBorder = color565(90, 230, 150);
    textColor  = color565(190, 255, 215);
  } else if (expr == EXPR_SLEEPING) {
    pillBorder = color565(100, 135, 220);
    textColor  = color565(190, 210, 255);
  }

  carrier->display.fillRoundRect(46, 173, 148, 20, 8, pillBg);
  carrier->display.drawRoundRect(46, 173, 148, 20, 8, pillBorder);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(textColor);

  int len = (int)strlen(moodText);
  int textX = 120 - (len * 3);
  if (textX < 52) textX = 52;
  carrier->display.setCursor(textX, 179);
  carrier->display.print(moodText);
}

void UiPetRenderer::drawMemorialCard(const PetLogic& pet, unsigned long nowMs, bool force) {
  if (carrier == nullptr) return;

  uint16_t cardBg     = color565(12, 18, 34);
  uint16_t cardBorder = color565(55, 85, 125);
  uint16_t goldColor  = color565(255, 215, 70);
  uint16_t coralColor = color565(255, 125, 115);
  uint16_t textDim    = color565(180, 205, 225);
  uint16_t statColor  = color565(110, 150, 195);

  if (force) {
    carrier->display.fillRoundRect(33, 160, 174, 64, 7, cardBg);
    carrier->display.drawRoundRect(33, 160, 174, 64, 7, cardBorder);

    carrier->display.setTextSize(1);

    // Line 1: * <NAME> HVILER * (GEN X)
    char titleBuf[32];
    snprintf(titleBuf, sizeof(titleBuf), "* %s HVILER * (GEN %u)", pet.getName(), (unsigned int)pet.getGeneration());
    int tLen = (int)strlen(titleBuf);
    int tX = 120 - (tLen * 3);
    if (tX < 36) tX = 36;
    carrier->display.setTextColor(goldColor);
    carrier->display.setCursor(tX, 164);
    carrier->display.print(titleBuf);

    // Line 2: Cause of death
    const char* causeStr = pet.getDeathCauseName();
    int cLen = (int)strlen(causeStr);
    int cX = 120 - (cLen * 3);
    if (cX < 36) cX = 36;
    carrier->display.setTextColor(coralColor);
    carrier->display.setCursor(cX, 174);
    carrier->display.print(causeStr);

    // Line 3: Epitaph quote
    const char* epitaph = pet.getDeathEpitaph();
    char epBuf[36];
    snprintf(epBuf, sizeof(epBuf), "\"%s\"", epitaph);
    int epLen = (int)strlen(epBuf);
    int epX = 120 - (epLen * 3);
    if (epX < 36) epX = 36;
    carrier->display.setTextColor(textDim);
    carrier->display.setCursor(epX, 184);
    carrier->display.print(epBuf);

    // Line 4: Alder & record stats
    char statBuf[36];
    uint16_t bestScore = pet.getHighScorePong();
    if (pet.getHighScoreMaze() > 0 && (bestScore == 0 || pet.getHighScoreMaze() < bestScore)) {
      snprintf(statBuf, sizeof(statBuf), "Alder: %lu dage | Lab: %us", (unsigned long)pet.getAgeMinutes(), (unsigned int)pet.getHighScoreMaze());
    } else {
      snprintf(statBuf, sizeof(statBuf), "Alder: %lu dage | Pong: %u", (unsigned long)pet.getAgeMinutes(), (unsigned int)bestScore);
    }
    int sLen = (int)strlen(statBuf);
    int sX = 120 - (sLen * 3);
    if (sX < 36) sX = 36;
    carrier->display.setTextColor(statColor);
    carrier->display.setCursor(sX, 195);
    carrier->display.print(statBuf);
  }

  // Line 5: Blinking prompt [OK] NYT AEG | T4: MINDE (500 ms toggle)
  bool promptBlink = ((nowMs / 500) % 2 == 0);
  if (force || promptBlink != lastPromptBlink) {
    carrier->display.fillRect(38, 207, 164, 12, cardBg);
    carrier->display.setTextSize(1);
    carrier->display.setCursor(51, 209);
    if (promptBlink) {
      carrier->display.setTextColor(color565(60, 240, 150)); // Bright emerald
    } else {
      carrier->display.setTextColor(color565(25, 75, 45));   // Dim green
    }
    carrier->display.print("[OK] NYT AEG | T4: MINDE");
    lastPromptBlink = promptBlink;
  }
}

void UiPetRenderer::eraseOldCharacterRim(int oldCx, int oldCy, int newCx, int newCy, uint16_t bgColor) {
  if (carrier == nullptr) return;
  int dx = newCx - oldCx;
  int dy = newCy - oldCy;
  if (dx == 0 && dy == 0) return;

  // Character & bed bounding box relative to (cx, cy): X in [-36..+37], Y in [-35..+22]
  if (dy > 0) {
    // Moved down: clear top strip exposed above sprout, ears, horns, or bed headboard
    carrier->display.fillRect(oldCx - 36, oldCy - 35, 74, dy + 2, bgColor);
  } else if (dy < 0) {
    // Moved up: clear bottom strip above the mossy platform (Y < 153)
    int clearH = (-dy) + 2;
    int clearY = newCy + 20;
    if (clearY + clearH > 153) {
      clearH = 153 - clearY;
    }
    if (clearH > 0) {
      carrier->display.fillRect(oldCx - 36, clearY, 74, clearH, bgColor);
    }
  }

  if (dx > 0) {
    // Moved right: clear left margin strip (including headphones and side paws)
    carrier->display.fillRect(oldCx - 36, oldCy - 35, dx + 4, 57, bgColor);
  } else if (dx < 0) {
    // Moved left: clear right margin strip (including headphones and briefcase)
    carrier->display.fillRect(newCx + 25, oldCy - 35, (-dx) + 12, 57, bgColor);
  }
}

void UiPetRenderer::drawFaceFeatures(int cx, int cy, LumiExpression expr, bool isBlinking, PetStage stage, PetOutfit outfit, uint16_t bodyColor, bool wipeEyesOnly) {
  if (carrier == nullptr) return;
  (void)outfit;

  uint16_t eyeDark     = color565(16, 22, 38);
  uint16_t irisEmerald = color565(45, 195, 165);
  uint16_t irisBlue    = color565(75, 175, 255);
  uint16_t pureWhite   = 0xFFFF;
  uint16_t cheekPink   = color565(255, 130, 155);
  uint16_t cheekCoral  = color565(255, 95, 120);
  uint16_t cheekIce    = color565(100, 185, 255);
  uint16_t cheekHot    = color565(255, 60, 60);
  uint16_t mouthDark   = color565(28, 45, 58);
  uint16_t tonguePink  = color565(255, 115, 140);

  int leftEyeX  = (stage == STAGE_BABY) ? (cx - 7) : (cx - 10);
  int rightEyeX = (stage == STAGE_BABY) ? (cx + 7) : (cx + 10);
  int eyeY      = (stage == STAGE_BABY) ? (cy - 3) : (cy - 4);

  if (wipeEyesOnly) {
    // Fast flicker-free blink: wipe ONLY the two compact eye boxes without touching cheeks, mouth, beard or hats
    int eyeW = (stage == STAGE_BABY) ? 9 : 10;
    int eyeH = (stage == STAGE_BABY) ? 10 : 11;
    carrier->display.fillRect(leftEyeX - 4, eyeY - 5, eyeW, eyeH, bodyColor);
    carrier->display.fillRect(rightEyeX - 4, eyeY - 5, eyeW, eyeH, bodyColor);
  } else {
    // Rosy kawaii cheeks (extra round & rosy for baby!)
    uint16_t activeCheek = cheekPink;
    if (expr == EXPR_SUPER_HAPPY || expr == EXPR_EATING || expr == EXPR_PLAYING || expr == EXPR_SCOLDED) {
      activeCheek = cheekCoral;
    } else if (expr == EXPR_FREEZING) {
      activeCheek = cheekIce;
    } else if (expr == EXPR_OVERHEATING) {
      activeCheek = cheekHot;
    }

    if (stage == STAGE_BABY) {
      carrier->display.fillCircle(cx - 12, cy + 2, 3, activeCheek);
      carrier->display.fillCircle(cx + 12, cy + 2, 3, activeCheek);
    } else {
      carrier->display.fillRoundRect(cx - 18, cy + 1, 6, 3, 1, activeCheek);
      carrier->display.fillRoundRect(cx + 12, cy + 1, 6, 3, 1, activeCheek);
    }
  }

  // Sweat drops for overheating
  if (expr == EXPR_OVERHEATING) {
    int dropY = eyeY - 5 + ((int)(lastAnimTickMs / 180) % 4);
    uint16_t dropCyan = color565(90, 220, 255);
    uint16_t dropBlue = color565(25, 125, 215);

    // Primary trickling sweat drop on right temple
    carrier->display.drawPixel(cx + 14, dropY - 2, dropCyan);
    carrier->display.fillRect(cx + 13, dropY - 1, 3, 2, dropCyan);
    carrier->display.fillRect(cx + 12, dropY,     5, 4, dropCyan);
    carrier->display.drawPixel(cx + 13, dropY,     pureWhite);
    carrier->display.drawPixel(cx + 14, dropY + 3, dropBlue);

    // Secondary bead of perspiration on left brow
    carrier->display.drawPixel(cx - 13, eyeY - 6, dropCyan);
    carrier->display.drawPixel(cx - 14, eyeY - 5, pureWhite);
    carrier->display.drawPixel(cx - 13, eyeY - 5, dropCyan);
  }

  // 2. Render Eyes according to expression and blink state
  if (isBlinking || expr == EXPR_SLEEPING || expr == EXPR_DEAD) {
    // Peaceful closed sleeping/blinking eye arcs (-  -)
    carrier->display.drawFastHLine(leftEyeX - 3, eyeY + 1, 7, eyeDark);
    carrier->display.drawFastHLine(leftEyeX - 2, eyeY + 2, 5, eyeDark);
    carrier->display.drawFastHLine(rightEyeX - 3, eyeY + 1, 7, eyeDark);
    carrier->display.drawFastHLine(rightEyeX - 2, eyeY + 2, 5, eyeDark);
  } else if (expr == EXPR_OVERHEATING) {
    // Squinting exhausted panting eyes (>  <)
    carrier->display.drawLine(leftEyeX - 3, eyeY - 2, leftEyeX + 2, eyeY + 1, eyeDark);
    carrier->display.drawLine(leftEyeX + 2, eyeY + 1, leftEyeX - 3, eyeY + 3, eyeDark);
    carrier->display.drawLine(rightEyeX + 3, eyeY - 2, rightEyeX - 2, eyeY + 1, eyeDark);
    carrier->display.drawLine(rightEyeX - 2, eyeY + 1, rightEyeX + 3, eyeY + 3, eyeDark);
  } else if (expr == EXPR_EATING || expr == EXPR_PLAYING) {
    // Joyful Nintendo / Tamagotchi arch eyes (^  ^)
    carrier->display.drawLine(leftEyeX - 3, eyeY + 2, leftEyeX, eyeY - 2, eyeDark);
    carrier->display.drawLine(leftEyeX, eyeY - 2, leftEyeX + 3, eyeY + 2, eyeDark);
    carrier->display.drawLine(leftEyeX - 3, eyeY + 3, leftEyeX, eyeY - 1, eyeDark);
    carrier->display.drawLine(leftEyeX, eyeY - 1, leftEyeX + 3, eyeY + 3, eyeDark);

    carrier->display.drawLine(rightEyeX - 3, eyeY + 2, rightEyeX, eyeY - 2, eyeDark);
    carrier->display.drawLine(rightEyeX, eyeY - 2, rightEyeX + 3, eyeY + 2, eyeDark);
    carrier->display.drawLine(rightEyeX - 3, eyeY + 3, rightEyeX, eyeY - 1, eyeDark);
    carrier->display.drawLine(rightEyeX, eyeY - 1, rightEyeX + 3, eyeY + 3, eyeDark);
  } else if (expr == EXPR_FREEZING) {
    // Tight shivering squint (>  <)
    carrier->display.drawLine(leftEyeX - 3, eyeY - 2, leftEyeX + 2, eyeY + 1, eyeDark);
    carrier->display.drawLine(leftEyeX + 2, eyeY + 1, leftEyeX - 3, eyeY + 4, eyeDark);
    carrier->display.drawLine(rightEyeX + 3, eyeY - 2, rightEyeX - 2, eyeY + 1, eyeDark);
    carrier->display.drawLine(rightEyeX - 2, eyeY + 1, rightEyeX + 3, eyeY + 4, eyeDark);
  } else {
    // Large Glossy Tamagotchi Paradise / Zelda Anime Eyes
    int eyeW = (stage == STAGE_BABY) ? 8 : 9;
    int eyeH = (stage == STAGE_BABY) ? 9 : 10;
    carrier->display.fillRoundRect(leftEyeX - 4, eyeY - 5, eyeW, eyeH, 4, eyeDark);
    carrier->display.fillRoundRect(rightEyeX - 4, eyeY - 5, eyeW, eyeH, 4, eyeDark);

    // Lower iris color glow
    uint16_t irisColor = (expr == EXPR_HUNGRY || expr == EXPR_SAD) ? irisBlue : irisEmerald;
    carrier->display.fillRect(leftEyeX - 2, eyeY + 1, 5, 3, irisColor);
    carrier->display.fillRect(rightEyeX - 2, eyeY + 1, 5, 3, irisColor);

    // Primary top-left 3x3 white catchlight + secondary bottom-right pixel sparkle
    carrier->display.fillRect(leftEyeX - 3, eyeY - 4, 3, 3, pureWhite);
    carrier->display.drawPixel(leftEyeX + 2, eyeY + 2, pureWhite);

    carrier->display.fillRect(rightEyeX - 3, eyeY - 4, 3, 3, pureWhite);
    carrier->display.drawPixel(rightEyeX + 2, eyeY + 2, pureWhite);

    // Extra star twinkle inside eyes when SUPER_HAPPY
    if (expr == EXPR_SUPER_HAPPY) {
      carrier->display.drawPixel(leftEyeX + 1, eyeY - 2, pureWhite);
      carrier->display.drawPixel(rightEyeX + 1, eyeY - 2, pureWhite);
    }

    // Half-closed droopy eyelids when EXHAUSTED or SICK
    if (expr == EXPR_EXHAUSTED || expr == EXPR_SICK) {
      carrier->display.fillRect(leftEyeX - 4, eyeY - 5, 9, 4, bodyColor);
      carrier->display.drawFastHLine(leftEyeX - 4, eyeY - 1, 9, eyeDark);
      carrier->display.fillRect(rightEyeX - 4, eyeY - 5, 9, 4, bodyColor);
      carrier->display.drawFastHLine(rightEyeX - 4, eyeY - 1, 9, eyeDark);
    }

    // Sad / Scolded eyebrows & shiny tear drop
    if (expr == EXPR_SAD) {
      carrier->display.drawLine(leftEyeX - 4, eyeY - 4, leftEyeX + 2, eyeY - 6, bodyColor);
      carrier->display.drawLine(rightEyeX - 2, eyeY - 6, rightEyeX + 4, eyeY - 4, bodyColor);
      // Shiny cyan tear drop at outer corner of left eye
      carrier->display.fillRect(leftEyeX - 5, eyeY + 3, 2, 3, color565(80, 220, 255));
      carrier->display.drawPixel(leftEyeX - 5, eyeY + 3, pureWhite);
    } else if (expr == EXPR_SCOLDED) {
      // Determined angled eyebrows
      carrier->display.drawLine(leftEyeX - 3, eyeY - 6, leftEyeX + 3, eyeY - 4, eyeDark);
      carrier->display.drawLine(rightEyeX - 3, eyeY - 4, rightEyeX + 3, eyeY - 6, eyeDark);
    } else if (stage == STAGE_TEEN && expr == EXPR_CONTENT) {
      // Teen default: raised swag eyebrow on the right side!
      carrier->display.drawLine(leftEyeX - 3, eyeY - 6, leftEyeX + 3, eyeY - 6, eyeDark);
      carrier->display.drawLine(rightEyeX - 2, eyeY - 8, rightEyeX + 4, eyeY - 6, eyeDark);
    }
  }

  // 2b. Spectacles & Bushy Eyebrows for OLD STAGE
  if (stage == STAGE_OLD) {
    uint16_t glassesColor = (expr == EXPR_DEAD) ? color565(190, 220, 245) : color565(255, 215, 60);
    uint16_t glassGlint   = 0xFFFF;

    // Fluffy wise white elder eyebrows
    carrier->display.fillRoundRect(leftEyeX - 4, eyeY - 8, 9, 3, 1, 0xFFFF);
    carrier->display.fillRoundRect(rightEyeX - 4, eyeY - 8, 9, 3, 1, 0xFFFF);

    // Round golden wireframe spectacles (slips down nose when sweaty/overheating)
    int glassesY = (eyeY - 1) + (expr == EXPR_OVERHEATING ? 2 : 0);
    carrier->display.drawCircle(leftEyeX, glassesY, 5, glassesColor);
    carrier->display.drawCircle(rightEyeX, glassesY, 5, glassesColor);
    // Bridge connecting the round rims
    carrier->display.drawFastHLine(cx - 3, glassesY - 1, 7, glassesColor);

    // Subtle glint on upper lens edge
    carrier->display.drawPixel(leftEyeX - 2, glassesY - 2, glassGlint);
    carrier->display.drawPixel(rightEyeX - 2, glassesY - 2, glassGlint);
  }

  if (wipeEyesOnly) {
    return;
  }

  // 3. Render Mouth / Pacifier / Elder Mustache & Flowing Sage Beard
  int mouthY = cy + 2;

  // Cute white fluffy mochi mustache and flowing sage beard for OLD stage
  if (stage == STAGE_OLD) {
    carrier->display.fillCircle(cx - 3, mouthY, 2, 0xFFFF);
    carrier->display.fillCircle(cx + 3, mouthY, 2, 0xFFFF);
    carrier->display.drawPixel(cx, mouthY - 1, color565(225, 230, 240));

    // Flowing white sage beard tapering down to chest
    carrier->display.fillRoundRect(cx - 4, mouthY + 2, 9, 3, 1, 0xFFFF);
    carrier->display.fillRoundRect(cx - 3, mouthY + 4, 7, 3, 1, 0xFFFF);
    carrier->display.fillRoundRect(cx - 2, mouthY + 6, 5, 3, 1, 0xFFFF);
    carrier->display.drawFastHLine(cx - 1, mouthY + 9, 3, color565(230, 235, 245));
    carrier->display.drawPixel(cx, mouthY + 10, color565(200, 210, 225));
  }

  // If Baby is eating berry, show the berry eating action. Otherwise, Baby wears a cute pacifier/dummy!
  // If Baby is eating berry, show the berry eating action.
  // If Baby is overheating, pacifier drops out so Baby can pant with tongue!
  // Otherwise, Baby wears a cute pacifier/dummy that chatters when freezing!
  if (stage == STAGE_BABY && expr != EXPR_EATING && expr != EXPR_OVERHEATING && expr != EXPR_DEAD) {
    int paciDx = (expr == EXPR_FREEZING && (((int)(lastAnimTickMs / 50)) % 2 == 0)) ? -1 : 0;

    // Cute Baby Pacifier (Dummy)
    uint16_t paciShield = color565(255, 220, 70);  // Bright sunny yellow shield
    uint16_t paciBorder = color565(210, 150, 20);  // Warm outline
    uint16_t paciCenter = color565(255, 110, 130); // Cute coral pink button
    uint16_t paciRing   = color565(255, 245, 170); // Little dangling ring

    // Pacifier shield (oval disk in center mouth)
    carrier->display.fillRoundRect(cx - 5 + paciDx, mouthY - 1, 11, 7, 3, paciShield);
    carrier->display.drawRoundRect(cx - 5 + paciDx, mouthY - 1, 11, 7, 3, paciBorder);

    // Center button knob
    carrier->display.fillCircle(cx + paciDx, mouthY + 2, 2, paciCenter);
    carrier->display.drawPixel(cx - 1 + paciDx, mouthY + 1, pureWhite); // Tiny reflection

    // Hanging little ring handle
    carrier->display.drawCircle(cx + paciDx, mouthY + 5, 2, paciRing);
    return;
  }

  switch (expr) {
    case EXPR_SUPER_HAPPY:
    case EXPR_PLAYING:
      // Joyful open smile with cute pink tongue
      carrier->display.fillRoundRect(cx - 4, mouthY, 9, 5, 2, mouthDark);
      carrier->display.fillRect(cx - 2, mouthY + 2, 5, 2, tonguePink);
      break;

    case EXPR_EATING: {
      // Munching a red Kokiri berry right in front of mouth!
      uint16_t berryRed  = color565(255, 55, 75);
      uint16_t berryLeaf = color565(90, 230, 110);
      carrier->display.fillCircle(cx, mouthY + 2, 4, berryRed);
      carrier->display.drawPixel(cx - 1, mouthY + 1, pureWhite);
      carrier->display.fillRect(cx - 1, mouthY - 3, 3, 2, berryLeaf);
      break;
    }

    case EXPR_DEAD:
      // Peaceful resting smile
      carrier->display.drawLine(cx - 3, mouthY + 1, cx, mouthY + 2, mouthDark);
      carrier->display.drawLine(cx, mouthY + 2, cx + 3, mouthY + 1, mouthDark);
      break;

    case EXPR_CONTENT:
      if (stage == STAGE_TEEN) {
        // Teen: totally swag confident smirk tilted to the right!
        carrier->display.drawLine(cx - 3, mouthY + 1, cx, mouthY + 2, mouthDark);
        carrier->display.drawLine(cx, mouthY + 2, cx + 4, mouthY - 1, mouthDark);
        carrier->display.drawPixel(cx + 4, mouthY - 2, mouthDark);
      } else if (stage == STAGE_ADULT) {
        // Adult: distinguished gentle gentleman smile of a composed professional
        carrier->display.drawLine(cx - 4, mouthY + 1, cx - 1, mouthY + 2, mouthDark);
        carrier->display.drawLine(cx - 1, mouthY + 2, cx + 1, mouthY + 2, mouthDark);
        carrier->display.drawLine(cx + 1, mouthY + 2, cx + 4, mouthY + 1, mouthDark);
        carrier->display.drawPixel(cx - 4, mouthY, mouthDark);
        carrier->display.drawPixel(cx + 4, mouthY, mouthDark);
      } else if (stage == STAGE_OLD) {
        // Old: gentle sage smile resting softly beneath mustache
        carrier->display.drawLine(cx - 3, mouthY + 3, cx, mouthY + 4, mouthDark);
        carrier->display.drawLine(cx, mouthY + 4, cx + 3, mouthY + 3, mouthDark);
      } else {
        // Classic Tamagotchi 'w' cat smile
        carrier->display.drawLine(cx - 4, mouthY, cx - 2, mouthY + 2, mouthDark);
        carrier->display.drawLine(cx - 2, mouthY + 2, cx, mouthY, mouthDark);
        carrier->display.drawLine(cx, mouthY, cx + 2, mouthY + 2, mouthDark);
        carrier->display.drawLine(cx + 2, mouthY + 2, cx + 4, mouthY, mouthDark);
      }
      break;

    case EXPR_SLEEPING:
    case EXPR_HUNGRY:
    case EXPR_EXHAUSTED:
      // Cute small 'o' mouth (breathing / pleading / yawning)
      carrier->display.fillCircle(cx, mouthY + 1, 2, mouthDark);
      carrier->display.drawPixel(cx, mouthY + 1, tonguePink);
      break;

    case EXPR_FREEZING: {
      // Rapid shivering teeth chattering (alternates every 70 ms)
      bool chatterOpen = (((int)(lastAnimTickMs / 70)) % 2 == 0);
      if (chatterOpen) {
        carrier->display.fillRect(cx - 3, mouthY, 7, 3, mouthDark);
        carrier->display.drawFastHLine(cx - 2, mouthY + 1, 5, 0xFFFF); // Visible chattering white teeth
      } else {
        carrier->display.drawPixel(cx - 4, mouthY + 1, mouthDark);
        carrier->display.drawPixel(cx - 3, mouthY, mouthDark);
        carrier->display.drawPixel(cx - 2, mouthY + 1, mouthDark);
        carrier->display.drawPixel(cx - 1, mouthY + 2, mouthDark);
        carrier->display.drawPixel(cx,     mouthY + 1, mouthDark);
        carrier->display.drawPixel(cx + 1, mouthY, mouthDark);
        carrier->display.drawPixel(cx + 2, mouthY + 1, mouthDark);
        carrier->display.drawPixel(cx + 3, mouthY + 2, mouthDark);
      }
      break;
    }

    case EXPR_OVERHEATING: {
      // Heavy panting mouth with protruding tongue heaving in rhythm
      bool pantingBeat = (((int)(lastAnimTickMs / 120)) % 2 == 0);
      int tLen = pantingBeat ? 6 : 4;
      carrier->display.fillRoundRect(cx - 4, mouthY, 8, 6, 2, mouthDark);
      carrier->display.fillRoundRect(cx - 2, mouthY + 3, 5, tLen, 1, tonguePink);
      carrier->display.drawPixel(cx - 1, mouthY + 3 + tLen - 1, 0xFFFF); // saliva gleam
      break;
    }

    case EXPR_SAD:
    case EXPR_SCOLDED:
      // Cute pouty frown arc
      carrier->display.drawFastHLine(cx - 2, mouthY + 1, 5, mouthDark);
      carrier->display.drawPixel(cx - 3, mouthY + 2, mouthDark);
      carrier->display.drawPixel(cx + 3, mouthY + 2, mouthDark);
      break;

    case EXPR_SICK:
      // Pouty mouth with glass thermometer sticking out to the right
      carrier->display.drawFastHLine(cx - 3, mouthY + 1, 4, mouthDark);
      // Thermometer glass tube
      carrier->display.fillRect(cx - 1, mouthY, 11, 2, color565(230, 245, 255));
      // Red mercury inside tube
      carrier->display.drawFastHLine(cx + 4, mouthY, 6, color565(255, 40, 50));
      // Red mercury bulb at tip
      carrier->display.fillCircle(cx + 11, mouthY + 1, 2, color565(255, 30, 40));
      // Floating red medical cross (+) over right ear
      carrier->display.fillRect(cx + 17, cy - 17, 2, 6, color565(255, 40, 50));
      carrier->display.fillRect(cx + 15, cy - 15, 6, 2, color565(255, 40, 50));
      break;
  }
}

void UiPetRenderer::drawSleepingBed(int cx, int cy, PetStage stage, PetOutfit outfit, uint16_t bgColor) {
  if (carrier == nullptr) return;
  (void)bgColor;
  (void)outfit;

  // Breathing offset calculation relative to base position (typically 0 or -1)
  int dy = cy - LUMI_BASE_CY;

  // Color palette for wooden bed:
  uint16_t woodDark    = color565(55, 30, 15);    // Dark chestnut outline / shadow
  uint16_t woodBody    = color565(135, 75, 35);   // Warm polished oak
  uint16_t woodLight   = color565(180, 105, 55);  // Amber wood highlight
  uint16_t goldKnob    = color565(255, 215, 60);  // Brass finials

  // Color palette for pillow and bedding:
  uint16_t pillowWhite = color565(248, 248, 255); // Fluffy plush white pillow
  uint16_t pillowShade = color565(190, 205, 230); // Pillow depth shadow
  uint16_t sheetTrim   = 0xFFFF;                  // Crisp folded sheet rim
  uint16_t sheetShade  = color565(210, 225, 240);
  uint16_t quiltBody   = color565(38, 65, 125);   // Cozy deep midnight indigo quilt
  uint16_t quiltShadow = color565(22, 42, 85);    // Quilt side drape shadow
  uint16_t starGold    = color565(255, 220, 80);  // Embroidered gold stars

  // Color palette for sleeping Lumi:
  uint16_t headMain    = color565(130, 195, 225); // Dreamy Night Teal
  uint16_t headOutline = color565(35, 95, 130);
  uint16_t blushPink   = color565(255, 145, 170); // Warm rosy sleeping cheeks
  uint16_t eyeDark     = color565(20, 45, 70);

  // 1. Wooden Bed Frame - Background Elements (Stationary, anchored to platform Y = 153)
  // Left Headboard Post (X = cx - 35..cx - 31, Y = 114..153)
  carrier->display.fillRoundRect(cx - 35, 116, 5, 37, 2, woodBody);
  carrier->display.drawFastVLine(cx - 35, 116, 37, woodDark);
  carrier->display.drawFastVLine(cx - 33, 117, 35, woodLight);
  // Gold finial on headboard post
  carrier->display.fillCircle(cx - 33, 113, 3, goldKnob);
  carrier->display.drawPixel(cx - 34, 112, 0xFFFF);

  // Headboard Slats Panel (X = cx - 31..cx - 15, Y = 118..148)
  carrier->display.fillRoundRect(cx - 31, 118, 17, 32, 3, woodBody);
  carrier->display.drawFastHLine(cx - 30, 118, 15, woodLight);
  carrier->display.drawFastVLine(cx - 26, 120, 28, woodDark);
  carrier->display.drawFastVLine(cx - 21, 120, 28, woodDark);

  // Right Footboard Post (X = cx + 31..cx + 35, Y = 128..153)
  carrier->display.fillRoundRect(cx + 31, 128, 5, 25, 2, woodBody);
  carrier->display.drawFastVLine(cx + 31, 128, 25, woodDark);
  carrier->display.drawFastVLine(cx + 33, 129, 23, woodLight);
  // Gold finial on footboard post
  carrier->display.fillCircle(cx + 33, 125, 2, goldKnob);
  carrier->display.drawPixel(cx + 33, 124, 0xFFFF);

  // Footboard End Panel
  carrier->display.fillRoundRect(cx + 20, 134, 12, 18, 2, woodBody);
  carrier->display.drawFastHLine(cx + 21, 134, 10, woodLight);

  // 2. Plump Pillow (Nestled on left against headboard, heaves with breathing dy)
  int pillowY = 123 + dy;
  carrier->display.fillRoundRect(cx - 27, pillowY, 23, 16, 5, pillowWhite);
  carrier->display.drawRoundRect(cx - 27, pillowY, 23, 16, 5, pillowShade);
  carrier->display.drawFastHLine(cx - 23, pillowY + 11, 15, pillowShade);

  // 3. Lumi's Sleeping Head (Resting sideways on pillow, breathing at headY)
  int headX = cx - 9;
  int headY = 127 + dy;

  // Chubby cute sleeping head
  carrier->display.fillCircle(headX, headY, 11, headOutline);
  carrier->display.fillCircle(headX, headY, 10, headMain);

  // Peaceful closed sleeping eyes: soft curved arcs (-  -)
  carrier->display.drawFastHLine(headX - 6, headY - 1, 4, eyeDark);
  carrier->display.drawPixel(headX - 4, headY, eyeDark);
  carrier->display.drawFastHLine(headX + 2, headY - 1, 4, eyeDark);
  carrier->display.drawPixel(headX + 4, headY, eyeDark);

  if (stage == STAGE_OLD) {
    uint16_t glassesGold = color565(255, 215, 60);
    // Wise white bushy eyebrows
    carrier->display.fillRoundRect(headX - 7, headY - 4, 5, 2, 1, 0xFFFF);
    carrier->display.fillRoundRect(headX + 1, headY - 4, 5, 2, 1, 0xFFFF);

    // Spectacles frame resting on sleepy nose
    carrier->display.drawCircle(headX - 4, headY - 1, 3, glassesGold);
    carrier->display.drawCircle(headX + 4, headY - 1, 3, glassesGold);
    carrier->display.drawFastHLine(headX - 1, headY - 1, 3, glassesGold);

    // Fluffy white mustache and beard resting above blanket
    carrier->display.fillCircle(headX - 3, headY + 3, 2, 0xFFFF);
    carrier->display.fillCircle(headX + 2, headY + 3, 2, 0xFFFF);
    carrier->display.fillRoundRect(headX - 4, headY + 5, 8, 3, 1, 0xFFFF);
  }

  // Rosy blushing cheeks
  carrier->display.fillCircle(headX - 5, headY + 3, 2, blushPink);
  carrier->display.fillCircle(headX + 4, headY + 3, 2, blushPink);

  // Tiny snoring / breathing mouth
  carrier->display.fillCircle(headX - 1, headY + 3, 1, eyeDark);

  // Soft ear resting on pillow
  carrier->display.fillCircle(headX - 11, headY - 3, 3, headOutline);
  carrier->display.fillCircle(headX - 11, headY - 3, 2, headMain);
  carrier->display.drawPixel(headX - 11, headY - 3, blushPink);

  // Cozy Pointed Nightcap (Natsok) draped over head and pillow
  uint16_t capNavy   = color565(32, 52, 110);
  uint16_t capStripe = color565(225, 235, 255);
  carrier->display.fillRoundRect(headX - 8, headY - 14, 16, 7, 3, capNavy);
  carrier->display.drawFastHLine(headX - 8, headY - 8, 16, capStripe); // Folded rim
  // Cone draped backwards towards headboard
  carrier->display.fillTriangle(headX - 2, headY - 14, headX - 16, headY - 10, headX - 6, headY - 7, capNavy);
  carrier->display.drawLine(headX - 4, headY - 13, headX - 13, headY - 9, capStripe);
  // Fluffy white pompom at tip of nightcap
  carrier->display.fillCircle(headX - 17, headY - 9, 3, 0xFFFF);
  carrier->display.drawPixel(headX - 17, headY - 9, color565(190, 210, 235));

  // Stage-specific cute accessories peeking out from the nightcap
  if (stage == STAGE_BABY) {
    carrier->display.fillRect(headX + 5, headY - 16, 2, 4, color565(45, 165, 75));
    carrier->display.fillCircle(headX + 7, headY - 17, 2, color565(65, 225, 105));
    carrier->display.drawPixel(headX + 7, headY - 18, color565(175, 255, 165));
  } else if (stage == STAGE_TEEN) {
    carrier->display.fillTriangle(headX + 3, headY - 14, headX + 7, headY - 21, headX + 5, headY - 12, color565(255, 220, 60));
    carrier->display.drawPixel(headX + 6, headY - 20, 0xFFFF);
  } else if (stage == STAGE_ADULT || stage == STAGE_OLD) {
    carrier->display.fillCircle(headX + 5, headY - 15, 3, color565(255, 215, 60));
    carrier->display.drawPixel(headX + 5, headY - 16, 0xFFFF);
  }

  // 4. Cozy Quilt / Blanket (Tucked right up to chin at quiltY, heaves with breathing)
  int quiltY = 132 + dy;

  // Folded sheet top trim (Crisp white border under chin)
  carrier->display.fillRoundRect(cx - 15, quiltY, 44, 4, 2, sheetTrim);
  carrier->display.drawFastHLine(cx - 14, quiltY + 3, 42, sheetShade);

  // Two cute stubby paws clutching the blanket rim
  carrier->display.fillCircle(cx - 6, quiltY + 1, 2, headOutline);
  carrier->display.fillCircle(cx - 6, quiltY + 1, 1, headMain);
  carrier->display.fillCircle(cx + 6, quiltY + 1, 2, headOutline);
  carrier->display.fillCircle(cx + 6, quiltY + 1, 1, headMain);

  // Main Quilt body
  int quiltHeight = 149 - (quiltY + 3);
  if (quiltHeight < 10) quiltHeight = 10;
  carrier->display.fillRoundRect(cx - 17, quiltY + 3, 47, quiltHeight, 3, quiltBody);
  carrier->display.drawFastVLine(cx + 29, quiltY + 4, quiltHeight - 1, quiltShadow); // side drape fold
  carrier->display.drawFastHLine(cx - 16, quiltY + quiltHeight + 2, 45, quiltShadow);

  // Embroidered Golden Stars on Blanket
  // Star 1
  carrier->display.drawPixel(cx - 5, quiltY + 7, starGold);
  carrier->display.drawPixel(cx - 6, quiltY + 8, starGold);
  carrier->display.drawPixel(cx - 5, quiltY + 8, 0xFFFF);
  carrier->display.drawPixel(cx - 4, quiltY + 8, starGold);
  carrier->display.drawPixel(cx - 5, quiltY + 9, starGold);
  // Star 2
  carrier->display.drawPixel(cx + 12, quiltY + 6, starGold);
  carrier->display.drawPixel(cx + 11, quiltY + 7, starGold);
  carrier->display.drawPixel(cx + 12, quiltY + 7, 0xFFFF);
  carrier->display.drawPixel(cx + 13, quiltY + 7, starGold);
  carrier->display.drawPixel(cx + 12, quiltY + 8, starGold);
  // Star 3
  carrier->display.drawPixel(cx + 4, quiltY + 12, starGold);
  carrier->display.drawPixel(cx + 3, quiltY + 13, starGold);
  carrier->display.drawPixel(cx + 4, quiltY + 13, 0xFFFF);
  carrier->display.drawPixel(cx + 5, quiltY + 13, starGold);
  carrier->display.drawPixel(cx + 4, quiltY + 14, starGold);

  // 5. Wooden Bed Base Beam & Drop Shadow (Foreground elements for 2.5D depth)
  carrier->display.fillRoundRect(cx - 33, 148, 64, 4, 1, woodBody);
  carrier->display.drawFastHLine(cx - 32, 148, 62, woodLight);
  carrier->display.drawFastHLine(cx - 32, 151, 62, woodDark);
  // Bed drop shadow on mossy glade
  carrier->display.fillRoundRect(cx - 34, 152, 66, 3, 1, color565(12, 35, 24));
}

void UiPetRenderer::drawLumiCharacter(int cx, int cy, LumiExpression expr, bool isBlinking, PetStage stage, PetOutfit outfit, uint16_t bgColor) {
  if (carrier == nullptr) return;
  (void)bgColor;

  // When sleeping: Lumi rests in a cozy wooden bed with pillow, blanket and nightcap!
  if (expr == EXPR_SLEEPING) {
    drawSleepingBed(cx, cy, stage, outfit, bgColor);
    return;
  }

  // Dynamic 2.5D Color Palette based on Lumi's physical/emotional state
  uint16_t bodyMain    = color565(155, 248, 220); // Glowing Kokiri Mint-Cream
  uint16_t bodyOutline = color565(35, 125, 110);  // Deep Jade Outline
  uint16_t bodyShade   = color565(95, 200, 175);  // 2.5D Lower Ambient Shading
  uint16_t bellyPatch  = color565(235, 255, 240); // Soft Cream Mochi Belly
  uint16_t earInner    = color565(255, 165, 185); // Warm Pink Ear Tuft
  uint16_t footColor   = color565(65, 175, 150);  // Jade Paws

  if (expr == EXPR_OVERHEATING) {
    bodyMain    = color565(255, 220, 195); // Warm flushed peach
    bodyOutline = color565(190, 75, 55);   // Warm reddish outline
    bodyShade   = color565(240, 175, 150);
    bellyPatch  = color565(255, 240, 230);
    footColor   = color565(210, 95, 75);
    earInner    = color565(255, 80, 100);  // Very warm red ears
  } else if (expr == EXPR_FREEZING) {
    bodyMain    = color565(150, 215, 255); // Frosty Ice Blue
    bodyOutline = color565(45, 105, 175);
    bodyShade   = color565(105, 175, 235);
    bellyPatch  = color565(225, 245, 255);
    footColor   = color565(80, 145, 215);
  } else if (expr == EXPR_SLEEPING) {
    bodyMain    = color565(130, 195, 225); // Dreamy Night Teal
    bodyOutline = color565(35, 95, 130);
    bodyShade   = color565(85, 150, 185);
    bellyPatch  = color565(200, 235, 245);
    footColor   = color565(60, 130, 165);
  } else if (expr == EXPR_SCOLDED) {
    bodyMain    = color565(180, 240, 215);
    earInner    = color565(255, 110, 130);
  } else if (expr == EXPR_SICK) {
    bodyMain    = color565(190, 235, 205); // Pale sickly green-cream
    bodyOutline = color565(75, 120, 95);
    bodyShade   = color565(145, 195, 160);
    bellyPatch  = color565(225, 245, 230);
    footColor   = color565(120, 175, 135);
    earInner    = color565(230, 140, 150); // Pale droopy ears
  } else if (expr == EXPR_DEAD) {
    bodyMain    = color565(210, 225, 235); // Ethereal Spirit White-Blue
    bodyOutline = color565(110, 140, 165);
    bodyShade   = color565(170, 195, 215);
    bellyPatch  = 0xFFFF;
    footColor   = color565(140, 170, 195);
  }

  // 1. Expressive Forest Spirit Ears (Back Layer)
  if (expr == EXPR_DEAD) {
    // Unified gentle ethereal spirit ears
    carrier->display.fillCircle(cx - 17, cy - 11, 4, bodyOutline);
    carrier->display.fillCircle(cx - 17, cy - 11, 3, bodyMain);
    carrier->display.fillCircle(cx + 17, cy - 11, 4, bodyOutline);
    carrier->display.fillCircle(cx + 17, cy - 11, 3, bodyMain);
  } else if (stage == STAGE_BABY) {
    // Baby stage: Cute tiny rounded nub ears
    carrier->display.fillCircle(cx - 17, cy - 11, 4, bodyOutline);
    carrier->display.fillCircle(cx - 17, cy - 11, 3, bodyMain);
    carrier->display.fillCircle(cx - 17, cy - 11, 2, earInner);

    carrier->display.fillCircle(cx + 17, cy - 11, 4, bodyOutline);
    carrier->display.fillCircle(cx + 17, cy - 11, 3, bodyMain);
    carrier->display.fillCircle(cx + 17, cy - 11, 2, earInner);
  } else {
    // Teen/Adult ears (Perky when happy, slightly drooped when sad/tired, tucked tight when freezing, wilting when overheating)
    int earTipOffsetY = -25;
    if (expr == EXPR_SAD || expr == EXPR_EXHAUSTED || expr == EXPR_SCOLDED) {
      earTipOffsetY = -19;
    } else if (expr == EXPR_FREEZING) {
      earTipOffsetY = -17; // Tucked tight against shivering head
    } else if (expr == EXPR_OVERHEATING) {
      earTipOffsetY = -13; // Wilting limply sideways from heat exhaustion
    }
    // Left Ear
    carrier->display.fillTriangle(cx - 21, cy - 11, cx - 26, cy + earTipOffsetY, cx - 10, cy - 18, bodyOutline);
    carrier->display.fillTriangle(cx - 20, cy - 12, cx - 24, cy + earTipOffsetY + 2, cx - 11, cy - 17, bodyMain);
    carrier->display.fillTriangle(cx - 19, cy - 13, cx - 22, cy + earTipOffsetY + 4, cx - 13, cy - 16, earInner);
    // Right Ear
    carrier->display.fillTriangle(cx + 21, cy - 11, cx + 26, cy + earTipOffsetY, cx + 10, cy - 18, bodyOutline);
    carrier->display.fillTriangle(cx + 20, cy - 12, cx + 24, cy + earTipOffsetY + 2, cx + 11, cy - 17, bodyMain);
    carrier->display.fillTriangle(cx + 19, cy - 13, cx + 22, cy + earTipOffsetY + 4, cx + 13, cy - 16, earInner);
  }

  // 2. Feet or floating ghost tail wisps (Back Layer)
  if (expr == EXPR_DEAD) {
    // Soft floating spirit wisps
    carrier->display.fillCircle(cx - 7, cy + 14, 3, footColor);
    carrier->display.fillCircle(cx + 7, cy + 14, 3, footColor);
    carrier->display.fillCircle(cx, cy + 16, 2, footColor);
  } else if (stage == STAGE_BABY) {
    // Tiny baby button paws
    carrier->display.fillCircle(cx - 10, cy + 16, 4, footColor);
    carrier->display.fillCircle(cx + 10, cy + 16, 4, footColor);
  } else {
    carrier->display.fillRoundRect(cx - 16, cy + 16, 10, 6, 3, footColor);
    carrier->display.fillRoundRect(cx + 6,  cy + 16, 10, 6, 3, footColor);
  }

  // 3. 2.5D Chubby Mochi Body (Single-pass interior fill + perimeter outline to prevent SPI flash)
  if (expr == EXPR_DEAD) {
    // Ethereal feathery angel wings spreading behind Lumi
    uint16_t wingGlow   = color565(160, 210, 245);
    uint16_t wingWhite  = 0xFFFF;
    uint16_t wingShadow = color565(110, 155, 195);

    // Left Angel Wing (3 layered soft feathers spreading gracefully)
    carrier->display.fillTriangle(cx - 14, cy - 4, cx - 27, cy - 15, cx - 22, cy - 3, wingShadow);
    carrier->display.fillTriangle(cx - 14, cy - 4, cx - 26, cy - 14, cx - 21, cy - 3, wingWhite);
    carrier->display.fillTriangle(cx - 14, cy - 2, cx - 28, cy - 8,  cx - 21, cy + 2, wingShadow);
    carrier->display.fillTriangle(cx - 14, cy - 2, cx - 27, cy - 7,  cx - 20, cy + 2, wingWhite);
    carrier->display.fillTriangle(cx - 14, cy,     cx - 25, cy - 1,  cx - 18, cy + 6, wingGlow);
    carrier->display.drawPixel(cx - 26, cy - 13, 0xFFFF);
    carrier->display.drawPixel(cx - 27, cy - 6, 0xFFFF);

    // Right Angel Wing (3 layered soft feathers spreading gracefully)
    carrier->display.fillTriangle(cx + 14, cy - 4, cx + 27, cy - 15, cx + 22, cy - 3, wingShadow);
    carrier->display.fillTriangle(cx + 14, cy - 4, cx + 26, cy - 14, cx + 21, cy - 3, wingWhite);
    carrier->display.fillTriangle(cx + 14, cy - 2, cx + 28, cy - 8,  cx + 21, cy + 2, wingShadow);
    carrier->display.fillTriangle(cx + 14, cy - 2, cx + 27, cy - 7,  cx + 20, cy + 2, wingWhite);
    carrier->display.fillTriangle(cx + 14, cy,     cx + 25, cy - 1,  cx + 18, cy + 6, wingGlow);
    carrier->display.drawPixel(cx + 26, cy - 13, 0xFFFF);
    carrier->display.drawPixel(cx + 27, cy - 6, 0xFFFF);

    // Unified ethereal ghost body (bottom shade + single bright fill + perimeter outline)
    carrier->display.fillRoundRect(cx - 18, cy + 6,  36, 8,  5,  bodyShade);
    carrier->display.fillRoundRect(cx - 18, cy - 16, 36, 27, 11, bodyMain);
    carrier->display.drawRoundRect(cx - 20, cy - 18, 40, 34, 14, bodyOutline);
    carrier->display.drawRoundRect(cx - 19, cy - 17, 38, 32, 13, bodyOutline);
    carrier->display.fillRoundRect(cx - 10, cy - 14, 8,  3,  1,  0xFFFF);
    carrier->display.fillRoundRect(cx - 11, cy + 5,  22, 8,  4,  bellyPatch);
  } else if (stage == STAGE_BABY) {
    if (expr == EXPR_FREEZING) {
      // Baby huddles into a tighter shivering egg (36x30 px instead of 38x34 px)
      carrier->display.fillRoundRect(cx - 16, cy + 6,  32, 8,  5,  bodyShade);
      carrier->display.fillRoundRect(cx - 16, cy - 12, 32, 23, 10, bodyMain);
      carrier->display.drawRoundRect(cx - 18, cy - 14, 36, 30, 13, bodyOutline);
      carrier->display.drawRoundRect(cx - 17, cy - 13, 34, 28, 12, bodyOutline);
      carrier->display.fillRoundRect(cx - 9,  cy - 11, 7,  3,  1,  0xFFFF);
      carrier->display.fillRoundRect(cx - 9,  cy + 5,  18, 6,  3,  bellyPatch);
    } else {
      carrier->display.fillRoundRect(cx - 17, cy + 7,  34, 8,  6,  bodyShade);
      carrier->display.fillRoundRect(cx - 17, cy - 15, 34, 27, 12, bodyMain);
      carrier->display.drawRoundRect(cx - 19, cy - 17, 38, 34, 15, bodyOutline);
      carrier->display.drawRoundRect(cx - 18, cy - 16, 36, 32, 14, bodyOutline);
      carrier->display.fillRoundRect(cx - 10, cy - 14, 8,  3,  1,  0xFFFF);
      carrier->display.drawPixel(cx - 11, cy - 12, 0xFFFF);
      carrier->display.fillRoundRect(cx - 10, cy + 5,  20, 8,  4,  bellyPatch);
    }
  } else {
    if (expr == EXPR_FREEZING) {
      // Teen/Adult/Old hunches shoulders inward (44x35 px instead of 48x39 px)
      carrier->display.fillRoundRect(cx - 20, cy + 8,  40, 8,  6,  bodyShade);
      carrier->display.fillRoundRect(cx - 20, cy - 15, 40, 27, 11, bodyMain);
      carrier->display.drawRoundRect(cx - 22, cy - 17, 44, 35, 14, bodyOutline);
      carrier->display.drawRoundRect(cx - 21, cy - 16, 42, 33, 13, bodyOutline);
      carrier->display.fillRoundRect(cx - 11, cy - 13, 8,  3,  1,  0xFFFF);
      carrier->display.fillRoundRect(cx - 11, cy + 6,  22, 7,  3,  bellyPatch);
    } else {
      carrier->display.fillRoundRect(cx - 22, cy + 9,  44, 8,  6,  bodyShade);
      carrier->display.fillRoundRect(cx - 22, cy - 18, 44, 31, 13, bodyMain);
      carrier->display.drawRoundRect(cx - 24, cy - 20, 48, 39, 16, bodyOutline);
      carrier->display.drawRoundRect(cx - 23, cy - 19, 46, 37, 15, bodyOutline);
      carrier->display.fillRoundRect(cx - 13, cy - 16, 10, 3,  1,  0xFFFF);
      carrier->display.drawPixel(cx - 15, cy - 14, 0xFFFF);
      carrier->display.fillRoundRect(cx - 13, cy + 7,  26, 9,  4,  bellyPatch);
    }
  }

  // 4b. Crimson Business Tie with Gold Clip for ADULT stage
  if (stage == STAGE_ADULT && expr != EXPR_DEAD) {
    uint16_t tieRed      = color565(210, 35, 60);
    uint16_t tieShadow   = color565(140, 20, 40);
    uint16_t tieGoldClip = color565(255, 215, 60);

    if (expr == EXPR_OVERHEATING) {
      // Loosened collar knot, tie askew to the right from heat exhaustion
      carrier->display.fillRoundRect(cx - 2, cy + 3, 6, 3, 1, tieRed);
      carrier->display.fillTriangle(cx - 2, cy + 5, cx + 5, cy + 5, cx + 5, cy + 14, tieRed);
      carrier->display.drawFastHLine(cx + 1, cy + 8, 5, tieGoldClip);
    } else {
      // Collar knot
      carrier->display.fillRoundRect(cx - 3, cy + 1, 6, 4, 1, tieRed);
      carrier->display.drawPixel(cx, cy + 2, color565(255, 120, 140));

      // Pointed tie draping over belly
      carrier->display.fillTriangle(cx - 3, cy + 4, cx + 3, cy + 4, cx, cy + 13, tieRed);
      carrier->display.drawLine(cx, cy + 4, cx, cy + 12, tieShadow);

      // Golden tie bar / clip
      carrier->display.drawFastHLine(cx - 2, cy + 7, 5, tieGoldClip);
      carrier->display.drawPixel(cx + 2, cy + 7, 0xFFFF);
    }
  }

  // 4c. Golden Pocket Watch Chain for OLD stage
  if (stage == STAGE_OLD && expr != EXPR_DEAD) {
    uint16_t watchGold = color565(255, 215, 60);

    // Draped gold chain across belly
    carrier->display.drawLine(cx - 7, cy + 8, cx - 2, cy + 12, watchGold);
    carrier->display.drawLine(cx - 2, cy + 12, cx + 4, cy + 10, watchGold);
    // Little round gold pocket watch tucked into waist
    carrier->display.fillCircle(cx + 6, cy + 10, 2, watchGold);
    carrier->display.drawPixel(cx + 6, cy + 10, 0xFFFF);
  }

  // 4d. Winter Warmth Scarf (Adapts per stage when wearing OUTFIT_WINTER)
  if (outfit == OUTFIT_WINTER && expr != EXPR_DEAD) {
    if (stage == STAGE_BABY) {
      // BABY: Candy-cane red & white striped cozy woolen scarf under the chin
      uint16_t scarfRed   = color565(230, 45, 65);
      uint16_t scarfWhite = 0xFFFF;
      uint16_t scarfDark  = color565(160, 25, 40);

      carrier->display.fillRoundRect(cx - 14, cy + 3, 28, 6, 3, scarfRed);
      carrier->display.fillRect(cx - 8, cy + 3, 4, 6, scarfWhite);
      carrier->display.fillRect(cx + 2, cy + 3, 4, 6, scarfWhite);
      carrier->display.drawFastHLine(cx - 13, cy + 9, 26, scarfDark);

      // Cute dangling tail on the right
      carrier->display.fillRoundRect(cx + 7, cy + 7, 6, 9, 2, scarfRed);
      carrier->display.fillRect(cx + 7, cy + 10, 6, 3, scarfWhite);
      carrier->display.drawFastHLine(cx + 7, cy + 16, 6, scarfWhite);
    } else if (stage == STAGE_TEEN) {
      // TEEN: Gamer cyber-navy scarf with glowing neon cyan stripes & trailing tail
      uint16_t scarfNavy = color565(25, 30, 50);
      uint16_t scarfCyan = color565(0, 230, 255);

      carrier->display.fillRoundRect(cx - 17, cy + 4, 34, 6, 3, scarfNavy);
      carrier->display.fillRect(cx - 10, cy + 4, 4, 6, scarfCyan);
      carrier->display.fillRect(cx + 4,  cy + 4, 4, 6, scarfCyan);

      carrier->display.fillRoundRect(cx + 9, cy + 8, 7, 12, 2, scarfNavy);
      carrier->display.fillRect(cx + 9, cy + 12, 7, 3, scarfCyan);
      carrier->display.drawFastHLine(cx + 9, cy + 20, 7, scarfCyan);
    } else if (stage == STAGE_ADULT) {
      // ADULT: Stately burgundy cashmere scarf with gold embroidery over the chest
      uint16_t scarfBurgundy = color565(180, 25, 45);
      uint16_t scarfGold     = color565(255, 215, 60);
      uint16_t scarfDark     = color565(110, 15, 30);

      carrier->display.fillRoundRect(cx - 18, cy + 4, 36, 7, 3, scarfBurgundy);
      carrier->display.drawFastHLine(cx - 17, cy + 6, 34, scarfGold);
      carrier->display.drawFastHLine(cx - 17, cy + 11, 34, scarfDark);

      carrier->display.fillRoundRect(cx - 5, cy + 9, 10, 9, 2, scarfBurgundy);
      carrier->display.drawFastHLine(cx - 4, cy + 14, 8, scarfGold);
      carrier->display.drawFastHLine(cx - 5, cy + 18, 10, scarfGold);
    } else if (stage == STAGE_OLD) {
      // OLD: Chunky knitted hunter-green & cream grandfather scarf with tassels
      uint16_t scarfGreen = color565(55, 125, 80);
      uint16_t scarfCream = color565(245, 240, 215);

      carrier->display.fillRoundRect(cx - 18, cy + 4, 36, 7, 3, scarfGreen);
      carrier->display.fillRect(cx - 11, cy + 4, 5, 7, scarfCream);
      carrier->display.fillRect(cx + 3,  cy + 4, 5, 7, scarfCream);

      carrier->display.fillRoundRect(cx - 13, cy + 9, 8, 11, 2, scarfGreen);
      carrier->display.fillRect(cx - 13, cy + 13, 8, 3, scarfCream);
      carrier->display.drawFastHLine(cx - 13, cy + 20, 8, scarfCream);
    }
  }

  // 5. Stubby Arms / Paws (Raised in celebration when super happy/playing/eating!)
  if (stage == STAGE_BABY) {
    // Baby tiny cute flipper paws
    if (expr == EXPR_SUPER_HAPPY || expr == EXPR_PLAYING || expr == EXPR_EATING) {
      carrier->display.fillCircle(cx - 20, cy - 5, 3, bodyOutline);
      carrier->display.fillCircle(cx - 20, cy - 5, 2, bodyMain);
      carrier->display.fillCircle(cx + 20, cy - 5, 3, bodyOutline);
      carrier->display.fillCircle(cx + 20, cy - 5, 2, bodyMain);
    } else if (expr == EXPR_FREEZING) {
      // Huddled tight across chest
      carrier->display.fillCircle(cx - 13, cy + 2, 3, bodyOutline);
      carrier->display.fillCircle(cx - 13, cy + 2, 2, bodyMain);
      carrier->display.fillCircle(cx + 13, cy + 2, 3, bodyOutline);
      carrier->display.fillCircle(cx + 13, cy + 2, 2, bodyMain);
    } else if (expr == EXPR_OVERHEATING) {
      // Sprawled out limp on the ground from heat exhaustion
      carrier->display.fillCircle(cx - 22, cy + 5, 3, bodyOutline);
      carrier->display.fillCircle(cx - 22, cy + 5, 2, bodyMain);
      carrier->display.fillCircle(cx + 22, cy + 5, 3, bodyOutline);
      carrier->display.fillCircle(cx + 22, cy + 5, 2, bodyMain);
    } else {
      carrier->display.fillCircle(cx - 20, cy + 1, 3, bodyOutline);
      carrier->display.fillCircle(cx - 20, cy + 1, 2, bodyMain);
      carrier->display.fillCircle(cx + 20, cy + 1, 3, bodyOutline);
      carrier->display.fillCircle(cx + 20, cy + 1, 2, bodyMain);
    }
  } else if (stage == STAGE_ADULT && expr != EXPR_DEAD) {
    // -------------------------------------------------------------
    // ADULT STAGE: Right paw holding Diplomat Briefcase!
    // -------------------------------------------------------------
    uint16_t caseBrown = color565(145, 80, 40);   // Rich leather briefcase body
    uint16_t caseDark  = color565(75, 40, 20);    // Dark leather trim & handle
    uint16_t caseGold  = color565(255, 215, 60);  // Brass lock & corner protectors

    if (expr == EXPR_SUPER_HAPPY || expr == EXPR_PLAYING || expr == EXPR_EATING) {
      // Cheering: Left paw raised high
      carrier->display.fillRoundRect(cx - 27, cy - 7, 6, 8, 3, bodyOutline);
      carrier->display.fillRoundRect(cx - 26, cy - 6, 4, 6, 2, bodyMain);

      // Right paw raised holding briefcase high in celebration!
      int caseX = cx + 18;
      int caseY = cy - 8;
      int caseW = 13;
      int caseH = 10;

      // Leather briefcase body
      carrier->display.fillRoundRect(caseX, caseY, caseW, caseH, 2, caseBrown);
      carrier->display.drawRoundRect(caseX, caseY, caseW, caseH, 2, caseDark);
      carrier->display.drawFastHLine(caseX + 1, caseY + 4, caseW - 2, caseDark);

      // Brass latches and corner guards
      carrier->display.fillRect(caseX + 5, caseY + 3, 3, 3, caseGold);
      carrier->display.drawPixel(caseX + 6, caseY + 4, 0xFFFF);
      carrier->display.drawPixel(caseX, caseY, caseGold);
      carrier->display.drawPixel(caseX + caseW - 1, caseY, caseGold);
      carrier->display.drawPixel(caseX, caseY + caseH - 1, caseGold);
      carrier->display.drawPixel(caseX + caseW - 1, caseY + caseH - 1, caseGold);

      // Handle & raised right paw
      carrier->display.drawRoundRect(caseX + 4, caseY + caseH - 1, 5, 3, 1, caseDark);
      carrier->display.fillRoundRect(cx + 21, cy - 6, 6, 7, 2, bodyOutline);
      carrier->display.fillRoundRect(cx + 22, cy - 5, 4, 5, 1, bodyMain);

    } else if (expr == EXPR_FREEZING) {
      // Cold: Shivering paws tucked against body, holding briefcase close
      carrier->display.fillRoundRect(cx - 21, cy + 2, 6, 6, 2, bodyOutline);
      carrier->display.fillRoundRect(cx + 15, cy + 2, 6, 6, 2, bodyOutline);

      int caseX = cx + 17;
      int caseY = cy + 4;
      int caseW = 13;
      int caseH = 10;
      carrier->display.fillRoundRect(caseX, caseY, caseW, caseH, 2, caseBrown);
      carrier->display.drawRoundRect(caseX, caseY, caseW, caseH, 2, caseDark);
      carrier->display.fillRect(caseX + 5, caseY + 3, 3, 3, caseGold);

    } else if (expr == EXPR_OVERHEATING) {
      // Overheating: Left paw wiping sweaty brow at temple
      carrier->display.fillRoundRect(cx - 23, cy - 8, 6, 7, 2, bodyOutline);
      carrier->display.fillRoundRect(cx - 22, cy - 7, 4, 5, 1, bodyMain);

      // Briefcase slumped down flat on ground
      int caseX = cx + 18;
      int caseY = cy + 7;
      int caseW = 13;
      int caseH = 9;
      carrier->display.fillRoundRect(caseX, caseY, caseW, caseH, 2, caseBrown);
      carrier->display.drawRoundRect(caseX, caseY, caseW, caseH, 2, caseDark);
      carrier->display.fillRect(caseX + 5, caseY + 2, 3, 3, caseGold);

      // Right paw dangling limp beside briefcase
      carrier->display.fillRoundRect(cx + 20, cy + 2, 5, 7, 2, bodyOutline);
      carrier->display.fillRoundRect(cx + 21, cy + 3, 3, 5, 1, bodyMain);

    } else {
      // Composed adult: Left paw relaxed at side
      carrier->display.fillRoundRect(cx - 26, cy - 1, 5, 8, 2, bodyOutline);
      carrier->display.fillRoundRect(cx - 25, cy,     3, 6, 1, bodyMain);

      // Stately Diplomat Briefcase resting beside right paw
      int caseX = cx + 18;
      int caseY = cy + 4;
      int caseW = 13;
      int caseH = 10;

      // Handle arched on top
      carrier->display.drawRoundRect(caseX + 4, caseY - 3, 5, 4, 1, caseDark);

      // Leather briefcase body
      carrier->display.fillRoundRect(caseX, caseY, caseW, caseH, 2, caseBrown);
      carrier->display.drawRoundRect(caseX, caseY, caseW, caseH, 2, caseDark);
      carrier->display.drawFastHLine(caseX + 1, caseY + 4, caseW - 2, caseDark);

      // Center brass lock with bright keyhole shine
      carrier->display.fillRect(caseX + 5, caseY + 3, 3, 3, caseGold);
      carrier->display.drawPixel(caseX + 6, caseY + 4, 0xFFFF);

      // Brass corner protectors
      carrier->display.drawPixel(caseX, caseY, caseGold);
      carrier->display.drawPixel(caseX + caseW - 1, caseY, caseGold);
      carrier->display.drawPixel(caseX, caseY + caseH - 1, caseGold);
      carrier->display.drawPixel(caseX + caseW - 1, caseY + caseH - 1, caseGold);

      // Right paw firmly gripping the handle
      carrier->display.fillRoundRect(caseX + 3, caseY - 2, 7, 4, 2, bodyOutline);
      carrier->display.fillRoundRect(caseX + 4, caseY - 1, 5, 2, 1, bodyMain);
    }
  } else if (stage == STAGE_OLD && expr != EXPR_DEAD) {
    // -------------------------------------------------------------
    // OLD STAGE: Right paw holding Wooden Walking Cane (spadserestok)!
    // -------------------------------------------------------------
    uint16_t caneWood = color565(155, 85, 40);   // Polished mahogany wood shaft
    uint16_t caneDark = color565(85, 45, 20);    // Wood shadow
    uint16_t caneGold = color565(255, 215, 60);  // Brass crook handle & ferrule

    if (expr == EXPR_SUPER_HAPPY || expr == EXPR_PLAYING || expr == EXPR_EATING) {
      // Cheerful: Left paw raised high
      carrier->display.fillRoundRect(cx - 27, cy - 7, 6, 8, 3, bodyOutline);
      carrier->display.fillRoundRect(cx - 26, cy - 6, 4, 6, 2, bodyMain);

      // Right paw waving walking cane merrily in the air!
      int caneX = cx + 20;
      int caneY = cy - 12;

      // Wooden shaft
      carrier->display.fillRect(caneX, caneY, 3, 17, caneWood);
      carrier->display.drawFastVLine(caneX + 2, caneY, 17, caneDark);
      carrier->display.fillRect(caneX, caneY + 15, 3, 2, caneGold);

      // Curved brass crook handle
      carrier->display.fillRoundRect(caneX - 4, caneY - 4, 8, 5, 2, caneGold);
      carrier->display.fillRoundRect(caneX - 2, caneY - 2, 4, 3, 1, bgColor);

      // Right paw holding cane shaft
      carrier->display.fillRoundRect(caneX - 2, cy - 6, 6, 6, 2, bodyOutline);
      carrier->display.fillRoundRect(caneX - 1, cy - 5, 4, 4, 1, bodyMain);

    } else if (expr == EXPR_FREEZING) {
      // Shivering: leaning both paws against cane close to body
      int caneX = cx + 16;
      int caneY = cy;

      carrier->display.fillRect(caneX, caneY, 3, 21, caneWood);
      carrier->display.fillRect(caneX, caneY + 19, 3, 2, caneGold);
      carrier->display.fillRoundRect(caneX - 4, caneY - 4, 8, 5, 2, caneGold);
      carrier->display.fillRoundRect(caneX - 2, caneY - 2, 4, 3, 1, bgColor);

      carrier->display.fillRoundRect(cx - 20, cy + 2, 6, 6, 2, bodyOutline);
      carrier->display.fillRoundRect(cx + 14, cy + 2, 6, 6, 2, bodyOutline);

    } else if (expr == EXPR_OVERHEATING) {
      // Heat exhaustion: leaning heavily forward with both paws slumped on cane crook
      int caneX = cx + 19;
      int caneY = cy + 3;

      carrier->display.fillRect(caneX, caneY, 3, 19, caneWood);
      carrier->display.drawFastVLine(caneX + 2, caneY, 19, caneDark);
      carrier->display.fillRect(caneX, caneY + 17, 3, 2, caneGold);
      carrier->display.fillRoundRect(caneX - 5, caneY - 4, 9, 5, 2, caneGold);
      carrier->display.fillRoundRect(caneX - 3, caneY - 2, 5, 3, 1, bgColor);

      // Slumped paws together on the handle
      carrier->display.fillRoundRect(caneX - 4, caneY - 1, 8, 5, 2, bodyOutline);
      carrier->display.fillRoundRect(caneX - 3, caneY,     6, 3, 1, bodyMain);
      carrier->display.fillRoundRect(cx - 24, cy + 3, 5, 7, 2, bodyOutline);
      carrier->display.fillRoundRect(cx - 23, cy + 4, 3, 5, 1, bodyMain);

    } else {
      // Calm wise elder: Left paw relaxed at side
      carrier->display.fillRoundRect(cx - 26, cy - 1, 5, 8, 2, bodyOutline);
      carrier->display.fillRoundRect(cx - 25, cy,     3, 6, 1, bodyMain);

      // Stately walking cane planted firmly on the moss glade
      int caneX = cx + 21;
      int caneY = cy + 1;

      // Mahogany shaft down to ground (cy + 21)
      carrier->display.fillRect(caneX, caneY, 3, 21, caneWood);
      carrier->display.drawFastVLine(caneX + 2, caneY, 21, caneDark);

      // Brass ferrule tip at ground
      carrier->display.fillRect(caneX, cy + 20, 3, 2, caneGold);

      // Polished brass crook handle
      carrier->display.fillRoundRect(caneX - 5, caneY - 4, 9, 5, 2, caneGold);
      carrier->display.fillRoundRect(caneX - 3, caneY - 2, 5, 3, 1, bgColor);
      carrier->display.drawPixel(caneX - 3, caneY - 3, 0xFFFF); // gleam

      // Right paw resting gently on top of cane crook
      carrier->display.fillRoundRect(caneX - 4, caneY, 6, 5, 2, bodyOutline);
      carrier->display.fillRoundRect(caneX - 3, caneY + 1, 4, 3, 1, bodyMain);
    }
  } else {
    if (expr == EXPR_SUPER_HAPPY || expr == EXPR_PLAYING || expr == EXPR_EATING) {
      // Raised cheering paws
      carrier->display.fillRoundRect(cx - 27, cy - 7, 6, 8, 3, bodyOutline);
      carrier->display.fillRoundRect(cx - 26, cy - 6, 4, 6, 2, bodyMain);
      carrier->display.fillRoundRect(cx + 21, cy - 7, 6, 8, 3, bodyOutline);
      carrier->display.fillRoundRect(cx + 22, cy - 6, 4, 6, 2, bodyMain);
    } else if (expr == EXPR_FREEZING) {
      // Tucked-in shivering paws hugging body
      carrier->display.fillRoundRect(cx - 21, cy + 2, 6, 6, 2, bodyOutline);
      carrier->display.fillRoundRect(cx + 15, cy + 2, 6, 6, 2, bodyOutline);
    } else if (expr == EXPR_OVERHEATING) {
      // Limp paws sprawled down and outwards from heat exhaustion
      carrier->display.fillRoundRect(cx - 28, cy + 4, 6, 7, 2, bodyOutline);
      carrier->display.fillRoundRect(cx - 27, cy + 5, 4, 5, 1, bodyMain);
      carrier->display.fillRoundRect(cx + 22, cy + 4, 6, 7, 2, bodyOutline);
      carrier->display.fillRoundRect(cx + 23, cy + 5, 4, 5, 1, bodyMain);
    } else {
      // Relaxed side paws
      carrier->display.fillRoundRect(cx - 26, cy - 1, 5, 8, 2, bodyOutline);
      carrier->display.fillRoundRect(cx - 25, cy,     3, 6, 1, bodyMain);
      carrier->display.fillRoundRect(cx + 21, cy - 1, 5, 8, 2, bodyOutline);
      carrier->display.fillRoundRect(cx + 22, cy,     3, 6, 1, bodyMain);
    }
  }

  // 6. Facial Features (Eyes, Cheeks, Mouth / Pacifier)
  drawFaceFeatures(cx, cy, expr, isBlinking, stage, outfit, bodyMain, false);

  // 7. Foreground Headgear & Stage Head Props (Drawn after body so hats/earflaps are never overwritten)
  uint16_t stemGreen     = color565(45, 165, 75);
  uint16_t leafEmerald   = color565(65, 225, 105);
  uint16_t leafHighlight = color565(175, 255, 165);

  if (expr == EXPR_DEAD) {
    // Golden Spirit Halo above head
    carrier->display.drawRoundRect(cx - 10, cy - 29, 20, 5, 2, color565(255, 235, 90));
    // Tiny ethereal spirit sprout leaf
    carrier->display.fillRect(cx - 1, cy - 24, 2, 4, stemGreen);
    carrier->display.fillCircle(cx + 2, cy - 25, 3, leafEmerald);
    carrier->display.drawPixel(cx + 2, cy - 26, leafHighlight);

    if (celestialAuraActive) {
      // Celestial radiant halo double ring
      carrier->display.drawRoundRect(cx - 12, cy - 30, 24, 7, 3, color565(255, 215, 60));
      // Rotating golden sparkle on the halo rim
      int haloStep = ((int)(lastAnimTickMs / 80)) % 8;
      const int haloOffsets[8][2] = {
        {10, 0}, {7, 2}, {0, 3}, {-7, 2}, {-10, 0}, {-7, -2}, {0, -3}, {7, -2}
      };
      carrier->display.drawPixel(cx + haloOffsets[haloStep][0], (cy - 27) + haloOffsets[haloStep][1], 0xFFFF);
    }
  } else if (outfit == OUTFIT_WINTER) {
    // =============================================================
    // WINTER WARDROBE: Stage-adapted winter headgear & beanies
    // =============================================================
    if (stage == STAGE_BABY) {
      uint16_t beanieRed   = color565(225, 45, 65);
      uint16_t beanieDark  = color565(155, 25, 40);
      uint16_t beanieWhite = 0xFFFF;

      carrier->display.fillRoundRect(cx - 15, cy - 24, 30, 12, 6, beanieRed);
      carrier->display.drawFastHLine(cx - 13, cy - 20, 26, beanieDark);
      carrier->display.fillRoundRect(cx - 16, cy - 16, 32, 5, 2, beanieRed);
      carrier->display.drawFastHLine(cx - 15, cy - 14, 30, beanieWhite);
      carrier->display.fillCircle(cx, cy - 26, 4, beanieWhite);
      carrier->display.drawPixel(cx, cy - 26, color565(200, 220, 240));
      carrier->display.fillRect(cx + 4, cy - 25, 2, 4, stemGreen);
      carrier->display.fillCircle(cx + 7, cy - 26, 3, leafEmerald);
      carrier->display.drawPixel(cx + 7, cy - 27, leafHighlight);
    } else if (stage == STAGE_TEEN) {
      uint16_t beanieDark  = color565(28, 32, 52);
      uint16_t beanieCyan  = color565(0, 230, 255);
      uint16_t hornDark    = color565(30, 95, 65);
      uint16_t hornGold    = color565(255, 220, 60);

      carrier->display.fillRoundRect(cx - 17, cy - 25, 34, 12, 5, beanieDark);
      carrier->display.fillRoundRect(cx - 18, cy - 17, 36, 5, 2, beanieCyan);
      carrier->display.fillRoundRect(cx + 12, cy - 28, 9, 8, 3, beanieDark);
      carrier->display.fillCircle(cx + 22, cy - 26, 4, beanieCyan);
      carrier->display.drawPixel(cx + 22, cy - 26, 0xFFFF);

      carrier->display.fillTriangle(cx - 5, cy - 24, cx - 11, cy - 34, cx - 8, cy - 21, hornDark);
      carrier->display.fillTriangle(cx - 5, cy - 23, cx - 10, cy - 32, cx - 7, cy - 21, leafEmerald);
      carrier->display.fillTriangle(cx - 8, cy - 30, cx - 11, cy - 34, cx - 10, cy - 28, hornGold);
      carrier->display.drawPixel(cx - 10, cy - 33, 0xFFFF);

      carrier->display.fillTriangle(cx + 2, cy - 24, cx + 10, cy - 34, cx + 6, cy - 21, hornDark);
      carrier->display.fillTriangle(cx + 2, cy - 23, cx + 9, cy - 32, cx + 5, cy - 21, leafEmerald);
      carrier->display.fillTriangle(cx + 7, cy - 30, cx + 10, cy - 34, cx + 9, cy - 28, hornGold);
      carrier->display.drawPixel(cx + 9, cy - 33, 0xFFFF);
    } else if (stage == STAGE_ADULT) {
      uint16_t capTweed = color565(35, 95, 75);
      uint16_t capFur   = color565(240, 245, 250);
      uint16_t capGold  = color565(255, 215, 60);

      carrier->display.fillRoundRect(cx - 15, cy - 26, 30, 13, 5, capTweed);
      carrier->display.fillRoundRect(cx - 17, cy - 18, 34, 6, 3, capFur);
      carrier->display.fillRoundRect(cx - 19, cy - 14, 5, 9, 2, capFur);
      carrier->display.fillRoundRect(cx + 14, cy - 14, 5, 9, 2, capFur);
      carrier->display.fillCircle(cx, cy - 23, 4, capGold);
      carrier->display.fillCircle(cx, cy - 23, 2, color565(255, 140, 40));
      carrier->display.drawPixel(cx - 1, cy - 24, 0xFFFF);
    } else if (stage == STAGE_OLD) {
      uint16_t hatColor     = color565(35, 40, 55);
      uint16_t hatBrim      = color565(25, 28, 40);
      uint16_t hatRibbon    = color565(190, 45, 65);
      uint16_t hatGold      = color565(255, 215, 60);
      uint16_t furWhite     = color565(245, 250, 255);
      uint16_t snowCrystal  = color565(180, 235, 255);

      carrier->display.fillRoundRect(cx - 11, cy - 35, 22, 12, 2, hatColor);
      carrier->display.drawFastVLine(cx - 9, cy - 34, 10, color565(60, 65, 85));
      carrier->display.fillRoundRect(cx - 16, cy - 24, 32, 4, 1, hatBrim);
      carrier->display.fillRect(cx - 11, cy - 26, 22, 3, hatRibbon);
      carrier->display.drawRect(cx - 2, cy - 27, 5, 5, hatGold);
      carrier->display.drawPixel(cx, cy - 25, 0xFFFF);

      carrier->display.fillCircle(cx - 18, cy - 16, 5, furWhite);
      carrier->display.fillCircle(cx - 18, cy - 16, 3, color565(220, 230, 240));
      carrier->display.fillCircle(cx + 18, cy - 16, 5, furWhite);
      carrier->display.fillCircle(cx + 18, cy - 16, 3, color565(220, 230, 240));

      carrier->display.drawPixel(cx + 10, cy - 29, snowCrystal);
      carrier->display.drawPixel(cx + 10, cy - 27, snowCrystal);
      carrier->display.drawPixel(cx + 9,  cy - 28, snowCrystal);
      carrier->display.drawPixel(cx + 11, cy - 28, snowCrystal);
    }
  } else if (stage == STAGE_BABY) {
    carrier->display.fillRect(cx - 1, cy - 23, 2, 5, stemGreen);
    carrier->display.fillCircle(cx + 2, cy - 24, 3, leafEmerald);
    carrier->display.drawPixel(cx + 2, cy - 25, leafHighlight);
  } else if (stage == STAGE_TEEN) {
    uint16_t hornDark   = color565(30, 95, 65);
    uint16_t hornGold   = color565(255, 220, 60);

    carrier->display.fillTriangle(cx - 2, cy - 24, cx - 10, cy - 34, cx - 6, cy - 21, hornDark);
    carrier->display.fillTriangle(cx - 2, cy - 23, cx - 9, cy - 32, cx - 5, cy - 21, leafEmerald);
    carrier->display.fillTriangle(cx - 7, cy - 30, cx - 10, cy - 34, cx - 9, cy - 28, hornGold);
    carrier->display.drawPixel(cx - 9, cy - 33, 0xFFFF);

    carrier->display.fillTriangle(cx + 2, cy - 24, cx + 10, cy - 34, cx + 6, cy - 21, hornDark);
    carrier->display.fillTriangle(cx + 2, cy - 23, cx + 9, cy - 32, cx + 5, cy - 21, leafEmerald);
    carrier->display.fillTriangle(cx + 7, cy - 30, cx + 10, cy - 34, cx + 9, cy - 28, hornGold);
    carrier->display.drawPixel(cx + 9, cy - 33, 0xFFFF);

    carrier->display.fillTriangle(cx - 2, cy - 22, cx, cy - 29, cx + 2, cy - 22, leafHighlight);
  } else if (stage == STAGE_ADULT) {
    uint16_t blossomGold  = color565(255, 215, 60);
    uint16_t blossomAmber = color565(255, 140, 40);

    carrier->display.fillRect(cx - 1, cy - 26, 3, 7, stemGreen);
    carrier->display.fillTriangle(cx - 1, cy - 25, cx - 11, cy - 32, cx - 7, cy - 22, leafEmerald);
    carrier->display.fillTriangle(cx + 1, cy - 25, cx + 11, cy - 32, cx + 7, cy - 22, leafEmerald);
    carrier->display.drawLine(cx - 2, cy - 26, cx - 9, cy - 30, leafHighlight);
    carrier->display.drawLine(cx + 2, cy - 26, cx + 9, cy - 30, leafHighlight);
    carrier->display.fillCircle(cx, cy - 27, 4, blossomGold);
    carrier->display.fillCircle(cx, cy - 27, 2, blossomAmber);
    carrier->display.drawPixel(cx - 1, cy - 28, 0xFFFF);
  } else if (stage == STAGE_OLD) {
    uint16_t hatColor     = color565(35, 40, 55);
    uint16_t hatBrim      = color565(25, 28, 40);
    uint16_t hatRibbon    = color565(190, 45, 65);
    uint16_t hatGold      = color565(255, 215, 60);
    uint16_t featherGreen = color565(65, 225, 105);

    carrier->display.fillRoundRect(cx - 11, cy - 35, 22, 12, 2, hatColor);
    carrier->display.drawFastVLine(cx - 9, cy - 34, 10, color565(60, 65, 85));
    carrier->display.fillRoundRect(cx - 16, cy - 24, 32, 4, 1, hatBrim);
    carrier->display.fillRect(cx - 11, cy - 26, 22, 3, hatRibbon);
    carrier->display.drawRect(cx - 2, cy - 27, 5, 5, hatGold);
    carrier->display.drawPixel(cx, cy - 25, 0xFFFF);
    carrier->display.fillTriangle(cx + 7, cy - 26, cx + 12, cy - 33, cx + 9, cy - 27, featherGreen);
    carrier->display.drawLine(cx + 8, cy - 26, cx + 11, cy - 31, leafHighlight);
  } else {
    carrier->display.fillRect(cx - 1, cy - 26, 3, 7, stemGreen);
    carrier->display.fillTriangle(cx - 1, cy - 25, cx - 10, cy - 31, cx - 7, cy - 23, leafEmerald);
    carrier->display.fillTriangle(cx + 1, cy - 25, cx + 10, cy - 31, cx + 7, cy - 23, leafEmerald);
    carrier->display.drawLine(cx - 2, cy - 26, cx - 8, cy - 29, leafHighlight);
    carrier->display.drawLine(cx + 2, cy - 26, cx + 8, cy - 29, leafHighlight);
  }

  // 8. Swag Gamer/DJ Headphones for TEEN stage!
  if (stage == STAGE_TEEN && expr != EXPR_DEAD) {
    uint16_t hpBand     = color565(30, 35, 55);    // Dark matte headband
    uint16_t hpNeonCyan = color565(0, 230, 255);   // Glowing cyber cyan ear cups
    uint16_t hpAccent   = color565(255, 60, 110);  // Neon pink accent ring

    int hpOffsetY = (expr == EXPR_OVERHEATING) ? 5 : 0; // headphones slipping down neck from heat/sweat

    // Arch headband over the head
    carrier->display.drawCircle(cx, cy - 8 + hpOffsetY, 22, hpBand);
    carrier->display.drawCircle(cx, cy - 8 + hpOffsetY, 23, hpBand);

    // Left DJ Ear Cup (rounded capsule over left ear)
    carrier->display.fillRoundRect(cx - 27, cy - 13 + hpOffsetY, 6, 12, 3, hpBand);
    carrier->display.fillRoundRect(cx - 26, cy - 11 + hpOffsetY, 4, 8, 2, hpNeonCyan);
    carrier->display.drawPixel(cx - 25, cy - 7 + hpOffsetY, hpAccent);

    // Right DJ Ear Cup (rounded capsule over right ear)
    carrier->display.fillRoundRect(cx + 21, cy - 13 + hpOffsetY, 6, 12, 3, hpBand);
    carrier->display.fillRoundRect(cx + 22, cy - 11 + hpOffsetY, 4, 8, 2, hpNeonCyan);
    carrier->display.drawPixel(cx + 24, cy - 7 + hpOffsetY, hpAccent);
  }
}

void UiPetRenderer::updateFairies(int naviX, int naviY, int smallFairyY, bool isSleeping, uint16_t bgColor, bool force) {
  if (carrier == nullptr) return;
  if (!force && naviX == lastNaviX && naviY == lastNaviY && smallFairyY == lastSmallFairyY) {
    return;
  }

  // 1. Primary Fairy: Navi (Cyan-White orb with wings around X=75, Y=96)
  if (lastNaviX >= 0 && lastNaviY >= 0 && (lastNaviX != naviX || lastNaviY != naviY)) {
    carrier->display.fillRect(lastNaviX - 10, lastNaviY - 9, 21, 18, bgColor);
  }
  lastNaviX = naviX;
  lastNaviY = naviY;

  uint16_t wingColor = isSleeping ? color565(110, 145, 200) : color565(200, 225, 255);
  uint16_t auraCyan  = isSleeping ? color565(40, 120, 190)  : color565(65, 220, 255);
  uint16_t coreWhite = 0xFFFF;

  // Upper wings
  carrier->display.fillTriangle(naviX - 2, naviY - 1, naviX - 8, naviY - 7, naviX - 4, naviY - 1, wingColor);
  carrier->display.fillTriangle(naviX + 2, naviY - 1, naviX + 8, naviY - 7, naviX + 4, naviY - 1, wingColor);
  // Lower small wings
  carrier->display.drawPixel(naviX - 5, naviY + 2, wingColor);
  carrier->display.drawPixel(naviX + 5, naviY + 2, wingColor);
  // Glowing cyan halo + bright white center
  carrier->display.fillCircle(naviX, naviY, 4, auraCyan);
  carrier->display.fillCircle(naviX, naviY, 2, coreWhite);

  // 2. Secondary Miniature Forest Spirit Fairy (Tatl-inspired warm gold-pink fairy at X=53, Y=120..125)
  int sfX = 53;
  if (lastSmallFairyY >= 0 && lastSmallFairyY != smallFairyY) {
    carrier->display.fillRect(sfX - 6, lastSmallFairyY - 5, 13, 11, bgColor);
  }
  lastSmallFairyY = smallFairyY;

  uint16_t tatlAura = isSleeping ? color565(150, 110, 160) : color565(255, 190, 95);
  carrier->display.drawLine(sfX - 2, smallFairyY - 1, sfX - 5, smallFairyY - 4, wingColor);
  carrier->display.drawLine(sfX + 2, smallFairyY - 1, sfX + 5, smallFairyY - 4, wingColor);
  carrier->display.fillCircle(sfX, smallFairyY, 2, tatlAura);
  carrier->display.drawPixel(sfX, smallFairyY, coreWhite);
}

void UiPetRenderer::updateSideOverlay(int baseCx, int baseCy, LumiExpression expr, int phase, uint16_t bgColor, bool force) {
  if (carrier == nullptr) return;
  if (!force && phase == lastZzzPhase && expr == lastExpression) {
    return;
  }

  // Dedicated right-side overlay zone below the Sun/Moon: X = 152..188, Y = 94..134
  int boxX = baseCx + 32; // 152
  int boxY = baseCy - 37; // 94
  carrier->display.fillRect(boxX, boxY, 36, 40, bgColor);

  if (expr == EXPR_SLEEPING) {
    // Floating animated 'z Z' sleep letters
    uint16_t zzzLight = color565(150, 210, 255);
    uint16_t zzzDim   = color565(85, 140, 210);
    carrier->display.setTextSize(1);

    if (phase >= 1) {
      carrier->display.setTextColor(zzzDim);
      carrier->display.setCursor(boxX + 3, boxY + 26);
      carrier->display.print("z");
    }
    if (phase >= 2) {
      carrier->display.setTextColor(zzzLight);
      carrier->display.setCursor(boxX + 11, boxY + 14);
      carrier->display.print("Z");
    }
    if (phase >= 3) {
      carrier->display.setTextColor(0xFFFF);
      carrier->display.setCursor(boxX + 20, boxY + 3);
      carrier->display.print("Z");
    }
  } else if (expr == EXPR_HUNGRY) {
    // Cute Tamagotchi thought-bubble with a red Kokiri berry inside
    uint16_t bubbleWhite = color565(225, 240, 255);
    carrier->display.fillCircle(boxX + 4, boxY + 26, 2, bubbleWhite);
    carrier->display.fillRoundRect(boxX + 7, boxY + 6, 22, 18, 6, bubbleWhite);
    carrier->display.fillCircle(boxX + 18, boxY + 16, 4, color565(255, 55, 75));
    carrier->display.fillRect(boxX + 17, boxY + 10, 3, 2, color565(70, 200, 90));
  } else if (expr == EXPR_FREEZING) {
    // Extra floating snowflake on the right side when freezing
    int sy = boxY + 12 + (phase % 3) * 3;
    drawSnowflake(boxX + 14, sy, color565(165, 235, 255));
  } else if (expr == EXPR_OVERHEATING) {
    // Floating sweat droplets in the side overlay
    uint16_t dropCyan = color565(80, 215, 255);
    int sy = boxY + 12 + (phase % 3) * 4;
    carrier->display.fillCircle(boxX + 16, sy, 3, dropCyan);
    carrier->display.fillTriangle(boxX + 16, sy - 5, boxX + 13, sy, boxX + 19, sy, dropCyan);
    carrier->display.drawPixel(boxX + 15, sy - 1, 0xFFFF);
  } else if (expr == EXPR_SUPER_HAPPY) {
    // Floating animated Kokiri Love Hearts <3
    uint16_t heartPink = color565(255, 65, 115);
    uint16_t heartCore = color565(255, 185, 210);
    int hy = boxY + 20 - (phase * 4);
    int hx = boxX + 15 + ((phase % 2 == 0) ? -2 : 2);
    carrier->display.drawPixel(hx - 2, hy - 2, heartPink);
    carrier->display.drawPixel(hx,     hy - 2, heartPink);
    carrier->display.fillRect(hx - 3,  hy - 1, 5, 2, heartPink);
    carrier->display.fillRect(hx - 2,  hy + 1, 3, 1, heartPink);
    carrier->display.drawPixel(hx - 1, hy + 2, heartPink);
    carrier->display.drawPixel(hx - 1, hy - 1, heartCore); // Highlight sparkle
  } else if (expr == EXPR_PLAYING) {
    // Twinkling Nintendo star / heart sparkles
    uint16_t starGold = color565(255, 235, 80);
    int sy = boxY + 16 - (phase % 2) * 4;
    carrier->display.drawFastHLine(boxX + 10, sy, 7, starGold);
    carrier->display.drawFastVLine(boxX + 13, sy - 3, 7, starGold);
    carrier->display.drawPixel(boxX + 13, sy, 0xFFFF);
  } else if (expr == EXPR_SCOLDED) {
    // Manga scold / discipline symbol
    uint16_t alertRed = color565(255, 75, 75);
    carrier->display.drawLine(boxX + 8, boxY + 12, boxX + 14, boxY + 18, alertRed);
    carrier->display.drawLine(boxX + 14, boxY + 12, boxX + 8, boxY + 18, alertRed);
  }
}

void UiPetRenderer::updateHomeDisplay(const PetLogic& pet, unsigned long nowMs, bool forceRedraw) {
  if (carrier == nullptr) return;

  bool beingPetted       = pet.isBeingPetted(nowMs);
  LumiExpression expr    = beingPetted ? EXPR_SUPER_HAPPY : determineExpression(pet);
  PetStage stage         = pet.getStageEnum();
  PetOutfit outfit       = pet.getOutfitEnum();
  SkyWeatherMode weather = determineSkyWeather(pet, expr);
  bool isNight           = (weather == SKY_NIGHT_STARRY);

  // Check if bath or evolution morph just finished so we can cleanly restore the stage
  bool currentlyBathing  = isBathing(nowMs);
  bool bathJustEnded     = (lastWasBathing && !currentlyBathing);
  lastWasBathing         = currentlyBathing;
  if (!currentlyBathing) {
    lastBubbleStep = -1;
  }

  bool currentlyMorphing = isMorphing(nowMs);
  bool morphJustEnded    = (lastMorphStep >= 0 && !currentlyMorphing);
  if (!currentlyMorphing) {
    lastMorphStep = -1;
  }

  if (bathJustEnded || morphJustEnded) {
    forceRedraw = true;
  }

  // Dynamic frame rate: 45 ms (~22 FPS) for shivering/panting/morphing, 65 ms (~15 FPS) for calm states
  unsigned long minFrameMs = (currentlyMorphing || expr == EXPR_FREEZING || expr == EXPR_OVERHEATING) ? 45 : 65;
  if (!forceRedraw && stageInitialized && (nowMs - lastAnimTickMs < minFrameMs)) {
    return;
  }
  lastAnimTickMs = nowMs;

  // Match the exact screen background from ui_menu.cpp so there is no rectangular box
  uint16_t nativeBg = color565(8, 12, 26);

  // 1. Compute smooth 1-pixel breathing, happy hop, cold shiver, and panting offsets
  int dx = 0;
  int dy = 0;

  if (expr == EXPR_DEAD) {
    // Ethereal gentle vertical hover oscillation above cenotaph
    const int ghostHover[4] = {0, -1, -2, -1};
    dy = ghostHover[(nowMs / 380) % 4];
    dx = 0;
  } else if (expr == EXPR_FREEZING) {
    // 4-phase micro-tremor spasm: asymmetric involuntary shivering + hunched posture (+1..+2 px)
    const int shiverX[4] = {-1, 1, -1, 1};
    const int shiverY[4] = {2, 1, 2, 1}; // hunched down tight
    dx = shiverX[(nowMs / 65) % 4];
    dy = shiverY[(nowMs / 65) % 4];
  } else if (expr == EXPR_OVERHEATING) {
    // Heavy thermal panting heave: exhausted wilt/slump rhythm
    const int pantingDy[4] = {-1, 0, 1, 0};
    dy = pantingDy[(nowMs / 120) % 4];
    dx = 0;
  } else if (expr == EXPR_SLEEPING) {
    // Slow, deep sleeping respiration (1-px gentle heave)
    const int sleepBreath[4] = {0, 0, -1, -1};
    dy = sleepBreath[(nowMs / 700) % 4];
  } else if (expr == EXPR_SUPER_HAPPY) {
    // Smooth 1-px harmonic Nintendo bounce rhythm (zero jump tearing)
    const int happyHop[4] = {0, -1, -2, -1};
    dy = happyHop[(nowMs / 320) % 4];
  } else {
    // Calm organic idle breathing (1-px gentle chest rise)
    const int idleBreath[4] = {0, 0, -1, -1};
    dy = idleBreath[(nowMs / 520) % 4];
  }

  // Screen shake micro-jitter on impact events
  if (nowMs < screenShakeEndTimeMs) {
    int shake = ((nowMs / 35) % 2 == 0) ? -2 : 2;
    dx += shake;
  }

  int targetCx = LUMI_BASE_CX + dx;
  int targetCy = (expr == EXPR_DEAD) ? (98 + dy) : (LUMI_BASE_CY + dy);

  // Natural eye blink: 140 ms blink every 3.4 seconds (never when dead or sleeping)
  bool isBlinking = (!isNight && expr != EXPR_DEAD) && ((nowMs % 3400) < 140);

  // Fairies gentle floating orbits
  const int naviDx[4] = {0, 1, 0, -1};
  const int naviDy[4] = {0, -1, -2, -1};
  int naviStep        = (nowMs / 360) % 4;
  int targetNaviX     = 75 + naviDx[naviStep];
  int targetNaviY     = (isNight ? 100 : 96) + naviDy[naviStep];
  int targetSmallY    = 123 - naviDy[(naviStep + 2) % 4];

  // Weather & side overlay phase (0..3)
  int overlayPhase = (nowMs / 480) % 4;

  // 1b. Multi-frame Evolution / Stage Transformation Morph Animation
  if (currentlyMorphing) {
    int morphStep = (int)(((nowMs - morphStartTimeMs) * 6) / morphDurationMs);
    if (morphStep > 5) morphStep = 5;
    if (morphStep != lastMorphStep) {
      lastMorphStep = morphStep;
      // Clear character & aura zone cleanly
      carrier->display.fillRect(LUMI_BASE_CX - 38, 72, 76, 70, nativeBg);
      drawForestStage(weather, (expr == EXPR_DEAD));

      int mCy = (expr == EXPR_DEAD) ? 98 : LUMI_BASE_CY;
      uint16_t morphGold = color565(255, 225, 80);
      uint16_t morphCyan = color565(80, 240, 255);
      uint16_t morphGlow = color565(255, 248, 200);

      if (morphStep == 0) {
        // Phase 0: Golden aura rings contracting toward Lumi + warm glowing silhouette
        carrier->display.drawCircle(LUMI_BASE_CX, mCy - 2, 33, morphCyan);
        carrier->display.drawCircle(LUMI_BASE_CX, mCy - 2, 26, morphGold);
        carrier->display.fillRoundRect(LUMI_BASE_CX - 18, mCy - 15, 36, 30, 12, morphGold);
        carrier->display.fillRoundRect(LUMI_BASE_CX - 14, mCy - 12, 28, 24, 10, morphGlow);
      } else if (morphStep == 1) {
        // Phase 1: Pure white-gold energy core compression
        carrier->display.drawCircle(LUMI_BASE_CX, mCy - 2, 28, morphGold);
        carrier->display.fillRoundRect(LUMI_BASE_CX - 20, mCy - 17, 40, 33, 14, morphGlow);
        carrier->display.fillRoundRect(LUMI_BASE_CX - 16, mCy - 14, 32, 26, 11, 0xFFFF);
      } else if (morphStep == 2) {
        // Phase 2: Radiant starburst flash at core
        carrier->display.fillRoundRect(LUMI_BASE_CX - 22, mCy - 18, 44, 35, 15, 0xFFFF);
        carrier->display.drawCircle(LUMI_BASE_CX, mCy - 2, 31, morphCyan);
        carrier->display.drawFastHLine(LUMI_BASE_CX - 34, mCy - 2, 10, morphGold);
        carrier->display.drawFastHLine(LUMI_BASE_CX + 25, mCy - 2, 10, morphGold);
        carrier->display.drawFastVLine(LUMI_BASE_CX, mCy - 34, 10, morphGold);
      } else {
        // Phases 3..5: Newly evolved Lumi emerges inside expanding magic rings!
        drawLumiCharacter(targetCx, targetCy, expr, false, stage, outfit, nativeBg);
        int ringR = 27 + (morphStep - 3) * 4;
        carrier->display.drawCircle(LUMI_BASE_CX, mCy - 2, ringR, (morphStep % 2 == 1) ? morphGold : morphCyan);
      }
    }
    return;
  }

  // 2. Full stage initialization when entering SCREEN_HOME or when weather / stage / outfit changes
  bool weatherChanged = (weather != lastWeather);
  bool stageChanged   = (stage != lastStage);
  bool outfitChanged  = (outfit != lastOutfit);
  bool deathChanged   = ((expr == EXPR_DEAD) != (lastExpression == EXPR_DEAD));
  bool sleepChanged   = ((expr == EXPR_SLEEPING) != (lastExpression == EXPR_SLEEPING));
  if (forceRedraw || !stageInitialized || weatherChanged || stageChanged || outfitChanged || deathChanged || sleepChanged) {
    if (expr == EXPR_DEAD && lastShowcaseAct == 0) {
      // Clear top menu area for peaceful memorial starry sky (when not showing showcase header)
      carrier->display.fillRect(38, 14, 164, 50, nativeBg);
      carrier->display.setTextSize(1);
      carrier->display.setTextColor(color565(120, 150, 200));
      carrier->display.setCursor(74, 32);
      carrier->display.print("* I EVIG HVILE *");
      carrier->display.drawPixel(54, 25, color565(180, 220, 255));
      carrier->display.drawPixel(186, 28, color565(180, 220, 255));
      carrier->display.drawFastHLine(44, 66, 152, color565(35, 50, 80));
    }
    drawForestStage(weather, (expr == EXPR_DEAD));
    updateSkyWeather(weather, overlayPhase, nativeBg, true);
    drawLumiCharacter(targetCx, targetCy, expr, isBlinking, stage, outfit, nativeBg);
    updateFairies(targetNaviX, targetNaviY, targetSmallY, isNight, nativeBg, true);
    updateSideOverlay(LUMI_BASE_CX, LUMI_BASE_CY, expr, overlayPhase, nativeBg, true);

    if (expr == EXPR_DEAD) {
      drawMemorialCard(pet, nowMs, true);
    } else {
      const char* mood = beingPetted ? "LUMI ELSKER DIG! <3" : pet.getMoodStatusDanish();
      drawMoodBanner(mood, expr);
      lastMoodText = mood;
      if (pet.getHasPoop()) {
        drawPoop(168, 145, overlayPhase);
        lastHadPoop = true;
      }
    }

    lastDrawnCx      = targetCx;
    lastDrawnCy      = targetCy;
    lastBlinking     = isBlinking;
    lastExpression   = expr;
    lastStage        = stage;
    lastOutfit       = outfit;
    lastWeather      = weather;
    lastZzzPhase     = overlayPhase;
    stageInitialized = true;
    return;
  }

  // 3. Delta-update Lumi (zero-flicker partial redraw)
  if (targetCx != lastDrawnCx || targetCy != lastDrawnCy || expr != lastExpression || outfit != lastOutfit) {
    if (expr == EXPR_DEAD) {
      carrier->display.fillRect(lastDrawnCx - 30, lastDrawnCy - 32, 61, 52, nativeBg);
    } else if (expr != lastExpression || outfit != lastOutfit) {
      // Expression or outfit changed: clear full character bounds
      carrier->display.fillRect(lastDrawnCx - 31, lastDrawnCy - 35, 65, 57, nativeBg);
      carrier->display.fillRoundRect(96, 154, 48, 5, 2, color565(12, 42, 30)); // drop shadow
    } else {
      eraseOldCharacterRim(lastDrawnCx, lastDrawnCy, targetCx, targetCy, nativeBg);
    }
    drawLumiCharacter(targetCx, targetCy, expr, isBlinking, stage, outfit, nativeBg);
    lastDrawnCx    = targetCx;
    lastDrawnCy    = targetCy;
    lastBlinking   = isBlinking;
    lastExpression = expr;
    lastOutfit     = outfit;
  } else if (isBlinking != lastBlinking) {
    // Only the eyelids changed: wipe ONLY the two compact eye boxes for zero-flicker blinking!
    uint16_t bodyMain = color565(155, 248, 220);
    if (expr == EXPR_OVERHEATING) bodyMain = color565(255, 220, 195);
    else if (expr == EXPR_FREEZING) bodyMain = color565(150, 215, 255);
    else if (expr == EXPR_SLEEPING) bodyMain = color565(130, 195, 225);
    else if (expr == EXPR_SCOLDED) bodyMain = color565(180, 240, 215);
    else if (expr == EXPR_SICK) bodyMain = color565(190, 235, 205);
    else if (expr == EXPR_DEAD) bodyMain = color565(210, 225, 235);

    drawFaceFeatures(targetCx, targetCy, expr, isBlinking, stage, outfit, bodyMain, true);
    lastBlinking = isBlinking;
  }

  // 3b. Delta-update Celestial Aura ascending spirit motes (when deceased in Showcase Act 9)
  if (expr == EXPR_DEAD && celestialAuraActive) {
    int moteStep = (int)((nowMs / 90) % 24);
    if (moteStep != lastMoteStep) {
      const int moteX[4] = {82, 89, 151, 158};
      uint16_t spiritCyan = color565(140, 230, 255);
      if (lastMoteStep >= 0) {
        for (int i = 0; i < 4; i++) {
          int oldMy = 112 - ((lastMoteStep + i * 6) % 24) * 2;
          if (oldMy >= 68 && oldMy <= 114) {
            carrier->display.fillRect(moteX[i] - 1, oldMy - 1, 3, 3, nativeBg);
          }
        }
      }
      for (int i = 0; i < 4; i++) {
        int my = 112 - ((moteStep + i * 6) % 24) * 2;
        if (my >= 68 && my <= 114) {
          uint16_t mc = (i % 2 == 0) ? 0xFFFF : spiritCyan;
          carrier->display.drawPixel(moteX[i], my, mc);
          carrier->display.drawPixel(moteX[i], my - 1, mc);
        }
      }
      lastMoteStep = moteStep;
    }
  } else {
    lastMoteStep = -1;
  }

  // 4. Delta-update Fairies, Sky Weather (Sun/Stars/Snow), and Side Overlay
  bool zzzPhaseChanged = (overlayPhase != lastZzzPhase);
  updateFairies(targetNaviX, targetNaviY, targetSmallY, isNight, nativeBg, false);
  updateSkyWeather(weather, overlayPhase, nativeBg, false);
  updateSideOverlay(LUMI_BASE_CX, LUMI_BASE_CY, expr, overlayPhase, nativeBg, false);
  lastZzzPhase = overlayPhase;
  lastWeather  = weather;

  // 5. Delta-update Danish mood banner or memorial card prompt
  if (expr == EXPR_DEAD) {
    drawMemorialCard(pet, nowMs, false);
  } else {
    const char* currentMood = beingPetted ? "LUMI ELSKER DIG! <3" : pet.getMoodStatusDanish();
    if (currentMood != lastMoodText) {
      drawMoodBanner(currentMood, expr);
      lastMoodText = currentMood;
    }
  }

  // 6. Delta-update Poop (only on phase tick) and Soap Bubbles shower
  if (expr != EXPR_DEAD) {
    const int poopX = 168;
    const int poopY = 145;
    bool hasPoop = pet.getHasPoop();
    if (hasPoop) {
      if (!lastHadPoop || zzzPhaseChanged) {
        drawPoop(poopX, poopY, overlayPhase);
      }
      lastHadPoop = true;
    } else if (lastHadPoop) {
      erasePoop(poopX, poopY, nativeBg);
      lastHadPoop = false;
    }

    if (currentlyBathing) {
      drawSoapBubbles(nowMs);
    }
  }

  // 7. Dynamic Evolution / Hatching Starburst (Nova) - anchored outside Lumi's silhouette
  const int novaCenterX = LUMI_BASE_CX;
  const int novaCenterY = 114;
  if (novaDurationMs > 0 && (nowMs - novaStartTimeMs < novaDurationMs)) {
    unsigned long elapsedNova = nowMs - novaStartTimeMs;
    int radius = 28 + (int)((elapsedNova * 18) / novaDurationMs);
    uint16_t starGold  = color565(255, 230, 90);
    uint16_t starCyan  = color565(90, 240, 255);
    uint16_t starWhite = 0xFFFF;

    if (lastNovaRadius != radius) {
      // Erase previous star ring
      if (lastNovaRadius > 0) {
        for (int i = 0; i < 8; i++) {
          int oldX = novaCenterX + (int)(cos(i * 0.785) * lastNovaRadius);
          int oldY = novaCenterY + (int)(sin(i * 0.785) * (lastNovaRadius * 0.72));
          if (oldX > 36 && oldX < 204 && oldY > 68 && oldY < 138) {
            carrier->display.fillRect(oldX - 1, oldY - 1, 3, 3, nativeBg);
          }
        }
      }

      // Draw crisp expanding 8-diamond star ring around Lumi
      for (int i = 0; i < 8; i++) {
        int sx = novaCenterX + (int)(cos(i * 0.785) * radius);
        int sy = novaCenterY + (int)(sin(i * 0.785) * (radius * 0.72));
        if (sx > 36 && sx < 204 && sy > 68 && sy < 138) {
          uint16_t col = (i % 2 == 0) ? starGold : (i % 4 == 1 ? starCyan : starWhite);
          carrier->display.drawFastHLine(sx - 1, sy, 3, col);
          carrier->display.drawFastVLine(sx, sy - 1, 3, col);
          carrier->display.drawPixel(sx, sy, starWhite);
        }
      }
      lastNovaRadius = radius;
    }
  } else if (lastNovaRadius > 0) {
    // Erase final stars when nova burst finishes and restore Lumi & fairies cleanly
    for (int i = 0; i < 8; i++) {
      int oldX = novaCenterX + (int)(cos(i * 0.785) * lastNovaRadius);
      int oldY = novaCenterY + (int)(sin(i * 0.785) * (lastNovaRadius * 0.72));
      if (oldX > 36 && oldX < 204 && oldY > 68 && oldY < 138) {
        carrier->display.fillRect(oldX - 1, oldY - 1, 3, 3, nativeBg);
      }
    }
    lastNovaRadius = 0;
    novaDurationMs = 0;
    drawLumiCharacter(targetCx, targetCy, expr, isBlinking, stage, outfit, nativeBg);
    updateFairies(targetNaviX, targetNaviY, targetSmallY, isNight, nativeBg, true);
  }
}

void UiPetRenderer::drawActionScene(const PetLogic& pet,
                                    LumiExpression actionExpr,
                                    const char* title,
                                    const char* subtitle,
                                    unsigned long nowMs) {
  if (carrier == nullptr) return;
  (void)nowMs;

  SkyWeatherMode weather = determineSkyWeather(pet, actionExpr);
  bool isNight = (weather == SKY_NIGHT_STARRY);
  uint16_t nativeBg = color565(8, 12, 26);

  // 1. Draw the seamless stage with Lumi performing the action
  lastAnimTickMs = nowMs;
  drawForestStage(weather);
  int actionCy = (actionExpr == EXPR_PLAYING || actionExpr == EXPR_EATING) ? (LUMI_BASE_CY - 3) : LUMI_BASE_CY;
  drawLumiCharacter(LUMI_BASE_CX, actionCy, actionExpr, false, pet.getStageEnum(), pet.getOutfitEnum(), nativeBg);
  updateFairies(74, 98, 123, isNight, nativeBg, true);
  updateSideOverlay(LUMI_BASE_CX, LUMI_BASE_CY, actionExpr, 2, nativeBg, true);

  // 2. Draw compact top action title badge at Y = 71..91 (above Lumi's sprout)
  uint16_t badgeBg     = color565(20, 36, 62);
  uint16_t badgeBorder = color565(90, 230, 150);
  if (actionExpr == EXPR_SCOLDED) {
    badgeBorder = color565(255, 90, 80);
  } else if (actionExpr == EXPR_SLEEPING) {
    badgeBorder = color565(100, 150, 255);
  }

  carrier->display.fillRoundRect(46, 71, 148, 20, 6, badgeBg);
  carrier->display.drawRoundRect(46, 71, 148, 20, 6, badgeBorder);

  if (title != nullptr) {
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(0xFFFF);
    int titleLen = (int)strlen(title);
    int titleX = 120 - (titleLen * 3);
    if (titleX < 52) titleX = 52;
    carrier->display.setCursor(titleX, 77);
    carrier->display.print(title);
  }

  // 3. Draw subtitle in the bottom mood banner pill (Y = 173..193)
  drawMoodBanner(subtitle != nullptr ? subtitle : pet.getMoodStatusDanish(), actionExpr);

  // Mark stage so returning to SCREEN_HOME cleanly refreshes the sky weather
  stageInitialized = false;
}

void UiPetRenderer::drawHatchingScene(uint8_t phase, unsigned long nowMs, uint16_t generation) {
  if (carrier == nullptr) return;

  uint16_t bgColor   = color565(6, 10, 22);
  uint16_t rimColor  = color565(26, 42, 72);
  uint16_t titleGold = color565(255, 215, 80);
  uint16_t subCyan   = color565(120, 215, 255);

  static uint8_t lastDrawnPhase = 255;
  static unsigned long lastHatchFrameMs = 0;
  static int lastEggEx = 120;
  static int lastSparkStep = -1;

  // Detect fresh entry into hatching scene
  if (nowMs < lastHatchFrameMs || (nowMs - lastHatchFrameMs > 600)) {
    lastDrawnPhase = 255;
  }

  bool phaseChanged = (phase != lastDrawnPhase);
  if (!phaseChanged && (nowMs - lastHatchFrameMs < 55)) {
    return; // Throttle to ~18 FPS for smooth flicker-free animation
  }
  lastHatchFrameMs = nowMs;

  if (phaseChanged) {
    if (lastDrawnPhase == 255) {
      carrier->display.fillScreen(bgColor);
      carrier->display.drawCircle(120, 120, 118, rimColor);

      // Twinkling background stars
      carrier->display.drawPixel(38, 35, color565(180, 220, 255));
      carrier->display.drawPixel(55, 22, color565(255, 230, 120));
      carrier->display.drawPixel(185, 24, color565(255, 230, 120));
      carrier->display.drawPixel(202, 36, color565(180, 220, 255));
      carrier->display.drawPixel(28, 80, color565(100, 140, 190));
      carrier->display.drawPixel(212, 82, color565(100, 140, 190));

      // Title (Y = 22)
      carrier->display.setTextSize(1);
      carrier->display.setTextColor(titleGold);
      const char* title = "* ET NYT LIV BEGYNDER *";
      carrier->display.setCursor(120 - ((int)strlen(title) * 3), 22);
      carrier->display.print(title);

      // Subtitle (Y = 36)
      char subBuf[32];
      snprintf(subBuf, sizeof(subBuf), "< LUMI GENERATION %u >", (unsigned int)generation);
      carrier->display.setTextColor(subCyan);
      carrier->display.setCursor(120 - ((int)strlen(subBuf) * 3), 36);
      carrier->display.print(subBuf);

      // Mossy woven nest (X=90..150, Y=132..146)
      carrier->display.fillRoundRect(90, 134, 60, 15, 5, color565(45, 60, 35));
      carrier->display.fillRoundRect(95, 132, 50, 8, 3, color565(75, 150, 65));
      carrier->display.drawPixel(94, 138, color565(210, 180, 70));
      carrier->display.drawPixel(145, 137, color565(210, 180, 70));
      carrier->display.drawPixel(120, 142, color565(95, 185, 80));
    } else {
      // Phase transition (0->1 or 1->2): clear only the egg box & prompt line
      carrier->display.fillRect(82, 66, 76, 65, bgColor);
      carrier->display.fillRect(36, 170, 168, 16, bgColor);
    }

    // Draw phase prompt once on phase change
    carrier->display.setTextSize(1);
    if (phase == 0) {
      carrier->display.setTextColor(color565(190, 215, 235), bgColor);
      const char* p0 = "Aegget vugger roligt...";
      carrier->display.setCursor(120 - ((int)strlen(p0) * 3), 175);
      carrier->display.print(p0);
    } else if (phase == 1) {
      carrier->display.setTextColor(color565(255, 220, 90), bgColor);
      const char* p1 = "KRAK! Skallen revner!";
      carrier->display.setCursor(120 - ((int)strlen(p1) * 3), 175);
      carrier->display.print(p1);
    } else {
      carrier->display.setTextColor(color565(80, 245, 150), bgColor);
      const char* p2 = "Velkommen, Baby Lumi!";
      carrier->display.setCursor(120 - ((int)strlen(p2) * 3), 175);
      carrier->display.print(p2);
    }

    lastDrawnPhase = phase;
    lastEggEx = -1;
    lastSparkStep = -1;
  }

  int cx = 120;
  int cy = 114;

  uint16_t eggShell   = color565(250, 246, 230); // Warm ivory
  uint16_t eggShadow  = color565(215, 205, 185); // Cream shadow
  uint16_t eggSpeckle = color565(80, 195, 115);  // Emerald Kokiri dots
  uint16_t crackColor = color565(40, 35, 30);    // Charcoal crack
  uint16_t sparkGold  = color565(255, 225, 90);   // Golden spark

  if (phase == 0) {
    int wobble = (int)(sin((double)nowMs * 0.010) * 2.5);
    int ex = cx + wobble;
    if (ex == lastEggEx) return; // Zero redraw if egg hasn't shifted pixel

    if (lastEggEx > 0) {
      // Delta-erase only left/right exposed slivers
      if (ex > lastEggEx) {
        carrier->display.fillRect(lastEggEx - 14, cy - 17, (ex - lastEggEx) + 2, 34, bgColor);
      } else {
        carrier->display.fillRect(ex + 12, cy - 17, (lastEggEx - ex) + 3, 34, bgColor);
      }
    }
    lastEggEx = ex;

    carrier->display.fillRoundRect(ex - 13, cy - 16, 26, 32, 11, eggShell);
    carrier->display.drawRoundRect(ex - 13, cy - 16, 26, 32, 11, eggShadow);
    carrier->display.fillCircle(ex - 4, cy - 6, 2, eggSpeckle);
    carrier->display.fillCircle(ex + 5, cy - 2, 3, eggSpeckle);
    carrier->display.fillCircle(ex - 2, cy + 6, 2, eggSpeckle);
    carrier->display.drawPixel(ex + 4, cy + 7, eggSpeckle);
  } else if (phase == 1) {
    int wobble = (int)(sin((double)nowMs * 0.022) * 3.0);
    int ex = cx + wobble;
    if (ex == lastEggEx) return;

    if (lastEggEx > 0) {
      if (ex > lastEggEx) {
        carrier->display.fillRect(lastEggEx - 14, cy - 17, (ex - lastEggEx) + 2, 34, bgColor);
      } else {
        carrier->display.fillRect(ex + 12, cy - 17, (lastEggEx - ex) + 3, 34, bgColor);
      }
    }
    lastEggEx = ex;

    carrier->display.fillRoundRect(ex - 13, cy - 16, 26, 32, 11, eggShell);
    carrier->display.drawRoundRect(ex - 13, cy - 16, 26, 32, 11, eggShadow);
    carrier->display.fillCircle(ex - 4, cy - 6, 2, eggSpeckle);
    carrier->display.fillCircle(ex + 5, cy - 2, 3, eggSpeckle);

    // Zigzag fissure across egg
    carrier->display.drawLine(ex - 11, cy - 1, ex - 4, cy - 5, crackColor);
    carrier->display.drawLine(ex - 4, cy - 5, ex + 3, cy + 2, crackColor);
    carrier->display.drawLine(ex + 3, cy + 2, ex + 10, cy - 3, crackColor);

    // Golden light leaking out of crack
    carrier->display.drawPixel(ex - 4, cy - 4, sparkGold);
    carrier->display.drawPixel(ex + 3, cy + 3, sparkGold);
  } else {
    // Phase 2: Top shell pops off, Baby Lumi emerges with pacifier!
    if (lastEggEx != cx) {
      lastEggEx = cx;
      carrier->display.fillRoundRect(cx - 13, cy + 4, 26, 12, 4, eggShell);
      carrier->display.drawRoundRect(cx - 13, cy + 4, 26, 12, 4, eggShadow);

      int topY = cy - 26;
      carrier->display.fillRoundRect(cx - 15, topY, 22, 11, 4, eggShell);
      carrier->display.drawRoundRect(cx - 15, topY, 22, 11, 4, eggShadow);

      carrier->display.fillRoundRect(cx - 14, cy - 16, 28, 22, 9, color565(155, 248, 220));

      uint16_t eyeDark = color565(18, 45, 35);
      carrier->display.drawLine(cx - 8, cy - 8, cx - 6, cy - 11, eyeDark);
      carrier->display.drawLine(cx - 6, cy - 11, cx - 4, cy - 8, eyeDark);
      carrier->display.drawLine(cx + 4, cy - 8, cx + 6, cy - 11, eyeDark);
      carrier->display.drawLine(cx + 6, cy - 11, cx + 8, cy - 8, eyeDark);

      carrier->display.fillCircle(cx - 9, cy - 5, 2, color565(255, 140, 175));
      carrier->display.fillCircle(cx + 9, cy - 5, 2, color565(255, 140, 175));

      carrier->display.fillCircle(cx, cy - 3, 3, color565(255, 220, 60));
      carrier->display.drawCircle(cx, cy - 3, 3, color565(200, 160, 30));

      carrier->display.drawFastVLine(cx, cy - 20, 4, color565(75, 205, 95));
      carrier->display.fillCircle(cx + 2, cy - 21, 2, color565(95, 235, 115));
    }

    // Radiating magical golden stars (delta-erased)
    int step = ((nowMs / 90) % 6);
    if (step != lastSparkStep) {
      if (lastSparkStep >= 0) {
        int oldRad = 22 + lastSparkStep * 2;
        carrier->display.drawPixel(cx - oldRad, cy - 5, bgColor);
        carrier->display.drawPixel(cx + oldRad, cy - 5, bgColor);
        carrier->display.drawPixel(cx - (oldRad * 7 / 10), cy - (oldRad * 7 / 10), bgColor);
        carrier->display.drawPixel(cx + (oldRad * 7 / 10), cy - (oldRad * 7 / 10), bgColor);
      }
      int rad = 22 + step * 2;
      carrier->display.drawPixel(cx - rad, cy - 5, sparkGold);
      carrier->display.drawPixel(cx + rad, cy - 5, sparkGold);
      carrier->display.drawPixel(cx - (rad * 7 / 10), cy - (rad * 7 / 10), sparkGold);
      carrier->display.drawPixel(cx + (rad * 7 / 10), cy - (rad * 7 / 10), sparkGold);
      lastSparkStep = step;
    }
  }

  stageInitialized = false;
}

void UiPetRenderer::triggerBath(unsigned long nowMs) {
  bathEndTimeMs = nowMs + 1500;
  lastBubbleStep = -1;
}

bool UiPetRenderer::isBathing(unsigned long nowMs) const {
  return nowMs < bathEndTimeMs;
}

void UiPetRenderer::drawPoop(int x, int y, int phase) {
  if (carrier == nullptr) return;
  uint16_t nativeBg  = color565(8, 12, 26);
  uint16_t poopDark  = color565(110, 60, 25);
  uint16_t poopMid   = color565(145, 85, 35);
  uint16_t poopLight = color565(185, 115, 50);
  uint16_t flyColor  = color565(160, 210, 230);

  // 1. Swirl poop body
  carrier->display.fillRoundRect(x - 6, y + 2, 12, 5, 2, poopMid);
  carrier->display.drawRoundRect(x - 6, y + 2, 12, 5, 2, poopDark);
  carrier->display.fillCircle(x, y, 4, poopMid);
  carrier->display.drawCircle(x, y, 4, poopDark);
  carrier->display.drawPixel(x, y - 4, poopLight); // Tip swirl
  carrier->display.drawPixel(x - 2, y - 1, poopLight); // Specular highlight

  // 2. Clear tiny fly zone above poop before drawing alternating flies
  carrier->display.fillRect(x - 6, y - 9, 13, 4, nativeBg);
  int f1Y = (phase % 2 == 0) ? (y - 7) : (y - 8);
  int f2Y = (phase % 2 == 0) ? (y - 8) : (y - 6);
  carrier->display.drawPixel(x - 5, f1Y, flyColor);
  carrier->display.drawPixel(x + 5, f2Y, flyColor);
}

void UiPetRenderer::erasePoop(int x, int y, uint16_t bgColor) {
  if (carrier == nullptr) return;
  // Clear bounding box of poop + flies
  carrier->display.fillRect(x - 8, y - 9, 17, 18, bgColor);
  // Redraw underlying grass blades/flower
  uint16_t grassGreen  = color565(80, 190, 100);
  uint16_t petalBlue   = color565(150, 225, 255);
  uint16_t flowerCore  = color565(255, 230, 80);
  carrier->display.drawLine(171, 153, 169, 148, grassGreen);
  carrier->display.drawLine(172, 153, 172, 147, grassGreen);
  carrier->display.drawLine(173, 153, 176, 149, grassGreen);
  carrier->display.fillCircle(172, 146, 3, petalBlue);
  carrier->display.drawPixel(172, 146, flowerCore);
}

void UiPetRenderer::drawSoapBubbles(unsigned long nowMs) {
  if (carrier == nullptr) return;
  int step = (int)((nowMs / 95) % 6);
  if (step == lastBubbleStep) return;

  uint16_t nativeBg    = color565(8, 12, 26);
  uint16_t bubbleCyan  = color565(120, 230, 255);
  uint16_t bubbleWhite = 0xFFFF;

  // Frame Lumi on left & right sides in the sky so bubbles never corrupt Lumi's face
  const int bubblePos[6][2] = {
    {82, 130}, {88, 114}, {78, 102}, {154, 128}, {162, 114}, {150, 100}
  };

  if (lastBubbleStep >= 0) {
    for (int i = 0; i < 6; i++) {
      int oldBx = bubblePos[i][0] + ((lastBubbleStep + i) % 3) - 1;
      int oldBy = bubblePos[i][1] - (lastBubbleStep * 2);
      int r = 3 + (i % 2);
      carrier->display.drawCircle(oldBx, oldBy, r, nativeBg);
      carrier->display.drawPixel(oldBx - 1, oldBy - 1, nativeBg);
    }
  }

  for (int i = 0; i < 6; i++) {
    int bx = bubblePos[i][0] + ((step + i) % 3) - 1;
    int by = bubblePos[i][1] - (step * 2);
    int r = 3 + (i % 2);
    carrier->display.drawCircle(bx, by, r, bubbleCyan);
    carrier->display.drawPixel(bx - 1, by - 1, bubbleWhite);
  }
  lastBubbleStep = step;
}

void UiPetRenderer::triggerScreenShake(unsigned long nowMs, uint16_t durationMs) {
  screenShakeEndTimeMs = nowMs + durationMs;
}

void UiPetRenderer::triggerNova(unsigned long nowMs, uint16_t durationMs) {
  novaStartTimeMs = nowMs;
  novaDurationMs = durationMs;
  lastNovaRadius = 0;
}

void UiPetRenderer::triggerMorph(unsigned long nowMs, uint16_t durationMs) {
  morphStartTimeMs = nowMs;
  morphDurationMs = durationMs;
  lastMorphStep = -1;
}

bool UiPetRenderer::isMorphing(unsigned long nowMs) const {
  return (morphDurationMs > 0 && (nowMs - morphStartTimeMs < morphDurationMs));
}

void UiPetRenderer::playIrisTransition(uint16_t accentColor) {
  if (carrier == nullptr) return;
  uint16_t ringDim = color565(18, 32, 62);
  // Fast concentric circular portal sweep across the 240x240 display
  for (int r = 114; r >= 18; r -= 16) {
    carrier->display.drawCircle(120, 120, r, accentColor);
    carrier->display.drawCircle(120, 120, r - 4, ringDim);
  }
  stageInitialized = false;
  lastShowcaseAct = 0;
}

void UiPetRenderer::drawShowcaseHeader(uint8_t actNumber, const char* actTitle, bool force) {
  if (carrier == nullptr || actNumber == 0) return;
  if (!force && actNumber == lastShowcaseAct && actTitle == lastShowcaseTitle) {
    return;
  }
  lastShowcaseAct   = actNumber;
  lastShowcaseTitle = actTitle;

  uint16_t nativeBg   = color565(8, 12, 26);
  uint16_t cardBg     = color565(14, 24, 48);
  uint16_t cardBorder = color565(55, 165, 225);
  uint16_t goldColor  = color565(255, 215, 70);
  uint16_t barEmpty   = color565(28, 44, 78);
  uint16_t barFill    = color565(80, 240, 190);

  // Clear top menu bar region and render sleek Cinema Chapter Header (Y = 14..62)
  carrier->display.fillRect(34, 14, 172, 50, nativeBg);
  carrier->display.fillRoundRect(38, 16, 164, 44, 7, cardBg);
  carrier->display.drawRoundRect(38, 16, 164, 44, 7, cardBorder);

  // Line 1: Act indicator ("* SHOWCASE AKT X AF 11 *")
  char actBuf[28];
  snprintf(actBuf, sizeof(actBuf), "* SHOWCASE AKT %u/11 *", (unsigned int)actNumber);
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(goldColor);
  int actX = 120 - ((int)strlen(actBuf) * 3);
  carrier->display.setCursor(actX, 21);
  carrier->display.print(actBuf);

  // Line 2: Danish Chapter Title
  if (actTitle != nullptr && actTitle[0] != '\0') {
    carrier->display.setTextColor(0xFFFF);
    int titleX = 120 - ((int)strlen(actTitle) * 3);
    if (titleX < 44) titleX = 44;
    carrier->display.setCursor(titleX, 33);
    carrier->display.print(actTitle);
  }

  // Line 3: 11-segment Cinema Timeline Progress Bar (X = 50..190, Y = 48..52)
  const int barX = 50;
  const int barY = 48;
  const int segW = 11;
  const int segGap = 2;
  for (uint8_t i = 1; i <= 11; i++) {
    int sx = barX + (i - 1) * (segW + segGap);
    uint16_t col = (i < actNumber) ? barFill : (i == actNumber ? goldColor : barEmpty);
    carrier->display.fillRoundRect(sx, barY, segW, 5, 1, col);
  }
}

void UiPetRenderer::setCelestialAura(bool active) {
  celestialAuraActive = active;
}

bool UiPetRenderer::isCelestialAura() const {
  return celestialAuraActive;
}

void UiPetRenderer::drawTitleSplash(unsigned long nowMs) {
  if (carrier == nullptr) return;
  (void)nowMs;
  uint16_t nativeBg = color565(8, 12, 26);
  carrier->display.fillScreen(nativeBg);

  // Double golden-cyan circular border ring
  carrier->display.drawCircle(120, 120, 116, color565(40, 180, 220));
  carrier->display.drawCircle(120, 120, 114, color565(255, 215, 60));

  // Outer sparkles
  drawSnowflake(45, 55, color565(255, 230, 100));
  drawSnowflake(195, 55, color565(80, 240, 255));
  drawSnowflake(45, 185, color565(80, 240, 255));
  drawSnowflake(195, 185, color565(255, 230, 100));

  // Subtitle top
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(color565(140, 200, 255));
  carrier->display.setCursor(68, 42);
  carrier->display.print("* TAMAPAL 2026 *");

  // Main Title: TAMAPAL in big letters
  carrier->display.setTextSize(3);
  carrier->display.setTextColor(color565(20, 45, 80));
  carrier->display.setCursor(55, 78);
  carrier->display.print("TAMAPAL");

  carrier->display.setTextColor(color565(255, 225, 90));
  carrier->display.setCursor(53, 76);
  carrier->display.print("TAMAPAL");

  // Golden separator line
  carrier->display.drawFastHLine(48, 110, 144, color565(255, 215, 60));
  carrier->display.drawPixel(120, 110, 0xFFFF);

  // Mercantec subtitle
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(0xFFFF);
  carrier->display.setCursor(76, 122);
  carrier->display.print("MERCANTEC H3");

  carrier->display.setTextColor(color565(100, 235, 180));
  carrier->display.setCursor(72, 138);
  carrier->display.print("MIDDLE PROJEKT");

  // Team authors
  carrier->display.setTextColor(color565(255, 180, 80));
  carrier->display.setCursor(58, 158);
  carrier->display.print("OLEH * OLIVER * ANDRII");

  // Cute mini Lumi pixel mascot at bottom (X=120, Y=194)
  carrier->display.fillRoundRect(113, 188, 15, 12, 4, color565(155, 248, 220));
  carrier->display.drawFastVLine(120, 184, 4, color565(255, 215, 60));
  carrier->display.drawPixel(121, 183, color565(100, 255, 120));
  carrier->display.drawPixel(116, 191, color565(15, 30, 40));
  carrier->display.drawPixel(124, 191, color565(15, 30, 40));
  carrier->display.fillCircle(129, 189, 2, color565(155, 248, 220));

  // Bottom ready message
  carrier->display.setTextColor(color565(120, 140, 170));
  carrier->display.setCursor(74, 212);
  carrier->display.print("KLAR TIL KAMP!");
}
