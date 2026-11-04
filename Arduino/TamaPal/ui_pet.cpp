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
    stageInitialized(false),
    lastDrawnCx(LUMI_BASE_CX),
    lastDrawnCy(LUMI_BASE_CY),
    lastNaviX(-1),
    lastNaviY(-1),
    lastZzzPhase(-1),
    lastBlinking(false),
    lastExpression(EXPR_CONTENT),
    lastMoodText(nullptr),
    lastAnimTickMs(0) {}

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

void UiPetRenderer::drawForestStage(bool isNightMode) {
  if (carrier == nullptr) return;

  uint16_t skyBg = isNightMode ? color565(6, 11, 22) : color565(10, 18, 34);
  carrier->display.fillRect(STAGE_X, STAGE_Y, STAGE_W, STAGE_H, skyBg);

  // Subtle distant Kokiri forest fireflies / stars in upper sky corners
  uint16_t fireflyGold = color565(220, 255, 130);
  uint16_t starDim     = color565(100, 160, 210);
  carrier->display.drawPixel(48, 78, starDim);
  carrier->display.drawPixel(56, 118, fireflyGold);
  carrier->display.drawPixel(188, 80, starDim);
  carrier->display.drawPixel(142, 76, fireflyGold);

  // 2.5D Mossy Kokiri Forest Glade Platform (Y = 153..169)
  uint16_t mossShadow = color565(14, 48, 36);
  uint16_t mossBase   = color565(26, 86, 56);
  uint16_t mossTop    = color565(56, 148, 86);
  uint16_t mossEdge   = color565(110, 215, 125);

  carrier->display.fillRoundRect(52, 155, 136, 14, 7, mossShadow);
  carrier->display.fillRoundRect(54, 153, 132, 13, 6, mossBase);
  carrier->display.fillRoundRect(58, 153, 124, 6, 3, mossTop);
  carrier->display.drawFastHLine(66, 153, 108, mossEdge);

  // Soft ground shadow directly beneath Lumi (anchors 2.5D depth)
  uint16_t dropShadow = color565(12, 42, 30);
  carrier->display.fillRoundRect(96, 154, 48, 5, 2, dropShadow);

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
  uint16_t pillBg     = color565(16, 28, 48);
  uint16_t pillBorder = color565(55, 130, 175);
  uint16_t textColor  = color565(225, 245, 255);

  if (expr == EXPR_FREEZING || expr == EXPR_HUNGRY || expr == EXPR_EXHAUSTED || expr == EXPR_SAD) {
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

void UiPetRenderer::eraseOldCharacterRim(int oldCx, int oldCy, int newCx, int newCy, uint16_t bgColor) {
  if (carrier == nullptr) return;
  int dx = newCx - oldCx;
  int dy = newCy - oldCy;
  if (dx == 0 && dy == 0) return;

  // Character bounding box relative to (cx, cy): X in [-29..+29], Y in [-33..+22]
  if (dy > 0) {
    // Moved down: clear top strip exposed above sprout and ears
    carrier->display.fillRect(oldCx - 29, oldCy - 33, 59, dy + 3, bgColor);
  } else if (dy < 0) {
    // Moved up: clear bottom strip above the mossy platform (Y < 153)
    int clearH = (-dy) + 2;
    int clearY = newCy + 20;
    if (clearY + clearH > 153) {
      clearH = 153 - clearY;
    }
    if (clearH > 0) {
      carrier->display.fillRect(oldCx - 29, clearY, 59, clearH, bgColor);
    }
  }

  if (dx > 0) {
    // Moved right: clear left margin strip
    carrier->display.fillRect(oldCx - 29, oldCy - 33, dx + 4, 54, bgColor);
  } else if (dx < 0) {
    // Moved left: clear right margin strip
    carrier->display.fillRect(newCx + 25, oldCy - 33, (-dx) + 4, 54, bgColor);
  }
}

void UiPetRenderer::drawFaceFeatures(int cx, int cy, LumiExpression expr, bool isBlinking, uint16_t bodyColor) {
  if (carrier == nullptr) return;

  // 1. Wipe interior face canvas with bodyColor so blinking/expression switches never leave ghost pixels
  carrier->display.fillRoundRect(cx - 19, cy - 11, 38, 18, 4, bodyColor);

  uint16_t eyeDark     = color565(16, 22, 38);
  uint16_t irisEmerald = color565(45, 195, 165);
  uint16_t irisBlue    = color565(75, 175, 255);
  uint16_t pureWhite   = 0xFFFF;
  uint16_t cheekPink   = color565(255, 130, 155);
  uint16_t cheekCoral  = color565(255, 95, 120);
  uint16_t cheekIce    = color565(100, 185, 255);
  uint16_t mouthDark   = color565(28, 45, 58);
  uint16_t tonguePink  = color565(255, 115, 140);

  // Rosy kawaii cheeks
  uint16_t activeCheek = cheekPink;
  if (expr == EXPR_SUPER_HAPPY || expr == EXPR_EATING || expr == EXPR_PLAYING || expr == EXPR_SCOLDED) {
    activeCheek = cheekCoral;
  } else if (expr == EXPR_FREEZING) {
    activeCheek = cheekIce;
  }
  carrier->display.fillRoundRect(cx - 18, cy + 1, 6, 3, 1, activeCheek);
  carrier->display.fillRoundRect(cx + 12, cy + 1, 6, 3, 1, activeCheek);

  int leftEyeX  = cx - 10;
  int rightEyeX = cx + 10;
  int eyeY      = cy - 4;

  // 2. Render Eyes according to expression and blink state
  if (isBlinking || expr == EXPR_SLEEPING || expr == EXPR_DEAD) {
    // Peaceful closed sleeping/blinking eye arcs (-  -)
    carrier->display.drawFastHLine(leftEyeX - 3, eyeY + 1, 7, eyeDark);
    carrier->display.drawFastHLine(leftEyeX - 2, eyeY + 2, 5, eyeDark);
    carrier->display.drawFastHLine(rightEyeX - 3, eyeY + 1, 7, eyeDark);
    carrier->display.drawFastHLine(rightEyeX - 2, eyeY + 2, 5, eyeDark);
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
    // Large Glossy Tamagotchi Paradise / Zelda Anime Eyes (9x10 rounded ovals)
    carrier->display.fillRoundRect(leftEyeX - 4, eyeY - 5, 9, 10, 4, eyeDark);
    carrier->display.fillRoundRect(rightEyeX - 4, eyeY - 5, 9, 10, 4, eyeDark);

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

    // Half-closed droopy eyelids when EXHAUSTED
    if (expr == EXPR_EXHAUSTED) {
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
    }
  }

  // 3. Render Mouth according to expression
  int mouthY = cy + 2;
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

    case EXPR_CONTENT:
    case EXPR_DEAD:
      // Classic Tamagotchi 'w' cat smile
      carrier->display.drawLine(cx - 4, mouthY, cx - 2, mouthY + 2, mouthDark);
      carrier->display.drawLine(cx - 2, mouthY + 2, cx, mouthY, mouthDark);
      carrier->display.drawLine(cx, mouthY, cx + 2, mouthY + 2, mouthDark);
      carrier->display.drawLine(cx + 2, mouthY + 2, cx + 4, mouthY, mouthDark);
      break;

    case EXPR_SLEEPING:
    case EXPR_HUNGRY:
    case EXPR_EXHAUSTED:
      // Cute small 'o' mouth (breathing / pleading / yawning)
      carrier->display.fillCircle(cx, mouthY + 1, 2, mouthDark);
      carrier->display.drawPixel(cx, mouthY + 1, tonguePink);
      break;

    case EXPR_FREEZING:
      // Chattering wavy mouth (~)
      carrier->display.drawPixel(cx - 4, mouthY + 1, mouthDark);
      carrier->display.drawPixel(cx - 3, mouthY, mouthDark);
      carrier->display.drawPixel(cx - 2, mouthY + 1, mouthDark);
      carrier->display.drawPixel(cx - 1, mouthY + 2, mouthDark);
      carrier->display.drawPixel(cx,     mouthY + 1, mouthDark);
      carrier->display.drawPixel(cx + 1, mouthY, mouthDark);
      carrier->display.drawPixel(cx + 2, mouthY + 1, mouthDark);
      carrier->display.drawPixel(cx + 3, mouthY + 2, mouthDark);
      break;

    case EXPR_SAD:
    case EXPR_SCOLDED:
      // Cute pouty frown arc
      carrier->display.drawFastHLine(cx - 2, mouthY + 1, 5, mouthDark);
      carrier->display.drawPixel(cx - 3, mouthY + 2, mouthDark);
      carrier->display.drawPixel(cx + 3, mouthY + 2, mouthDark);
      break;
  }
}

void UiPetRenderer::drawLumiCharacter(int cx, int cy, LumiExpression expr, bool isBlinking, uint16_t bgColor) {
  if (carrier == nullptr) return;
  (void)bgColor;

  // Dynamic 2.5D Color Palette based on Lumi's physical/emotional state
  uint16_t bodyMain    = color565(155, 248, 220); // Glowing Kokiri Mint-Cream
  uint16_t bodyOutline = color565(35, 125, 110);  // Deep Jade Outline
  uint16_t bodyShade   = color565(95, 200, 175);  // 2.5D Lower Ambient Shading
  uint16_t bellyPatch  = color565(235, 255, 240); // Soft Cream Mochi Belly
  uint16_t earInner    = color565(255, 165, 185); // Warm Pink Ear Tuft
  uint16_t footColor   = color565(65, 175, 150);  // Jade Paws

  if (expr == EXPR_FREEZING) {
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
  } else if (expr == EXPR_DEAD) {
    bodyMain    = color565(210, 225, 235); // Ethereal Spirit White-Blue
    bodyOutline = color565(110, 140, 165);
    bodyShade   = color565(170, 195, 215);
    bellyPatch  = 0xFFFF;
    footColor   = color565(140, 170, 195);
  }

  // 1. Kokiri Emerald Sprout on Head (Zelda Ocarina of Time forest spirit motif)
  uint16_t stemGreen     = color565(45, 165, 75);
  uint16_t leafEmerald   = color565(65, 225, 105);
  uint16_t leafHighlight = color565(175, 255, 165);

  if (expr == EXPR_DEAD) {
    // Golden Spirit Halo above head
    carrier->display.drawRoundRect(cx - 10, cy - 29, 20, 5, 2, color565(255, 235, 90));
  } else {
    // Stem
    carrier->display.fillRect(cx - 1, cy - 26, 3, 7, stemGreen);
    // Left & Right Kokiri Leaves
    carrier->display.fillTriangle(cx - 1, cy - 25, cx - 10, cy - 31, cx - 7, cy - 23, leafEmerald);
    carrier->display.fillTriangle(cx + 1, cy - 25, cx + 10, cy - 31, cx + 7, cy - 23, leafEmerald);
    // Specular leaf vein highlights
    carrier->display.drawLine(cx - 2, cy - 26, cx - 8, cy - 29, leafHighlight);
    carrier->display.drawLine(cx + 2, cy - 26, cx + 8, cy - 29, leafHighlight);
  }

  // 2. Expressive Forest Spirit Ears (Perky when happy, slightly drooped when sad/tired)
  int earTipOffsetY = (expr == EXPR_SAD || expr == EXPR_EXHAUSTED || expr == EXPR_SCOLDED) ? -19 : -25;
  // Left Ear
  carrier->display.fillTriangle(cx - 21, cy - 11, cx - 26, cy + earTipOffsetY, cx - 10, cy - 18, bodyOutline);
  carrier->display.fillTriangle(cx - 20, cy - 12, cx - 24, cy + earTipOffsetY + 2, cx - 11, cy - 17, bodyMain);
  carrier->display.fillTriangle(cx - 19, cy - 13, cx - 22, cy + earTipOffsetY + 4, cx - 13, cy - 16, earInner);
  // Right Ear
  carrier->display.fillTriangle(cx + 21, cy - 11, cx + 26, cy + earTipOffsetY, cx + 10, cy - 18, bodyOutline);
  carrier->display.fillTriangle(cx + 20, cy - 12, cx + 24, cy + earTipOffsetY + 2, cx + 11, cy - 17, bodyMain);
  carrier->display.fillTriangle(cx + 19, cy - 13, cx + 22, cy + earTipOffsetY + 4, cx + 13, cy - 16, earInner);

  // 3. Cute Stubby Feet resting on the mossy forest glade
  carrier->display.fillRoundRect(cx - 16, cy + 16, 10, 6, 3, footColor);
  carrier->display.fillRoundRect(cx + 6,  cy + 16, 10, 6, 3, footColor);

  // 4. 2.5D Chubby Mochi Body (48x38 rounded silhouette)
  carrier->display.fillRoundRect(cx - 24, cy - 20, 48, 39, 16, bodyOutline);
  carrier->display.fillRoundRect(cx - 22, cy - 18, 44, 35, 14, bodyShade);
  carrier->display.fillRoundRect(cx - 22, cy - 18, 44, 31, 13, bodyMain);

  // Top-left 2.5D Specular Gloss Highlight on forehead
  carrier->display.fillRoundRect(cx - 13, cy - 16, 10, 3, 1, 0xFFFF);
  carrier->display.drawPixel(cx - 15, cy - 14, 0xFFFF);

  // Soft Cream Belly Patch (Tamagotchi Paradise aesthetic)
  carrier->display.fillRoundRect(cx - 13, cy + 7, 26, 9, 4, bellyPatch);

  // 5. Stubby Arms / Paws (Raised in celebration when super happy/playing/eating!)
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
  } else {
    // Relaxed side paws
    carrier->display.fillRoundRect(cx - 26, cy - 1, 5, 8, 2, bodyOutline);
    carrier->display.fillRoundRect(cx - 25, cy,     3, 6, 1, bodyMain);
    carrier->display.fillRoundRect(cx + 21, cy - 1, 5, 8, 2, bodyOutline);
    carrier->display.fillRoundRect(cx + 22, cy,     3, 6, 1, bodyMain);
  }

  // 6. Facial Features (Eyes, Cheeks, Mouth)
  drawFaceFeatures(cx, cy, expr, isBlinking, bodyMain);
}

void UiPetRenderer::updateNaviFairy(int targetX, int targetY, bool isSleeping, uint16_t bgColor, bool force) {
  if (carrier == nullptr) return;
  if (!force && targetX == lastNaviX && targetY == lastNaviY) {
    return;
  }

  // Erase previous Navi 20x18 bounding box if she moved
  if (lastNaviX >= 0 && lastNaviY >= 0 && (lastNaviX != targetX || lastNaviY != targetY)) {
    carrier->display.fillRect(lastNaviX - 10, lastNaviY - 9, 21, 18, bgColor);
  }

  lastNaviX = targetX;
  lastNaviY = targetY;

  // Zelda Ocarina of Time 'Navi' Fairy Companion:
  // Translucent lavender-white wings + glowing cyan aura + bright white fairy core
  uint16_t wingColor = isSleeping ? color565(110, 145, 200) : color565(200, 225, 255);
  uint16_t auraCyan  = isSleeping ? color565(40, 120, 190)  : color565(65, 220, 255);
  uint16_t coreWhite = 0xFFFF;

  // Upper wings
  carrier->display.fillTriangle(targetX - 2, targetY - 1, targetX - 8, targetY - 7, targetX - 4, targetY - 1, wingColor);
  carrier->display.fillTriangle(targetX + 2, targetY - 1, targetX + 8, targetY - 7, targetX + 4, targetY - 1, wingColor);
  // Lower small wings
  carrier->display.drawPixel(targetX - 5, targetY + 2, wingColor);
  carrier->display.drawPixel(targetX + 5, targetY + 2, wingColor);

  // Glowing cyan halo + bright white center
  carrier->display.fillCircle(targetX, targetY, 4, auraCyan);
  carrier->display.fillCircle(targetX, targetY, 2, coreWhite);
}

void UiPetRenderer::updateSideOverlay(int baseCx, int baseCy, LumiExpression expr, int phase, uint16_t bgColor, bool force) {
  if (carrier == nullptr) return;
  if (!force && phase == lastZzzPhase && expr == lastExpression) {
    return;
  }
  lastZzzPhase = phase;

  // Dedicated right-side overlay zone: X = 152..188, Y = 84..128 (never overlaps Lumi or right flower)
  int boxX = baseCx + 32; // 152
  int boxY = baseCy - 47; // 84
  carrier->display.fillRect(boxX, boxY, 36, 44, bgColor);

  if (expr == EXPR_SLEEPING) {
    // Floating animated 'z Z' sleep letters
    uint16_t zzzLight = color565(150, 210, 255);
    uint16_t zzzDim   = color565(85, 140, 210);
    carrier->display.setTextSize(1);

    if (phase >= 1) {
      carrier->display.setTextColor(zzzDim);
      carrier->display.setCursor(boxX + 3, boxY + 28);
      carrier->display.print("z");
    }
    if (phase >= 2) {
      carrier->display.setTextColor(zzzLight);
      carrier->display.setCursor(boxX + 11, boxY + 16);
      carrier->display.print("Z");
    }
    if (phase >= 3) {
      carrier->display.setTextColor(0xFFFF);
      carrier->display.setCursor(boxX + 20, boxY + 4);
      carrier->display.print("Z");
    }
  } else if (expr == EXPR_HUNGRY) {
    // Cute Tamagotchi thought-bubble with a red Kokiri berry inside
    uint16_t bubbleWhite = color565(225, 240, 255);
    carrier->display.fillCircle(boxX + 4, boxY + 28, 2, bubbleWhite);
    carrier->display.fillRoundRect(boxX + 7, boxY + 8, 22, 18, 6, bubbleWhite);
    carrier->display.fillCircle(boxX + 18, boxY + 18, 4, color565(255, 55, 75));
    carrier->display.fillRect(boxX + 17, boxY + 12, 3, 2, color565(70, 200, 90));
  } else if (expr == EXPR_SUPER_HAPPY || expr == EXPR_PLAYING) {
    // Twinkling Nintendo star / heart sparkles
    uint16_t starGold = color565(255, 235, 80);
    int sy = boxY + 18 - (phase % 2) * 4;
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

  // Throttle smooth procedural animation to ~15 FPS (65 ms per frame)
  if (!forceRedraw && stageInitialized && (nowMs - lastAnimTickMs < 65)) {
    return;
  }
  lastAnimTickMs = nowMs;

  LumiExpression expr = determineExpression(pet);
  bool isNight = pet.isSleeping();
  uint16_t skyBg = isNight ? color565(6, 11, 22) : color565(10, 18, 34);

  // 1. Compute procedural squash/stretch breathing, happy hop, and cold shiver offsets
  int dx = 0;
  int dy = 0;

  if (expr == EXPR_FREEZING) {
    // Rapid left-right shivering (+/- 2 px) as described in the Game Design Doc
    dx = ((nowMs / 65) % 2 == 0) ? -2 : 2;
  } else if (expr == EXPR_SLEEPING) {
    // Slow, deep sleeping respiration
    const int sleepBreath[4] = {0, 0, -1, -1};
    dy = sleepBreath[(nowMs / 500) % 4];
  } else if (expr == EXPR_SUPER_HAPPY) {
    // Joyful Nintendo bounce rhythm
    const int happyHop[4] = {0, -2, -3, -1};
    dy = happyHop[(nowMs / 240) % 4];
  } else if (expr != EXPR_DEAD) {
    // Gentle organic idle breathing
    const int idleBreath[4] = {0, -1, -2, -1};
    dy = idleBreath[(nowMs / 340) % 4];
  }

  int targetCx = LUMI_BASE_CX + dx;
  int targetCy = LUMI_BASE_CY + dy;

  // Natural eye blink: 140 ms blink every 3.2 seconds
  bool isBlinking = (!isNight) && ((nowMs % 3200) < 140);

  // Navi fairy gentle figure-8 orbit on upper-left (X = 74..78, Y = 90..96)
  const int naviDx[4] = {0, 2, 0, -2};
  const int naviDy[4] = {0, -2, -4, -2};
  int naviStep = (nowMs / 280) % 4;
  int targetNaviX = 75 + naviDx[naviStep];
  int targetNaviY = (isNight ? 100 : 94) + naviDy[naviStep];

  // Side overlay phase (0..3)
  int overlayPhase = (nowMs / 450) % 4;

  // 2. Full stage initialization when entering SCREEN_HOME or switching Day/Night mode
  bool nightChanged = (expr == EXPR_SLEEPING) != (lastExpression == EXPR_SLEEPING);
  if (forceRedraw || !stageInitialized || nightChanged) {
    drawForestStage(isNight);
    drawLumiCharacter(targetCx, targetCy, expr, isBlinking, skyBg);
    updateNaviFairy(targetNaviX, targetNaviY, isNight, skyBg, true);
    updateSideOverlay(LUMI_BASE_CX, LUMI_BASE_CY, expr, overlayPhase, skyBg, true);

    const char* mood = pet.getMoodStatusDanish();
    drawMoodBanner(mood, expr);

    lastDrawnCx     = targetCx;
    lastDrawnCy     = targetCy;
    lastBlinking    = isBlinking;
    lastExpression  = expr;
    lastMoodText    = mood;
    stageInitialized = true;
    return;
  }

  // 3. Delta-update Lumi (zero-flicker partial redraw)
  if (targetCx != lastDrawnCx || targetCy != lastDrawnCy || expr != lastExpression) {
    eraseOldCharacterRim(lastDrawnCx, lastDrawnCy, targetCx, targetCy, skyBg);
    drawLumiCharacter(targetCx, targetCy, expr, isBlinking, skyBg);
    lastDrawnCx    = targetCx;
    lastDrawnCy    = targetCy;
    lastBlinking   = isBlinking;
    lastExpression = expr;
  } else if (isBlinking != lastBlinking) {
    // Only the eyelids changed: redraw face interior directly without touching body!
    uint16_t bodyMain = color565(155, 248, 220);
    if (expr == EXPR_FREEZING) bodyMain = color565(150, 215, 255);
    else if (expr == EXPR_SLEEPING) bodyMain = color565(130, 195, 225);
    else if (expr == EXPR_SCOLDED) bodyMain = color565(180, 240, 215);
    else if (expr == EXPR_DEAD) bodyMain = color565(210, 225, 235);

    drawFaceFeatures(targetCx, targetCy, expr, isBlinking, bodyMain);
    lastBlinking = isBlinking;
  }

  // 4. Delta-update Navi and side overlay
  updateNaviFairy(targetNaviX, targetNaviY, isNight, skyBg, false);
  updateSideOverlay(LUMI_BASE_CX, LUMI_BASE_CY, expr, overlayPhase, skyBg, false);

  // 5. Delta-update Danish mood banner if status text changed
  const char* currentMood = pet.getMoodStatusDanish();
  if (currentMood != lastMoodText) {
    drawMoodBanner(currentMood, expr);
    lastMoodText = currentMood;
  }
}

void UiPetRenderer::drawActionScene(const PetLogic& pet,
                                    LumiExpression actionExpr,
                                    const char* title,
                                    const char* subtitle,
                                    unsigned long nowMs) {
  if (carrier == nullptr) return;
  (void)nowMs;

  bool isNight = (actionExpr == EXPR_SLEEPING);
  uint16_t skyBg = isNight ? color565(6, 11, 22) : color565(10, 18, 34);

  // 1. Draw the Kokiri forest stage with Lumi performing the action
  drawForestStage(isNight);
  int actionCy = (actionExpr == EXPR_PLAYING || actionExpr == EXPR_EATING) ? (LUMI_BASE_CY - 3) : LUMI_BASE_CY;
  drawLumiCharacter(LUMI_BASE_CX, actionCy, actionExpr, false, skyBg);
  updateNaviFairy(74, 96, isNight, skyBg, true);
  updateSideOverlay(LUMI_BASE_CX, LUMI_BASE_CY, actionExpr, 2, skyBg, true);

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

  // Mark stage so returning to SCREEN_HOME cleanly refreshes the top sky strip
  stageInitialized = false;
}

