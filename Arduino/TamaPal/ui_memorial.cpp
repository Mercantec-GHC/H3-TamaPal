// WHAT: Graphical renderer for the Sacred Forest Cemetery & Life Cycle Chronicles (SCREEN_MEMORIAL)
// WHY:  Renders atmospheric sacred grove with pine trees, meadow, graves and detailed ancestor life cycle
// USED BY: TamaPal.ino

#include "ui_memorial.h"
#include <string.h>
#include <stdio.h>

UiMemorialRenderer::UiMemorialRenderer()
  : carrier(nullptr) {
}

void UiMemorialRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
}

void UiMemorialRenderer::drawMysticalSkyAndMoon() {
  if (carrier == nullptr) return;

  uint16_t rimColor   = color565(26, 40, 72);
  uint16_t starCyan   = color565(180, 225, 255);
  uint16_t starGold   = color565(255, 230, 120);
  uint16_t starFaint  = color565(90, 120, 170);
  uint16_t haloDark   = color565(28, 42, 68);
  uint16_t moonGlow   = color565(255, 248, 185);
  uint16_t skyBg      = color565(6, 10, 24);

  // Outer circular rim guide
  carrier->display.drawCircle(120, 120, 118, rimColor);

  // Slender glowing crescent moon in upper left (x=54, y=36)
  carrier->display.drawCircle(54, 36, 10, haloDark);
  carrier->display.fillCircle(54, 36, 7, moonGlow);
  carrier->display.fillCircle(57, 34, 6, skyBg); // Shadow mask

  // Twinkling celestial stars scattered across the night sky
  carrier->display.drawPixel(32, 24, starCyan);
  carrier->display.drawPixel(88, 22, starGold);
  carrier->display.drawPixel(120, 15, starCyan);
  carrier->display.drawPixel(155, 20, starFaint);
  carrier->display.drawPixel(198, 28, starGold);
  carrier->display.drawPixel(175, 48, starCyan);
  carrier->display.drawPixel(105, 38, starFaint);
  carrier->display.drawPixel(42, 58, starGold);

  // Floating ethereal fireflies (ildfluer) drifting softly in the night air
  carrier->display.drawPixel(68, 88, starGold);
  carrier->display.drawPixel(172, 92, starCyan);
  carrier->display.drawPixel(122, 78, starGold);
  carrier->display.drawPixel(48, 105, starCyan);
  carrier->display.drawPixel(192, 112, starGold);
}

void UiMemorialRenderer::drawPineTree(int x, int baseY, int scale) {
  if (carrier == nullptr) return;

  uint16_t trunkColor = color565(38, 24, 16);
  uint16_t pineDark   = color565(12, 44, 22);
  uint16_t pineMid    = color565(15, 55, 28);
  uint16_t pineHigh   = color565(22, 76, 38);
  uint16_t pinePeak   = color565(36, 115, 58);

  // Tree trunk
  int trunkW = (scale == 2) ? 4 : 3;
  int trunkH = (scale == 2) ? 18 : 14;
  carrier->display.fillRect(x - (trunkW / 2), baseY - trunkH, trunkW, trunkH, trunkColor);

  // Tier 1 (bottom bough)
  int b1W = (scale == 2) ? 15 : 11;
  int b1H = (scale == 2) ? 14 : 10;
  carrier->display.fillTriangle(x - b1W, baseY - 10, x + b1W, baseY - 10, x, baseY - 10 - b1H, pineDark);
  carrier->display.drawLine(x - b1W + 2, baseY - 11, x, baseY - 9 - b1H, pineHigh);

  // Tier 2 (middle bough)
  int b2W = (scale == 2) ? 12 : 9;
  int b2H = (scale == 2) ? 13 : 9;
  int b2Y = baseY - 18;
  carrier->display.fillTriangle(x - b2W, b2Y, x + b2W, b2Y, x, b2Y - b2H, pineMid);
  carrier->display.drawLine(x - b2W + 2, b2Y - 1, x, b2Y - b2H + 1, pineHigh);

  // Tier 3 (top peak bough)
  int b3W = (scale == 2) ? 8 : 6;
  int b3H = (scale == 2) ? 12 : 8;
  int b3Y = baseY - 26;
  carrier->display.fillTriangle(x - b3W, b3Y, x + b3W, b3Y, x, b3Y - b3H, pineHigh);
  carrier->display.drawLine(x - b3W + 1, b3Y - 1, x, b3Y - b3H + 1, pinePeak);
}

void UiMemorialRenderer::drawGraveStone(int cx, int cy, const char* name, uint16_t gen, bool isSelected, uint8_t style) {
  if (carrier == nullptr) return;

  uint16_t shadowColor = color565(6, 18, 12);
  uint16_t plinthBody  = isSelected ? color565(65, 85, 110) : color565(36, 46, 62);
  uint16_t plinthBevel = isSelected ? color565(115, 155, 205) : color565(58, 72, 92);
  uint16_t stoneBody   = isSelected ? color565(85, 105, 135) : color565(48, 60, 78);
  uint16_t stoneBorder = isSelected ? color565(155, 195, 255) : color565(72, 90, 115);
  uint16_t leafSymbol  = color565(90, 220, 140);
  uint16_t starSymbol  = color565(255, 225, 90);
  uint16_t crossSymbol = color565(120, 215, 255);
  uint16_t flowerRed   = color565(255, 95, 115);
  uint16_t flowerLeaf  = color565(45, 140, 65);

  // 1. Soft 3D drop shadow beneath plinth
  carrier->display.fillRoundRect(cx - 16, cy + 9, 32, 7, 3, shadowColor);

  // 2. Stone base plinth / sarcophagus mound
  carrier->display.fillRoundRect(cx - 15, cy + 4, 30, 8, 2, plinthBody);
  carrier->display.drawRoundRect(cx - 15, cy + 4, 30, 8, 2, plinthBevel);

  // 3. Upright gravestone slab (rounded arch top)
  carrier->display.fillRoundRect(cx - 11, cy - 18, 22, 24, 4, stoneBody);
  carrier->display.drawRoundRect(cx - 11, cy - 18, 22, 24, 4, stoneBorder);

  // 4. Sacred carved emblem on headstone
  if (style % 3 == 0) {
    // Kokiri Leaf symbol
    carrier->display.drawPixel(cx, cy - 11, leafSymbol);
    carrier->display.fillRect(cx - 1, cy - 10, 3, 2, leafSymbol);
    carrier->display.drawPixel(cx, cy - 8, leafSymbol);
  } else if (style % 3 == 1) {
    // Golden Celestial Star
    carrier->display.drawPixel(cx, cy - 11, starSymbol);
    carrier->display.drawFastHLine(cx - 2, cy - 10, 5, starSymbol);
    carrier->display.drawPixel(cx, cy - 9, starSymbol);
  } else {
    // Ethereal Cross / Rune
    carrier->display.drawFastVLine(cx, cy - 12, 5, crossSymbol);
    carrier->display.drawFastHLine(cx - 2, cy - 10, 5, crossSymbol);
  }

  // 5. Tiny blooming forest flower at the foot of the grave
  carrier->display.drawPixel(cx + 9, cy + 3, flowerRed);
  carrier->display.drawPixel(cx + 10, cy + 3, starSymbol);
  carrier->display.drawPixel(cx + 8, cy + 4, flowerLeaf);

  // 6. Name and Generation label beneath base
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(isSelected ? color565(255, 230, 90) : color565(150, 175, 205));
  int nLen = (name != nullptr) ? (int)strlen(name) : 0;
  carrier->display.setCursor(cx - (nLen * 3), cy + 15);
  if (name != nullptr) {
    carrier->display.print(name);
  }

  char genBuf[12];
  snprintf(genBuf, sizeof(genBuf), "GEN %u", (unsigned int)gen);
  int gLen = (int)strlen(genBuf);
  carrier->display.setTextColor(isSelected ? color565(120, 220, 255) : color565(95, 125, 155));
  carrier->display.setCursor(cx - (gLen * 3), cy + 25);
  carrier->display.print(genBuf);

  // 7. Radiant golden aura if selected
  if (isSelected) {
    carrier->display.drawRoundRect(cx - 16, cy - 21, 32, 36, 6, color565(255, 215, 80));
    carrier->display.drawRoundRect(cx - 17, cy - 22, 34, 38, 7, color565(70, 160, 230));
  }
}

void UiMemorialRenderer::drawHoveringWisp(int cx, int cy) {
  if (carrier == nullptr) return;

  uint16_t haloGold   = color565(255, 230, 90);
  uint16_t wispCyan   = color565(140, 230, 255);
  uint16_t wispCore   = color565(255, 255, 255);
  uint16_t wingCyan   = color565(190, 240, 255);
  uint16_t arrowGold  = color565(255, 215, 80);

  int wy = cy - 27;

  // Golden circular halo
  carrier->display.drawFastHLine(cx - 4, wy - 9, 9, haloGold);

  // Glowing fairy orb (Lumi wisp)
  carrier->display.fillCircle(cx, wy, 4, wispCyan);
  carrier->display.fillCircle(cx, wy, 2, wispCore);

  // Little translucent fairy wings
  carrier->display.drawPixel(cx - 4, wy - 2, wingCyan);
  carrier->display.drawPixel(cx + 4, wy - 2, wingCyan);

  // Indicator arrow pointing downward to chosen grave
  carrier->display.fillTriangle(cx - 3, wy + 6, cx + 3, wy + 6, cx, wy + 10, arrowGold);
}

void UiMemorialRenderer::drawAncestralStele(int cx, int cy) {
  if (carrier == nullptr) return;

  uint16_t plinthDark = color565(35, 45, 65);
  uint16_t stoneBody  = color565(60, 75, 100);
  uint16_t stoneBevel = color565(95, 115, 145);
  uint16_t carvedLeaf = color565(110, 220, 160);
  uint16_t leafShadow = color565(30, 80, 50);

  carrier->display.fillRoundRect(cx - 16, cy + 12, 32, 6, 2, plinthDark);
  carrier->display.fillRoundRect(cx - 12, cy - 18, 24, 30, 5, stoneBody);
  carrier->display.drawRoundRect(cx - 12, cy - 18, 24, 30, 5, stoneBevel);

  carrier->display.drawPixel(cx, cy - 8, carvedLeaf);
  carrier->display.drawPixel(cx - 1, cy - 7, carvedLeaf);
  carrier->display.drawPixel(cx + 1, cy - 7, carvedLeaf);
  carrier->display.fillRect(cx - 2, cy - 6, 5, 3, carvedLeaf);
  carrier->display.drawPixel(cx - 1, cy - 3, carvedLeaf);
  carrier->display.drawPixel(cx + 1, cy - 3, carvedLeaf);
  carrier->display.drawPixel(cx, cy - 2, carvedLeaf);
  carrier->display.drawFastVLine(cx, cy - 1, 3, leafShadow);
}

void UiMemorialRenderer::drawHoveringSpirit(int cx, int cy) {
  if (carrier == nullptr) return;

  uint16_t haloGold    = color565(255, 230, 90);
  uint16_t auraGlow    = color565(35, 65, 90);
  uint16_t spiritBody  = color565(195, 240, 230);
  uint16_t spiritEyes  = color565(25, 45, 65);
  uint16_t spiritWisp  = color565(150, 215, 205);
  uint16_t wispDim     = color565(85, 150, 155);

  carrier->display.fillCircle(cx, cy - 2, 14, auraGlow);
  carrier->display.drawFastHLine(cx - 4, cy - 14, 9, haloGold);
  carrier->display.drawPixel(cx - 5, cy - 13, haloGold);
  carrier->display.drawPixel(cx + 5, cy - 13, haloGold);

  carrier->display.fillRoundRect(cx - 8, cy - 10, 16, 15, 6, spiritBody);
  carrier->display.drawFastHLine(cx - 5, cy - 4, 3, spiritEyes);
  carrier->display.drawFastHLine(cx + 3, cy - 4, 3, spiritEyes);

  carrier->display.fillRect(cx - 6, cy + 5, 3, 3, spiritWisp);
  carrier->display.fillRect(cx - 1, cy + 5, 3, 5, spiritWisp);
  carrier->display.fillRect(cx + 4, cy + 5, 3, 3, spiritWisp);
  carrier->display.drawPixel(cx, cy + 10, wispDim);
}

void UiMemorialRenderer::drawCemeteryGrove(uint16_t selectedIndex, uint16_t totalCount, StoragePet& storage, uint16_t currentGen) {
  if (carrier == nullptr) return;
  (void)currentGen;

  uint16_t bgColor    = color565(6, 10, 24);
  uint16_t titleGold  = color565(255, 215, 80);
  uint16_t subCyan    = color565(120, 210, 255);
  uint16_t meadowDark = color565(14, 44, 22);
  uint16_t grassTuft  = color565(35, 110, 50);
  uint16_t navColor   = color565(120, 170, 220);

  carrier->display.fillScreen(bgColor);

  // 1. Mystical night sky and crescent moon
  drawMysticalSkyAndMoon();

  // 2. Dark pine forest trees framing the sacred clearing (lesok)
  drawPineTree(16, 138, 2);
  drawPineTree(32, 122, 1);
  drawPineTree(224, 138, 2);
  drawPineTree(208, 122, 1);

  // 3. Lush grassy meadow clearing (pole)
  carrier->display.fillRoundRect(10, 114, 220, 95, 18, meadowDark);

  // Grass tufts and ethereal ground fog
  carrier->display.drawPixel(50, 120, grassTuft);
  carrier->display.drawPixel(51, 119, grassTuft);
  carrier->display.drawPixel(125, 122, grassTuft);
  carrier->display.drawPixel(190, 124, grassTuft);
  carrier->display.drawFastHLine(30, 118, 180, color565(20, 48, 30));

  // 4. Determine 3 perspective graves on the meadow field
  if (totalCount == 0) totalCount = 1;
  if (selectedIndex >= totalCount) selectedIndex = totalCount - 1;

  uint16_t windowStart = 0;
  if (totalCount <= 3) {
    windowStart = 0;
  } else {
    windowStart = (selectedIndex >= 2) ? (selectedIndex - 1) : 0;
    if (windowStart + 3 > totalCount) {
      windowStart = totalCount - 3;
    }
  }

  // Coordinates of the 3 grave plots in perspective
  const int graveX[3] = { 66, 120, 174 };
  const int graveY[3] = { 148, 160, 148 };

  GraveRecord activeRecord;
  bool activeRecordLoaded = false;

  for (uint8_t slot = 0; slot < 3; slot++) {
    uint16_t recordIndex = windowStart + slot;
    if (recordIndex < totalCount) {
      GraveRecord rec;
      bool ok = storage.loadCemeteryRecord(recordIndex, rec);
      bool isSelected = (recordIndex == selectedIndex);

      if (isSelected && ok) {
        activeRecord = rec;
        activeRecordLoaded = true;
      }

      drawGraveStone(graveX[slot], graveY[slot], ok ? rec.name : "LUMI", ok ? rec.generation : 0, isSelected, slot);

      if (isSelected) {
        drawHoveringWisp(graveX[slot], graveY[slot]);
      }
    }
  }

  // Scroll arrow guides if list exceeds 3 records
  if (windowStart > 0) {
    carrier->display.setTextColor(color565(120, 210, 255));
    carrier->display.setCursor(38, 148);
    carrier->display.print("<");
  }
  if (windowStart + 3 < totalCount) {
    carrier->display.setTextColor(color565(120, 210, 255));
    carrier->display.setCursor(200, 148);
    carrier->display.print(">");
  }

  // 5. Header Title
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(titleGold);
  const char* title = "* DE DOEDES SKOV *";
  int tLen = (int)strlen(title);
  carrier->display.setCursor(120 - (tLen * 3), 16);
  carrier->display.print(title);

  // Subtitle with selected grave name
  char subBuf[40];
  if (activeRecordLoaded) {
    snprintf(subBuf, sizeof(subBuf), "< GRAV %u AF %u: %s >", (unsigned int)(selectedIndex + 1), (unsigned int)totalCount, activeRecord.name);
  } else {
    snprintf(subBuf, sizeof(subBuf), "< GRAV %u AF %u >", (unsigned int)(selectedIndex + 1), (unsigned int)totalCount);
  }
  int sLen = (int)strlen(subBuf);
  carrier->display.setTextColor(subCyan);
  carrier->display.setCursor(120 - (sLen * 3), 28);
  carrier->display.print(subBuf);

  // 6. Bottom Navigation Bar
  carrier->display.setTextColor(navColor);
  const char* navHint = "T0:<  T1:>  T2:Livscyklus  T3:Ud";
  int nLen = (int)strlen(navHint);
  carrier->display.setCursor(120 - (nLen * 3), 212);
  carrier->display.print(navHint);
}

void UiMemorialRenderer::drawLifeCycleDetail(uint16_t index, uint16_t totalCount, const GraveRecord& record, uint16_t currentGen) {
  if (carrier == nullptr) return;
  (void)currentGen;

  uint16_t bgColor    = color565(6, 10, 24);
  uint16_t rimColor   = color565(26, 40, 72);
  uint16_t titleGold  = color565(255, 215, 80);
  uint16_t subCyan    = color565(120, 210, 255);
  uint16_t cardBg     = color565(12, 18, 36);
  uint16_t cardBorder = color565(45, 70, 115);
  uint16_t navColor   = color565(120, 170, 220);

  carrier->display.fillScreen(bgColor);
  carrier->display.drawCircle(120, 120, 118, rimColor);
  drawMysticalSkyAndMoon();

  // 1. Screen Title (Y = 16)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(titleGold);
  char titleBuf[36];
  snprintf(titleBuf, sizeof(titleBuf), "* %s'S LIVSCYKLUS *", record.name);
  int tLen = (int)strlen(titleBuf);
  carrier->display.setCursor(120 - (tLen * 3), 16);
  carrier->display.print(titleBuf);

  // 2. Subtitle pill (Y = 28)
  char subBuf[32];
  snprintf(subBuf, sizeof(subBuf), "< FORFADER %u AF %u >", (unsigned int)(index + 1), (unsigned int)totalCount);
  int sLen = (int)strlen(subBuf);
  carrier->display.setTextColor(subCyan);
  carrier->display.setCursor(120 - (sLen * 3), 28);
  carrier->display.print(subBuf);

  // 3. Center Visual: Stele at left (cx=92, cy=60), Hovering spirit at right (cx=146, cy=56)
  drawAncestralStele(92, 60);
  drawHoveringSpirit(146, 56);

  // 4. Ancestral Chronicle Card (X=18..221, Y=86..196, width 204 px, height 110 px)
  carrier->display.fillRoundRect(18, 86, 204, 110, 6, cardBg);
  carrier->display.drawRoundRect(18, 86, 204, 110, 6, cardBorder);

  // Line 1 (Y = 92): Pet name, Generation pill & age in days
  char genAgeBuf[40];
  snprintf(genAgeBuf, sizeof(genAgeBuf), "%s (GEN %u) | %lu dage", record.name, (unsigned int)record.generation, (unsigned long)record.ageMinutes);
  int gLen = (int)strlen(genAgeBuf);
  carrier->display.setTextColor(color565(255, 220, 90));
  carrier->display.setCursor(120 - (gLen * 3), 92);
  carrier->display.print(genAgeBuf);

  // Line 2 (Y = 106): Cause of death with thematic color
  uint16_t causeColor;
  switch (record.deathCause) {
    case DEATH_HUNGER:     causeColor = color565(255, 125, 90);  break;
    case DEATH_COLD:       causeColor = color565(110, 225, 255); break;
    case DEATH_HEAT:       causeColor = color565(255, 95, 75);   break;
    case DEATH_BOREDOM:    causeColor = color565(210, 150, 255); break;
    case DEATH_EXHAUSTION: causeColor = color565(185, 185, 205); break;
    case DEATH_OLD_AGE:    causeColor = color565(255, 225, 100); break;
    default:               causeColor = color565(200, 210, 225); break;
  }
  const char* causeName = PetLogic::getDeathCauseName(record.deathCause);
  char causeBuf[36];
  snprintf(causeBuf, sizeof(causeBuf), "Doed: %s", causeName);
  int cLen = (int)strlen(causeBuf);
  carrier->display.setTextColor(causeColor);
  carrier->display.setCursor(120 - (cLen * 3), 106);
  carrier->display.print(causeBuf);

  // Line 3 & 4 (Y = 120..136): Danish Epitaph
  char epBuf[42];
  snprintf(epBuf, sizeof(epBuf), "\"%s\"", record.epitaph);
  int epLen = (int)strlen(epBuf);

  if (epLen <= 28) {
    carrier->display.setTextColor(color565(215, 230, 245));
    carrier->display.setCursor(120 - (epLen * 3), 122);
    carrier->display.print(epBuf);
  } else {
    int splitIdx = 22;
    for (int i = 20; i <= 28 && i < epLen; i++) {
      if (epBuf[i] == ' ') {
        splitIdx = i;
        break;
      }
    }
    char line1[30];
    char line2[30];
    strncpy(line1, epBuf, splitIdx);
    line1[splitIdx] = '\0';
    const char* pLine2 = epBuf + splitIdx + (epBuf[splitIdx] == ' ' ? 1 : 0);
    strncpy(line2, pLine2, sizeof(line2) - 1);
    line2[sizeof(line2) - 1] = '\0';

    int l1Len = (int)strlen(line1);
    int l2Len = (int)strlen(line2);
    carrier->display.setTextColor(color565(215, 230, 245));
    carrier->display.setCursor(120 - (l1Len * 3), 120);
    carrier->display.print(line1);
    carrier->display.setCursor(120 - (l2Len * 3), 131);
    carrier->display.print(line2);
  }

  // Line 5 (Y = 148): Minigames record stats
  char scoreBuf[36];
  snprintf(scoreBuf, sizeof(scoreBuf), "Pong: %u p  |  Lab: %u s", (unsigned int)record.highScorePong, (unsigned int)record.highScoreMaze);
  int scLen = (int)strlen(scoreBuf);
  carrier->display.setTextColor(color565(80, 240, 160));
  carrier->display.setCursor(120 - (scLen * 3), 148);
  carrier->display.print(scoreBuf);

  // Line 6 (Y = 164): Eulogy banner
  carrier->display.setTextColor(color565(90, 125, 165));
  const char* eulogy = "* Evigt fred i skovens hjerte *";
  int euLen = (int)strlen(eulogy);
  carrier->display.setCursor(120 - (euLen * 3), 164);
  carrier->display.print(eulogy);

  // 5. Bottom Navigation Hint (Y = 210)
  carrier->display.setTextColor(navColor);
  const char* navHint = "T0:<  T1:>  T3:Tilbage til skov";
  int nLen = (int)strlen(navHint);
  carrier->display.setCursor(120 - (nLen * 3), 210);
  carrier->display.print(navHint);
}

void UiMemorialRenderer::drawRecord(uint16_t index, uint16_t totalCount, const GraveRecord& record) {
  drawLifeCycleDetail(index, totalCount, record, 1);
}

void UiMemorialRenderer::drawEmpty(uint16_t currentGen) {
  if (carrier == nullptr) return;

  uint16_t bgColor    = color565(6, 10, 24);
  uint16_t rimColor   = color565(26, 40, 72);
  uint16_t titleGold  = color565(255, 215, 80);
  uint16_t cardBg     = color565(12, 18, 36);
  uint16_t cardBorder = color565(45, 70, 115);
  uint16_t textGold   = color565(255, 220, 90);
  uint16_t textGreen  = color565(90, 230, 140);
  uint16_t textWhite  = color565(220, 235, 250);
  uint16_t navColor   = color565(120, 170, 220);

  carrier->display.fillScreen(bgColor);
  carrier->display.drawCircle(120, 120, 118, rimColor);
  drawMysticalSkyAndMoon();

  // Pine trees on sides
  drawPineTree(16, 138, 2);
  drawPineTree(224, 138, 2);

  // Screen Title (Y = 18)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(titleGold);
  const char* title = "* LUMIS MINDELUND *";
  int tLen = (int)strlen(title);
  carrier->display.setCursor(120 - (tLen * 3), 18);
  carrier->display.print(title);

  // Cute blooming Kokiri seedling sprout in center
  carrier->display.fillRoundRect(106, 75, 28, 7, 3, color565(75, 50, 30));
  carrier->display.drawFastVLine(120, 60, 15, color565(70, 200, 90));
  carrier->display.fillCircle(116, 61, 3, color565(90, 235, 115));
  carrier->display.fillCircle(124, 61, 3, color565(90, 235, 115));
  carrier->display.fillCircle(120, 56, 3, color565(255, 230, 90));

  // Card box
  carrier->display.fillRoundRect(22, 92, 196, 100, 6, cardBg);
  carrier->display.drawRoundRect(22, 92, 196, 100, 6, cardBorder);

  carrier->display.setTextColor(textGold);
  const char* line1 = "INGEN FORFAEDRE ENDNU";
  carrier->display.setCursor(120 - ((int)strlen(line1) * 3), 104);
  carrier->display.print(line1);

  char genBuf[32];
  snprintf(genBuf, sizeof(genBuf), "Lumi Generation %u lever!", (unsigned int)currentGen);
  carrier->display.setTextColor(textGreen);
  carrier->display.setCursor(120 - ((int)strlen(genBuf) * 3), 124);
  carrier->display.print(genBuf);

  carrier->display.setTextColor(textWhite);
  const char* line3 = "Ingen faldne i slaegten.";
  carrier->display.setCursor(120 - ((int)strlen(line3) * 3), 142);
  carrier->display.print(line3);

  const char* line4 = "Pas godt paa dit kaeledyr!";
  carrier->display.setTextColor(color565(160, 190, 220));
  carrier->display.setCursor(120 - ((int)strlen(line4) * 3), 158);
  carrier->display.print(line4);

  carrier->display.setTextColor(navColor);
  const char* navHint = "T3: Tilbage til menu";
  int nLen = (int)strlen(navHint);
  carrier->display.setCursor(120 - (nLen * 3), 208);
  carrier->display.print(navHint);
}
