// WHAT: Procedural 2.5D character renderer for Lumi (Tamagotchi Paradise + Zelda Kokiri + Nintendo style)
// WHY:  Brings Lumi to life on the 240x240 ST7789 display with zero RAM framebuffer and zero flicker
// USED BY: TamaPal.ino

#ifndef UI_PET_H
#define UI_PET_H

#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "pet_logic.h"

// Distinct emotional & action states for Lumi's facial expressions and posture
enum LumiExpression {
  EXPR_SUPER_HAPPY = 0, // Overall health >= 80%: Sparkling eyes, cat-smile, perky ears
  EXPR_CONTENT,         // Normal state: Glossy anime eyes with dual catchlights, warm smile
  EXPR_SLEEPING,        // Asleep: Peaceful closed eyes (- -), slow breath, floating Zzz
  EXPR_HUNGRY,          // Hunger < 20%: Pleading puppy eyes, open 'o' mouth, berry thought bubble
  EXPR_FREEZING,        // Warmth < 20%: Frosty blue tint, shivering, > < squint, wavy mouth
  EXPR_EXHAUSTED,       // Energy < 20% (awake): Half-lidded droopy eyes, yawning
  EXPR_SAD,             // Happiness < 20%: Downturned eyes with shiny tear drop, frown
  EXPR_EATING,          // Action mode: Munching a red Kokiri berry with ^ ^ joyful eyes
  EXPR_PLAYING,         // Action mode: Bouncing happily with stars & Navi spinning
  EXPR_SCOLDED,         // Action mode: Blushed pouty face with manga anger/sweat mark
  EXPR_DEAD             // Health == 0: Peaceful spirit with glowing halo
};

class UiPetRenderer {
public:
  UiPetRenderer();

  // Attach hardware display pointer
  void begin(MKRIoTCarrier* hardwareCarrier);

  // Determine which expression Lumi should show based on vitals
  static LumiExpression determineExpression(const PetLogic& pet);

  // Render or animate Lumi on SCREEN_HOME (flicker-free delta updates)
  void updateHomeDisplay(const PetLogic& pet, unsigned long nowMs, bool forceRedraw = false);

  // Render Lumi performing a temporary action on SCREEN_ACTION (Feed, Play, Scold, Sleep)
  void drawActionScene(const PetLogic& pet,
                       LumiExpression actionExpr,
                       const char* title,
                       const char* subtitle,
                       unsigned long nowMs);

private:
  MKRIoTCarrier* carrier;

  bool stageInitialized;
  int lastDrawnCx;
  int lastDrawnCy;
  int lastNaviX;
  int lastNaviY;
  int lastZzzPhase;
  bool lastBlinking;
  LumiExpression lastExpression;
  const char* lastMoodText;
  unsigned long lastAnimTickMs;

  // Draw static Kokiri forest glade background, mossy platform, and flora
  void drawForestStage(bool isNightMode);

  // Draw bottom Danish mood status bar
  void drawMoodBanner(const char* moodText, LumiExpression expr);

  // Erase only the exposed crescent/margins from previous frame to prevent flicker
  void eraseOldCharacterRim(int oldCx, int oldCy, int newCx, int newCy, uint16_t bgColor);

  // Core 2.5D character drawing routine
  void drawLumiCharacter(int cx, int cy, LumiExpression expr, bool isBlinking, uint16_t bgColor);

  // Draw eyes & mouth based on expression
  void drawFaceFeatures(int cx, int cy, LumiExpression expr, bool isBlinking, uint16_t bodyColor);

  // Draw floating Navi fairy companion
  void updateNaviFairy(int targetX, int targetY, bool isSleeping, uint16_t bgColor, bool force);

  // Draw floating Zzz or thought-bubble props on the right side
  void updateSideOverlay(int baseCx, int baseCy, LumiExpression expr, int phase, uint16_t bgColor, bool force);

  // Pack 24-bit RGB into 16-bit RGB565 for ST7789 display
  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_PET_H
