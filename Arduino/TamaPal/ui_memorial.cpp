// WHAT: Graphical renderer for the Hall of Fame / Ancestral Memorial gallery (SCREEN_MEMORIAL)
// WHY:  Renders ancestors from grave.txt with cenotaph stele, stats, and Danish epitaphs
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

void UiMemorialRenderer::drawBackgroundStars() {
  if (carrier == nullptr) return;

  uint16_t starCyan  = color565(180, 225, 255);
  uint16_t starGold  = color565(255, 230, 120);
  uint16_t starFaint = color565(90, 120, 170);

  // Faint mystical stars scattered around the circular perimeter
  carrier->display.drawPixel(35, 32, starCyan);
  carrier->display.drawPixel(52, 20, starFaint);
  carrier->display.drawPixel(188, 22, starGold);
  carrier->display.drawPixel(205, 34, starCyan);
  carrier->display.drawPixel(26, 75, starFaint);
  carrier->display.drawPixel(214, 76, starFaint);
}

void UiMemorialRenderer::drawAncestralStele(int cx, int cy) {
  if (carrier == nullptr) return;

  uint16_t plinthDark   = color565(35, 45, 65);
  uint16_t stoneBody    = color565(60, 75, 100);
  uint16_t stoneBevel   = color565(95, 115, 145);
  uint16_t carvedLeaf   = color565(110, 220, 160);
  uint16_t leafShadow   = color565(30, 80, 50);

  // Stele base plinth (32 px wide, 6 px high)
  carrier->display.fillRoundRect(cx - 16, cy + 12, 32, 6, 2, plinthDark);

  // Upright gravestone slab (24 px wide, 30 px high, rounded arch top)
  carrier->display.fillRoundRect(cx - 12, cy - 18, 24, 30, 5, stoneBody);
  carrier->display.drawRoundRect(cx - 12, cy - 18, 24, 30, 5, stoneBevel);

  // Sacred carved Kokiri leaf symbol in the center of the stone
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

  // Soft spherical ethereal aura
  carrier->display.fillCircle(cx, cy - 2, 14, auraGlow);

  // Golden circular halo hovering above
  carrier->display.drawFastHLine(cx - 4, cy - 14, 9, haloGold);
  carrier->display.drawPixel(cx - 5, cy - 13, haloGold);
  carrier->display.drawPixel(cx + 5, cy - 13, haloGold);

  // Cute chibi ghost body (16x16 rounded form)
  carrier->display.fillRoundRect(cx - 8, cy - 10, 16, 15, 6, spiritBody);

  // Peaceful closed sleeping eye curves (- -)
  carrier->display.drawFastHLine(cx - 5, cy - 4, 3, spiritEyes);
  carrier->display.drawFastHLine(cx + 3, cy - 4, 3, spiritEyes);

  // Floating ethereal tail wisps
  carrier->display.fillRect(cx - 6, cy + 5, 3, 3, spiritWisp);
  carrier->display.fillRect(cx - 1, cy + 5, 3, 5, spiritWisp);
  carrier->display.fillRect(cx + 4, cy + 5, 3, 3, spiritWisp);
  carrier->display.drawPixel(cx, cy + 10, wispDim);
}

void UiMemorialRenderer::drawRecord(uint16_t index, uint16_t totalCount, const GraveRecord& record) {
  if (carrier == nullptr) return;

  uint16_t bgColor    = color565(6, 10, 24);
  uint16_t rimColor   = color565(26, 40, 72);
  uint16_t titleGold  = color565(255, 215, 80);
  uint16_t subCyan    = color565(120, 210, 255);
  uint16_t cardBg     = color565(12, 18, 36);
  uint16_t cardBorder = color565(45, 70, 115);
  uint16_t navColor   = color565(120, 170, 220);

  carrier->display.fillScreen(bgColor);

  // Outer circular border guide
  carrier->display.drawCircle(120, 120, 118, rimColor);

  // Faint background stars
  drawBackgroundStars();

  // 1. Screen Title (Y = 16)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(titleGold);
  const char* title = "* LUMIS MINDELUND *";
  int tLen = (int)strlen(title);
  carrier->display.setCursor(120 - (tLen * 3), 16);
  carrier->display.print(title);

  // 2. Subtitle pill (Y = 28): < FORFADER X AF Y >
  char subBuf[32];
  snprintf(subBuf, sizeof(subBuf), "< FORFADER %u AF %u >", (unsigned int)(index + 1), (unsigned int)totalCount);
  int sLen = (int)strlen(subBuf);
  carrier->display.setTextColor(subCyan);
  carrier->display.setCursor(120 - (sLen * 3), 28);
  carrier->display.print(subBuf);

  // 3. Center Visual: Stele at left (cx=92, cy=64), Hovering spirit at right (cx=146, cy=60)
  drawAncestralStele(92, 64);
  drawHoveringSpirit(146, 60);

  // 4. Ancestral Chronicle Card (X=18..221, Y=90..196, width 204 px, height 106 px)
  carrier->display.fillRoundRect(18, 90, 204, 106, 6, cardBg);
  carrier->display.drawRoundRect(18, 90, 204, 106, 6, cardBorder);

  // Line 1 (Y = 96): Generation pill & age
  char genAgeBuf[36];
  snprintf(genAgeBuf, sizeof(genAgeBuf), "GEN %u  |  Alder: %lu min", (unsigned int)record.generation, (unsigned long)record.ageMinutes);
  int gLen = (int)strlen(genAgeBuf);
  carrier->display.setTextColor(color565(255, 220, 90));
  carrier->display.setCursor(120 - (gLen * 3), 96);
  carrier->display.print(genAgeBuf);

  // Line 2 (Y = 110): Cause of death with thematic color
  uint16_t causeColor;
  switch (record.deathCause) {
    case DEATH_HUNGER:     causeColor = color565(255, 125, 90);  break; // Coral
    case DEATH_COLD:       causeColor = color565(110, 225, 255); break; // Ice cyan
    case DEATH_HEAT:       causeColor = color565(255, 95, 75);   break; // Crimson
    case DEATH_BOREDOM:    causeColor = color565(210, 150, 255); break; // Lilac
    case DEATH_EXHAUSTION: causeColor = color565(185, 185, 205); break; // Slate
    case DEATH_OLD_AGE:    causeColor = color565(255, 225, 100); break; // Golden glow
    default:               causeColor = color565(200, 210, 225); break;
  }
  const char* causeName = PetLogic::getDeathCauseName(record.deathCause);
  char causeBuf[36];
  snprintf(causeBuf, sizeof(causeBuf), "Doed: %s", causeName);
  int cLen = (int)strlen(causeBuf);
  carrier->display.setTextColor(causeColor);
  carrier->display.setCursor(120 - (cLen * 3), 110);
  carrier->display.print(causeBuf);

  // Line 3 & 4 (Y = 125..145): Danish Epitaph in quotes
  char epBuf[42];
  snprintf(epBuf, sizeof(epBuf), "\"%s\"", record.epitaph);
  int epLen = (int)strlen(epBuf);

  if (epLen <= 28) {
    // Fits neatly on a single line
    carrier->display.setTextColor(color565(215, 230, 245));
    carrier->display.setCursor(120 - (epLen * 3), 127);
    carrier->display.print(epBuf);
  } else {
    // Split into 2 lines near middle space
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
    carrier->display.setCursor(120 - (l1Len * 3), 125);
    carrier->display.print(line1);
    carrier->display.setCursor(120 - (l2Len * 3), 136);
    carrier->display.print(line2);
  }

  // Line 5 (Y = 152): Minigames record stats
  char scoreBuf[36];
  snprintf(scoreBuf, sizeof(scoreBuf), "Pong: %u p  |  Lab: %u s", (unsigned int)record.highScorePong, (unsigned int)record.highScoreMaze);
  int scLen = (int)strlen(scoreBuf);
  carrier->display.setTextColor(color565(80, 240, 160));
  carrier->display.setCursor(120 - (scLen * 3), 152);
  carrier->display.print(scoreBuf);

  // Line 6 (Y = 168): Eulogy banner
  carrier->display.setTextColor(color565(90, 125, 165));
  const char* eulogy = "* Evig fred i skoven *";
  int euLen = (int)strlen(eulogy);
  carrier->display.setCursor(120 - (euLen * 3), 168);
  carrier->display.print(eulogy);

  // 5. Bottom Navigation Hint (Y = 208)
  carrier->display.setTextColor(navColor);
  const char* navHint = "T0:<   T1:>   T3:Tilb";
  int nLen = (int)strlen(navHint);
  carrier->display.setCursor(120 - (nLen * 3), 208);
  carrier->display.print(navHint);
}

void UiMemorialRenderer::drawEmpty(uint16_t currentGen) {
  if (carrier == nullptr) return;

  uint16_t bgColor    = color565(6, 10, 24);
  uint16_t rimColor   = color565(26, 40, 72);
  uint16_t titleGold  = color565(255, 215, 80);
  uint16_t cardBg     = color565(12, 18, 36);
  uint16_t cardBorder = color565(45, 70, 115);
  uint16_t textGold   = color565(255, 220, 90);
  uint16_t textWhite  = color565(220, 235, 250);
  uint16_t textGreen  = color565(90, 230, 140);
  uint16_t navColor   = color565(120, 170, 220);

  carrier->display.fillScreen(bgColor);
  carrier->display.drawCircle(120, 120, 118, rimColor);
  drawBackgroundStars();

  // Screen Title (Y = 18)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(titleGold);
  const char* title = "* LUMIS MINDELUND *";
  int tLen = (int)strlen(title);
  carrier->display.setCursor(120 - (tLen * 3), 18);
  carrier->display.print(title);

  // Cute blooming Kokiri seedling sprout in center (cx=120, cy=62)
  carrier->display.fillRoundRect(106, 75, 28, 7, 3, color565(75, 50, 30)); // Soil mound
  carrier->display.drawFastVLine(120, 60, 15, color565(70, 200, 90));      // Stem
  carrier->display.fillCircle(116, 61, 3, color565(90, 235, 115));          // Left leaf
  carrier->display.fillCircle(124, 61, 3, color565(90, 235, 115));          // Right leaf
  carrier->display.fillCircle(120, 56, 3, color565(255, 230, 90));          // Golden bloom bud

  // Card box (Y = 92..192)
  carrier->display.fillRoundRect(22, 92, 196, 100, 6, cardBg);
  carrier->display.drawRoundRect(22, 92, 196, 100, 6, cardBorder);

  // Card Content
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

  // Bottom Navigation Hint (Y = 208)
  carrier->display.setTextColor(navColor);
  const char* navHint = "T3: Tilbage til menu";
  int nLen = (int)strlen(navHint);
  carrier->display.setCursor(120 - (nLen * 3), 208);
  carrier->display.print(navHint);
}
