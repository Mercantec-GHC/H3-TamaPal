// WHAT: Graphical renderer for the 5 Pet Vitals dashboard screen (SCREEN_STATS)
// WHY:  Draws 5 colorful horizontal progress bars and mood status with zero flicker
// USED BY: TamaPal.ino

#include "ui_stats.h"
#include <string.h>
#include <stdio.h>

UiStatsRenderer::UiStatsRenderer()
  : carrier(nullptr),
    lastMoodText(nullptr),
    lastHunger(-1),
    lastWarmth(-1),
    lastEnergy(-1),
    lastHappiness(-1),
    lastDiscipline(-1),
    isInitialized(false) {
}

void UiStatsRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
}

void UiStatsRenderer::drawInitialScreen() {
  if (carrier == nullptr) return;

  uint16_t bgColor = color565(8, 12, 26);
  carrier->display.fillScreen(bgColor);

  // Outer circular border guide
  carrier->display.drawCircle(120, 120, 118, color565(25, 45, 80));

  // Header Title (Y = 24, safe within round circular cutout)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(color565(0, 230, 255), bgColor);
  const char* title = "LUMI STATISTIK";
  int titleWidth = strlen(title) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - titleWidth) / 2, 24);
  carrier->display.print(title);

  // Reset cached values to trigger full redraw of all 5 bars
  lastHunger = -1;
  lastWarmth = -1;
  lastEnergy = -1;
  lastHappiness = -1;
  lastDiscipline = -1;
  lastMoodText = nullptr;
  isInitialized = true;

  // Bottom navigation hint
  carrier->display.setTextColor(color565(100, 135, 180), bgColor);
  const char* hint = "T3:Tilb  T2:Mindelund";
  int hintWidth = strlen(hint) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - hintWidth) / 2, 206);
  carrier->display.print(hint);
}

void UiStatsRenderer::drawSingleBar(int y, const char* label, int val, int16_t& lastVal, uint16_t barColor, bool force) {
  if (carrier == nullptr) return;

  uint16_t bgColor = color565(8, 12, 26);
  uint16_t frameColor = color565(40, 60, 95);

  if (val < 0) val = 0;
  if (val > 100) val = 100;

  // Draw static label and frame only on initial draw or force
  // X=36..60: Label (24 px) | X=66..170: Bar (104 px) | X=174..198: Percentage (24 px)
  // Total span X=36..198 = 162 px, perfectly centered with generous circular margins!
  if (force || lastVal == -1) {
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(color565(210, 225, 245), bgColor);
    carrier->display.setCursor(36, y + 2);
    carrier->display.print(label);

    // Empty bar outline frame (104 px wide, 11 px high, rounded corners)
    carrier->display.drawRoundRect(66, y, 104, 11, 2, frameColor);
  }

  // Dirty update: redraw fill & text only when value changes
  if (force || val != lastVal) {
    // If critical (< 20%), flash bar red for alert
    uint16_t activeColor = (val < PET_CRITICAL_THRESHOLD) ? color565(255, 60, 60) : barColor;

    // Fill inner bar: 100 px active span (X=68..167), 7 px height
    if (val > 0) {
      carrier->display.fillRect(68, y + 2, val, 7, activeColor);
    }
    // Clear empty remainder
    if (val < 100) {
      carrier->display.fillRect(68 + val, y + 2, 100 - val, 7, bgColor);
    }

    // Percentage text at X = 174
    char pctBuffer[8];
    snprintf(pctBuffer, sizeof(pctBuffer), "%3d%%", val);
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(color565(240, 245, 255), bgColor);
    carrier->display.setCursor(174, y + 2);
    carrier->display.print(pctBuffer);

    lastVal = val;
  }
}

void UiStatsRenderer::updateDisplay(const PetLogic& pet, bool forceRedraw) {
  if (carrier == nullptr) return;

  if (!isInitialized || forceRedraw) {
    drawInitialScreen();
  }

  uint16_t bgColor = color565(8, 12, 26);

  // 1. Sult (Food / Hunger) - Row Y = 46 (Green / Emerald)
  drawSingleBar(46, "SULT", pet.getHunger(), lastHunger, color565(0, 230, 120), forceRedraw);

  // 2. Varme (Warmth) - Row Y = 72 (Amber / Orange)
  drawSingleBar(72, "VARM", pet.getWarmth(), lastWarmth, color565(255, 160, 40), forceRedraw);

  // 3. Energi (Energy / Sleep) - Row Y = 98 (Sky Blue)
  drawSingleBar(98, "ENER", pet.getEnergy(), lastEnergy, color565(0, 220, 255), forceRedraw);

  // 4. Glæde (Happiness) - Row Y = 124 (Sunny Gold)
  drawSingleBar(124, "GLAD", pet.getHappiness(), lastHappiness, color565(255, 215, 0), forceRedraw);

  // 5. Disciplin (Discipline) - Row Y = 150 (Royal Purple)
  drawSingleBar(150, "DISC", pet.getDiscipline(), lastDiscipline, color565(180, 110, 255), forceRedraw);

  // 6. Mood Status String (Row Y = 176)
  const char* currentMood = pet.getMoodStatusDanish();
  if (forceRedraw || currentMood != lastMoodText) {
    // Clear status text area cleanly
    carrier->display.fillRect(24, 174, 192, 14, bgColor);

    // Color code based on mood
    uint16_t moodColor;
    if (pet.isSleeping()) {
      moodColor = color565(120, 190, 255); // Calming cyan
    } else if (pet.needsAttention()) {
      moodColor = color565(255, 75, 75);   // Red alert
    } else if (pet.getOverallHealth() >= 80) {
      moodColor = color565(100, 255, 130); // Vibrant green
    } else {
      moodColor = color565(255, 220, 120); // Warm yellow
    }

    carrier->display.setTextSize(1);
    carrier->display.setTextColor(moodColor, bgColor);
    int moodWidth = strlen(currentMood) * 6;
    carrier->display.setCursor((SCREEN_WIDTH - moodWidth) / 2, 176);
    carrier->display.print(currentMood);

    lastMoodText = currentMood;
  }

  // 7. Berry Bank wallet display (Row Y = 192)
  carrier->display.fillRect(24, 190, 192, 14, bgColor);
  char berryBuffer[24];
  snprintf(berryBuffer, sizeof(berryBuffer), "[ %u GYLDNE BAER ]", (unsigned int)pet.getBerries());
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(color565(255, 215, 60), bgColor);
  int bWidth = strlen(berryBuffer) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - bWidth) / 2, 192);
  carrier->display.print(berryBuffer);
}

