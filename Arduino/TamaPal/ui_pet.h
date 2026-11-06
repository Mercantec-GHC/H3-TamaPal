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
enum LumiExpression : uint8_t {
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
  EXPR_OVERHEATING,     // Overheated in winter clothes: Sweating droplet, flushed cheeks, panting
  EXPR_DEAD,            // Health == 0: Peaceful spirit with glowing halo
  EXPR_SICK             // Fever / illness: Pale sickly hue, thermometer, droopy eyes
};

// Ambient sky weather / time-of-day visual state
enum SkyWeatherMode : uint8_t {
  SKY_DAY_SUNNY = 0,    // Warm sun with pulsing ray dots + fairies
  SKY_NIGHT_STARRY,     // Crescent moon + twinkling stars + dimmed fairies
  SKY_WINTER_SNOW       // Drifting pixel snowflakes + frosty sparkles + fairies
};

class UiPetRenderer {
public:
  UiPetRenderer();

  // Attach hardware display pointer
  void begin(MKRIoTCarrier* hardwareCarrier);

  // Determine which expression Lumi should show based on vitals
  static LumiExpression determineExpression(const PetLogic& pet);

  // Determine sky weather mode (Sunny Day, Starry Night, or Winter Snow)
  static SkyWeatherMode determineSkyWeather(const PetLogic& pet, LumiExpression expr);

  // Render or animate Lumi on SCREEN_HOME (flicker-free delta updates)
  void updateHomeDisplay(const PetLogic& pet, unsigned long nowMs, bool forceRedraw = false);

  // Render Lumi performing a temporary action on SCREEN_ACTION (Feed, Play, Scold, Sleep)
  void drawActionScene(const PetLogic& pet,
                       LumiExpression actionExpr,
                       const char* title,
                       const char* subtitle,
                       unsigned long nowMs);

  // Render cinematic egg hatching sequence (SCREEN_HATCHING: 0=wobble, 1=crack, 2=breakout)
  void drawHatchingScene(uint8_t phase, unsigned long nowMs, uint16_t generation);

  // Trigger foamy soap bubble shower when shaking the board
  void triggerBath(unsigned long nowMs);
  bool isBathing(unsigned long nowMs) const;

private:
  MKRIoTCarrier* carrier;
  const char* lastMoodText;
  unsigned long lastAnimTickMs;

  int16_t lastDrawnCx;
  int16_t lastDrawnCy;
  int16_t lastNaviX;
  int16_t lastNaviY;
  int16_t lastSmallFairyY;
  int16_t lastZzzPhase;

  LumiExpression lastExpression;
  PetStage lastStage;
  PetOutfit lastOutfit;
  SkyWeatherMode lastWeather;
  bool stageInitialized;
  bool lastBlinking;
  bool lastPromptBlink;
  bool lastHadPoop;
  unsigned long bathEndTimeMs;

  // Draw seamless Kokiri forest glade platform and flora (cenotaph gravestone when dead)
  void drawForestStage(SkyWeatherMode weather, bool isDead = false);

  // Poop 💩 drawing and cleaning routines
  void drawPoop(int x, int y, int phase);
  void erasePoop(int x, int y, uint16_t bgColor);

  // Foamy soap bubble shower animation during bath
  void drawSoapBubbles(unsigned long nowMs);

  // Draw bottom memorial card when Lumi is deceased
  void drawMemorialCard(const PetLogic& pet, unsigned long nowMs, bool force);

  // Draw animated sky weather (sun with ray dots, night stars, or winter snowflakes)
  void updateSkyWeather(SkyWeatherMode weather, int phase, uint16_t bgColor, bool force);

  // Helper to draw a crisp pixel snowflake at (x, y)
  void drawSnowflake(int x, int y, uint16_t color);

  // Draw bottom Danish mood status bar
  void drawMoodBanner(const char* moodText, LumiExpression expr);

  // Erase only the exposed crescent/margins from previous frame to prevent flicker
  void eraseOldCharacterRim(int oldCx, int oldCy, int newCx, int newCy, uint16_t bgColor);

  // Core 2.5D character drawing routine
  void drawLumiCharacter(int cx, int cy, LumiExpression expr, bool isBlinking, PetStage stage, PetOutfit outfit, uint16_t bgColor);

  // Draw eyes & mouth based on expression and stage
  void drawFaceFeatures(int cx, int cy, LumiExpression expr, bool isBlinking, PetStage stage, PetOutfit outfit, uint16_t bodyColor);

  // Draw floating Navi fairy + secondary miniature forest fairy
  void updateFairies(int naviX, int naviY, int smallFairyY, bool isSleeping, uint16_t bgColor, bool force);

  // Draw floating Zzz or thought-bubble props on the right side
  void updateSideOverlay(int baseCx, int baseCy, LumiExpression expr, int phase, uint16_t bgColor, bool force);

  // Pack 24-bit RGB into 16-bit RGB565 for ST7789 display
  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
  }
};

#endif // UI_PET_H
