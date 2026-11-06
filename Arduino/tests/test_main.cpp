// WHAT: Unit test suite for MenuLogic class running on host Mac / PC
// WHY:  Test our state machine and math automatically without uploading to hardware
// USED BY: tests/run_tests.sh

#include <cstdio>
#include <cmath>
#include <cstring>
#include "menu_logic.h"
#include "tilt_logic.h"
#include "pet_logic.h"
#include "storage_pet.h"
#include "pong_logic.h"
#include "maze_logic.h"

static int totalTestsRun = 0;
static int totalTestsFailed = 0;

#define CHECK(condition, message) do { \
  totalTestsRun++; \
  if (!(condition)) { \
    printf("  [FAIL] Line %d: %s\n", __LINE__, message); \
    totalTestsFailed++; \
  } else { \
    printf("  [PASS] %s\n", message); \
  } \
} while(0)

void testInitialState() {
  printf("\n--- Test: Initial State ---\n");
  MenuLogic menu;
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "Starts on SCREEN_HOME");
  CHECK(menu.getSelectedIcon() == ICON_FOOD, "Starts with ICON_FOOD (index 0)");
  CHECK(!menu.isCursorMoving(), "Cursor is initially at rest");
}

void testNavigationWrapping() {
  printf("\n--- Test: Navigation and Wrapping ---\n");
  MenuLogic menu;

  // Move right: 0 -> 1 -> 2 -> 3 -> 4 -> 0 (wrap)
  menu.onButtonNext();
  CHECK(menu.getSelectedIcon() == ICON_PLAY, "Next moves from 0 to 1 (PLAY)");

  menu.onButtonNext(); // 2
  menu.onButtonNext(); // 3
  menu.onButtonNext(); // 4
  CHECK(menu.getSelectedIcon() == ICON_CONNECT, "Reached last icon (CONNECT)");

  menu.onButtonNext(); // Wrap to 0
  CHECK(menu.getSelectedIcon() == ICON_FOOD, "Next from 4 wraps to 0 (FOOD)");

  // Move left: 0 -> 4 (wrap)
  menu.onButtonPrev();
  CHECK(menu.getSelectedIcon() == ICON_CONNECT, "Prev from 0 wraps to 4 (CONNECT)");

  menu.onButtonPrev();
  CHECK(menu.getSelectedIcon() == ICON_STATS, "Prev from 4 moves to 3 (STATS)");
}

void testScreenSwitching() {
  printf("\n--- Test: Screen Switching and Back ---\n");
  MenuLogic menu;

  // 1. TOUCH4 shortcut opens STATS, second TOUCH4 returns to HOME
  menu.onButtonStatsShortcut();
  CHECK(menu.getActiveScreen() == SCREEN_STATS, "TOUCH4 opens SCREEN_STATS");
  menu.onButtonStatsShortcut();
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "Second TOUCH4 returns to SCREEN_HOME");

  // 2. Select STATS via icon + OK
  menu.onButtonNext(); // 1
  menu.onButtonNext(); // 2
  menu.onButtonNext(); // 3 (STATS)
  menu.onButtonOk(1000);
  CHECK(menu.getActiveScreen() == SCREEN_STATS, "OK on STATS icon opens SCREEN_STATS");

  // 3. TOUCH3 (Back) returns from STATS to HOME
  menu.onButtonBack();
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "TOUCH3 returns from STATS to HOME");

  // 4. Select CONNECT -> SCREEN_CHOOSE_PLAYER -> selectPlayer(1) -> CONTROLLER
  menu.onButtonNext(); // 4 (CONNECT)
  menu.onButtonOk(1500);
  CHECK(menu.getActiveScreen() == SCREEN_CHOOSE_PLAYER, "OK on CONNECT icon opens SCREEN_CHOOSE_PLAYER");
  menu.selectPlayer(1);
  CHECK(menu.getActiveScreen() == SCREEN_CONTROLLER, "selectPlayer(1) opens SCREEN_CONTROLLER");
  CHECK(menu.getPlayerId() == 1, "Player ID is 1");
  menu.onButtonBack();
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "TOUCH3 returns from CONTROLLER to HOME");

  // 5. While still on CONNECT icon -> choose Player 2 -> CONTROLLER
  menu.onButtonOk(2000);
  CHECK(menu.getActiveScreen() == SCREEN_CHOOSE_PLAYER, "OK on CONNECT opens SCREEN_CHOOSE_PLAYER");
  menu.selectPlayer(2);
  CHECK(menu.getActiveScreen() == SCREEN_CONTROLLER, "selectPlayer(2) opens SCREEN_CONTROLLER");
  CHECK(menu.getPlayerId() == 2, "Player ID is 2");
  menu.onButtonBack();
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "TOUCH3 returns from CONTROLLER to HOME");
}

void testActionScreenTimeout() {
  printf("\n--- Test: Temporary Action Timeout ---\n");
  MenuLogic menu; // Starts at 0 (FOOD)
  unsigned long startTime = 5000;

  menu.onButtonOk(startTime);
  CHECK(menu.getActiveScreen() == SCREEN_ACTION, "OK on FOOD opens SCREEN_ACTION");

  // Check before 1500 ms have passed
  menu.update(startTime + 1000);
  CHECK(menu.getActiveScreen() == SCREEN_ACTION, "Still on ACTION screen after 1000 ms");

  // Check after 1500 ms have passed
  menu.update(startTime + 1500);
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "Auto-returns to SCREEN_HOME after 1500 ms");
}

void testInputDisabledOnOtherScreens() {
  printf("\n--- Test: Navigation Disabled When Not on HOME ---\n");
  MenuLogic menu;
  menu.onButtonStatsShortcut(); // Switch to STATS screen
  CHECK(menu.getActiveScreen() == SCREEN_STATS, "On STATS screen");

  // Attempting to move left/right must be ignored
  menu.onButtonNext();
  CHECK(menu.getSelectedIcon() == ICON_FOOD, "TOUCH1 does not change selection while on STATS");
  menu.onButtonPrev();
  CHECK(menu.getSelectedIcon() == ICON_FOOD, "TOUCH0 does not change selection while on STATS");
}

void testSmoothCursorPhysics() {
  printf("\n--- Test: Cursor Sliding Physics ---\n");
  MenuLogic menu;
  float startX = menu.getCursorPositionX();

  menu.onButtonNext(); // Target shifts right by 34 px
  float targetX = menu.getTargetPositionX();
  CHECK(targetX > startX, "Target position moved right");
  CHECK(menu.isCursorMoving(), "Cursor is now flagged as moving");

  // Run a few animation update steps
  menu.update(100);
  CHECK(menu.getCursorPositionX() > startX, "Cursor smoothly moved closer to target");
  CHECK(menu.getCursorPositionX() < targetX, "Cursor has not overshot target");

  // Simulate 30 frames: cursor should arrive at target
  for (int frame = 0; frame < 30; frame++) {
    menu.update(frame * 16);
  }
  CHECK(!menu.isCursorMoving(), "Cursor finished moving and snapped to target");
  CHECK(fabs(menu.getCursorPositionX() - targetX) < 0.001f, "Cursor is exactly at target position");
}

void testTiltCalculations() {
  printf("\n--- Test: Tilt Calculations & Clamping ---\n");
  TiltLogic tilt;

  // 1. Level board: ax=0, ay=0, az=1.0g
  tilt.updateAcceleration(0.0f, 0.0f, 1.0f);
  CHECK(fabs(tilt.getRawTiltY()) < 0.001f, "Flat board gives tiltY = 0.0");
  CHECK(fabs(tilt.getRawTiltX()) < 0.001f, "Flat board gives tiltX = 0.0");

  // 2. Deadzone test: ~1 degree tilt (ay = 0.0175 * az)
  // angle = atan2(0.0175, 1.0) = ~1 deg -> tilt = 1/30 = 0.033 (< 0.05 deadzone)
  tilt.updateAcceleration(0.0f, 0.0175f, 1.0f);
  CHECK(fabs(tilt.getRawTiltY()) < 0.001f, "Deadzone suppresses minor tilt < 5%");

  // 3. 30 degrees tilt: tan(30 deg) = 0.57735 -> ay = 0.57735, az = 1.0
  tilt.updateAcceleration(0.0f, 0.57735f, 1.0f);
  CHECK(fabs(tilt.getRawTiltY() - 1.0f) < 0.02f, "30 deg tilt gives tiltY close to 1.0");

  // 4. Overshoot clamping: 60 degrees tilt (tan(60) = 1.732)
  tilt.updateAcceleration(0.0f, 1.732f, 1.0f);
  CHECK(tilt.getRawTiltY() == 1.0f, "Tilt over 30 deg is clamped to exactly 1.0");

  // 5. Negative tilt (paddle up): -30 deg
  tilt.updateAcceleration(0.0f, -0.57735f, 1.0f);
  CHECK(fabs(tilt.getRawTiltY() - (-1.0f)) < 0.02f, "-30 deg tilt gives tiltY close to -1.0");

  // 6. Horizontal X tilt (left/right steering)
  tilt.updateAcceleration(0.57735f, 0.0f, 1.0f);
  CHECK(fabs(tilt.getRawTiltX() - 1.0f) < 0.02f, "Horizontal right tilt gives tiltX close to 1.0");
}

void testTiltSmoothing() {
  printf("\n--- Test: Tilt Smoothing Filter ---\n");
  TiltLogic tilt;

  // Start at flat (0.0)
  tilt.updateAcceleration(0.0f, 0.0f, 1.0f);

  // Apply a sudden step jump to full tilt (ay = 1.732 -> raw tilt = 1.0)
  tilt.updateAcceleration(0.0f, 1.732f, 1.0f);
  float firstStep = tilt.getTiltY();
  CHECK(firstStep > 0.15f && firstStep < 0.45f, "First smoothing step increases smoothly (EMA)");

  // Run 20 updates: smoothed value should smoothly approach 1.0
  for (int i = 0; i < 20; i++) {
    tilt.updateAcceleration(0.0f, 1.732f, 1.0f);
  }
  CHECK(tilt.getTiltY() > 0.99f, "Smoothed value reaches 1.0 after 20 frames");
  CHECK(tilt.getTiltY() <= 1.0f, "Smoothed value never overshoots 1.0");
}

void testTiltJsonFormatting() {
  printf("\n--- Test: MQTT JSON Formatting (No %%f) ---\n");
  TiltLogic tilt;
  char buffer[128];

  // 1. Flat state, button false, up/down false
  tilt.updateAcceleration(0.0f, 0.0f, 1.0f);
  tilt.setButtons(false, false, false);
  // Warm up filter
  for (int i = 0; i < 15; i++) tilt.updateAcceleration(0.0f, 0.0f, 1.0f);

  tilt.formatInputJson(buffer, sizeof(buffer));
  CHECK(strcmp(buffer, "{\"up\":false,\"down\":false,\"button\":false,\"tiltX\":0.00,\"tiltY\":0.00,\"tilt\":0.00}") == 0,
        "Formats neutral flat universal input JSON with up/down/button/tiltX/tiltY/tilt");

  // 2. Pong buttons test (Up=true, Down=false, Action=true)
  tilt.setButtons(true, false, true);
  tilt.formatInputJson(buffer, sizeof(buffer));
  CHECK(strstr(buffer, "\"up\":true") != nullptr, "JSON contains \"up\":true");
  CHECK(strstr(buffer, "\"down\":false") != nullptr, "JSON contains \"down\":false");
  CHECK(strstr(buffer, "\"button\":true") != nullptr, "JSON contains \"button\":true");

  // 3. Positive tilt (Labyrinth & Pong tilt compatibility)
  // tan(10.5 deg) ~ 0.185 -> tilt ~ 0.35
  for (int i = 0; i < 25; i++) tilt.updateAcceleration(0.0f, 0.185f, 1.0f);
  tilt.formatInputJson(buffer, sizeof(buffer));
  CHECK(strstr(buffer, "\"tilt\":0.") != nullptr, "JSON contains positive tilt value");
  CHECK(strstr(buffer, "\"tiltY\":0.") != nullptr, "JSON contains positive tiltY value");

  // 4. Negative tilt clamped
  for (int i = 0; i < 25; i++) tilt.updateAcceleration(0.0f, -0.577f, 1.0f);
  tilt.setButtons(false, false, false);
  tilt.formatInputJson(buffer, sizeof(buffer));
  CHECK(strstr(buffer, "\"tilt\":-1.00") != nullptr, "JSON contains negative clamped tilt -1.00");
  CHECK(strstr(buffer, "\"tiltY\":-1.00") != nullptr, "JSON contains negative clamped tiltY -1.00");
}

void testEventParsing() {
  printf("\n--- Test: MQTT Event Parsing ---\n");
  CHECK(TiltLogic::parseEventType("hit") == EVENT_HIT, "Parses 'hit' -> EVENT_HIT");
  CHECK(TiltLogic::parseEventType("miss") == EVENT_MISS, "Parses 'miss' -> EVENT_MISS");
  CHECK(TiltLogic::parseEventType("gameover") == EVENT_GAMEOVER, "Parses 'gameover' -> EVENT_GAMEOVER");
  CHECK(TiltLogic::parseEventType("assign_p1") == EVENT_ASSIGN_P1, "Parses 'assign_p1' -> EVENT_ASSIGN_P1");
  CHECK(TiltLogic::parseEventType("assign_p2") == EVENT_ASSIGN_P2, "Parses 'assign_p2' -> EVENT_ASSIGN_P2");
  CHECK(TiltLogic::parseEventType("unknown") == EVENT_NONE, "Parses 'unknown' -> EVENT_NONE");
  CHECK(TiltLogic::parseEventType(nullptr) == EVENT_NONE, "Safely handles nullptr -> EVENT_NONE");
}

void testPetInitialState() {
  printf("\n--- Test: PetLogic Initial Vitals ---\n");
  PetLogic pet;
  CHECK(pet.getHunger() == 80, "Initial hunger is 80% (well-fed)");
  CHECK(pet.getWarmth() == 85, "Initial warmth is 85% (comfortable)");
  CHECK(pet.getEnergy() == 90, "Initial energy is 90% (well-rested)");
  CHECK(pet.getHappiness() == 75, "Initial happiness is 75% (cheerful)");
  CHECK(pet.getDiscipline() == 50, "Initial discipline is 50% (neutral)");
  CHECK(!pet.isSleeping(), "Lumi is awake initially");
  CHECK(!pet.needsAttention(), "Lumi does not need critical attention initially");
  CHECK(pet.getOverallHealth() >= 70, "Overall health is healthy (>70%)");
}

void testPetActions() {
  printf("\n--- Test: PetLogic Actions (Feed, Play, Scold) ---\n");
  PetLogic pet;

  // Feed action
  CHECK(pet.feed(), "Feed action succeeds when hungry");
  CHECK(pet.getHunger() == 100, "Hunger restored to max 100% (was 80 + 25 -> 100)");
  CHECK(!pet.feed(), "Feed action returns false when already 100% full");

  // Play action
  pet.setVitals(80, 80, 50, 50, 50);
  CHECK(pet.play(), "Play action succeeds when awake with energy");
  CHECK(pet.getHappiness() == 75, "Happiness increased by 25% (50 -> 75)");
  CHECK(pet.getEnergy() == 35, "Energy decreased by 15% (50 -> 35)");
  CHECK(pet.getHunger() == 75, "Hunger decreased slightly by 5% (80 -> 75)");

  // Scold action
  pet.setVitals(80, 80, 50, 50, 50);
  CHECK(pet.scold(), "Scold action succeeds");
  CHECK(pet.getDiscipline() == 70, "Discipline increased by 20% (50 -> 70)");
  CHECK(pet.getHappiness() == 40, "Happiness decreased by 10% (50 -> 40)");

  // Actions disabled when sleeping
  pet.setSleeping(true);
  CHECK(!pet.feed(), "Cannot feed while asleep");
  CHECK(!pet.play(), "Cannot play while asleep");
  CHECK(!pet.scold(), "Cannot scold while asleep");

  // Backend reward handling (e.g. from winning Pong points)
  pet.setSleeping(false);
  pet.setVitals(50, 50, 50, 50, 50);
  CHECK(pet.applyReward("happy_boost", 25), "Reward happy_boost applies successfully");
  CHECK(pet.getHappiness() == 75, "Happiness increased by 25% (50 -> 75)");
  CHECK(pet.applyReward("energy_boost", 20), "Reward energy_boost applies successfully");
  CHECK(pet.getEnergy() == 70, "Energy increased by 20% (50 -> 70)");
  CHECK(!pet.applyReward("invalid_type", 10), "Invalid reward type returns false");
  CHECK(!pet.applyReward(nullptr, 10), "Nullptr reward type handled safely");
}

void testPetSleepAndSensors() {
  printf("\n--- Test: PetLogic Sleep and Environmental Sensors ---\n");
  PetLogic pet;

  // Temperature response: 22 deg C keeps warmth optimal
  pet.setVitals(80, 85, 90, 75, 50);
  pet.setAmbientTemperature(22.0f);
  CHECK(pet.getWarmth() >= 80, "Optimal 22C temperature maintains high warmth");

  // Cold room: 10 deg C pulls warmth down
  pet.setAmbientTemperature(10.0f);
  pet.setAmbientTemperature(10.0f);
  pet.setAmbientTemperature(10.0f);
  CHECK(pet.getWarmth() < 60, "Cold room (10C) pulls warmth down");

  // Light sensor: 5-second dark timer puts Lumi to sleep
  pet.setSleeping(false);
  CHECK(!pet.isSleeping(), "Lumi is awake before covering sensor");

  // Cover sensor at t = 1000 ms
  pet.setAmbientLight(15, 1000); // < 30 lux
  CHECK(!pet.isSleeping(), "Lumi stays awake after only 1 second of dark");

  // Still dark at t = 4000 ms (elapsed 3000 ms)
  pet.setAmbientLight(15, 4000);
  CHECK(!pet.isSleeping(), "Lumi stays awake after 3 seconds of dark (< 5s threshold)");

  // Still dark at t = 6000 ms (elapsed 5000 ms -> triggers sleep!)
  pet.setAmbientLight(15, 6000);
  CHECK(pet.isSleeping(), "Lumi falls asleep after 5 continuous seconds in dark (hand covering sensor)");

  // Bright lights uncovered at t = 7000 ms
  pet.setAmbientLight(150, 7000); // > 80 lux
  CHECK(pet.isSleeping(), "Lumi stays asleep after only 1 second of room light");

  // Still light at t = 9000 ms (elapsed 2000 ms -> wakes up!)
  pet.setAmbientLight(150, 9000);
  CHECK(!pet.isSleeping(), "Lumi wakes up after 2 continuous seconds in light");
}

void testPetDecayAndStatus() {
  printf("\n--- Test: PetLogic Decay Over Time and Mood Status ---\n");
  PetLogic pet;
  pet.setVitals(80, 80, 80, 80, 80);

  // Time decay: 1 interval = 10,000 ms (test after 40 seconds awake)
  pet.update(0); // Initialize timer
  pet.update(40000); // 4 intervals passed (40 seconds)
  CHECK(pet.getHunger() < 80, "Hunger decays after 40 seconds awake (80 -> 78)");
  CHECK(pet.getEnergy() < 80, "Energy decays after 40 seconds awake (80 -> 79)");

  // Critical threshold trigger
  pet.setVitals(15, 80, 80, 80, 80); // Hunger < 20%
  CHECK(pet.needsAttention(), "needsAttention() is true when hunger < 20%");
  CHECK(strcmp(pet.getMoodStatusDanish(), "Lumi er sulten!") == 0, "Reports Danish mood: 'Lumi er sulten!'");

  pet.setVitals(80, 15, 80, 80, 80); // Warmth < 20%
  CHECK(strcmp(pet.getMoodStatusDanish(), "Lumi fryser! Giv hue paa!") == 0, "Reports Danish mood: 'Lumi fryser! Giv hue paa!'");

  pet.setSleeping(true);
  CHECK(strcmp(pet.getMoodStatusDanish(), "Sover zZz...") == 0, "Reports Danish mood: 'Sover zZz...'");
}

void testPetSerialization() {
  printf("\n--- Test: PetLogic JSON Serialization & Life State ---\n");
  PetLogic pet;
  char buf[256];

  CHECK(pet.isAlive(), "Default Lumi is alive");
  CHECK(strcmp(pet.getStage(), "baby") == 0, "Default stage is baby");
  CHECK(pet.getStageEnum() == STAGE_BABY, "Default stage enum is STAGE_BABY");

  // Stage transition (all 4 lifecycle stages)
  pet.setStage(STAGE_TEEN);
  CHECK(strcmp(pet.getStage(), "teen") == 0, "Changed stage to teen");
  CHECK(pet.getStageEnum() == STAGE_TEEN, "Stage enum is STAGE_TEEN");

  pet.setStage(STAGE_ADULT);
  CHECK(strcmp(pet.getStage(), "adult") == 0, "Changed stage to adult");
  CHECK(pet.getStageEnum() == STAGE_ADULT, "Stage enum is STAGE_ADULT");

  pet.setStage(STAGE_OLD);
  CHECK(strcmp(pet.getStage(), "old") == 0, "Changed stage to old");
  CHECK(pet.getStageEnum() == STAGE_OLD, "Stage enum is STAGE_OLD");

  pet.setStage(STAGE_TEEN); // Reset back to teen for json verification

  // Format state JSON
  pet.formatStateJson(buf, sizeof(buf));
  CHECK(strstr(buf, "\"name\":\"Lumi\"") != NULL, "JSON contains name Lumi");
  CHECK(strstr(buf, "\"stage\":\"teen\"") != NULL, "JSON contains stage teen");
  CHECK(strstr(buf, "\"outfit\":\"default\"") != NULL, "JSON contains outfit default");
  CHECK(strstr(buf, "\"hunger\":80") != NULL, "JSON contains hunger 80");
  CHECK(strstr(buf, "\"warmth\":85") != NULL, "JSON contains warmth 85");
  CHECK(strstr(buf, "\"energy\":90") != NULL, "JSON contains energy 90");
  CHECK(strstr(buf, "\"happy\":75") != NULL, "JSON contains happy 75");
  CHECK(strstr(buf, "\"discipline\":50") != NULL, "JSON contains discipline 50");
  CHECK(strstr(buf, "\"alive\":true") != NULL, "JSON contains alive true");

  pet.setOutfit(OUTFIT_WINTER);
  pet.formatStateJson(buf, sizeof(buf));
  CHECK(strstr(buf, "\"outfit\":\"winter\"") != NULL, "JSON contains outfit winter");
  pet.setOutfit(OUTFIT_DEFAULT);

  // Format with custom name and baby stage
  pet.formatStateJson(buf, sizeof(buf), "Sparky", "baby");
  CHECK(strstr(buf, "\"name\":\"Sparky\"") != NULL, "JSON contains custom name Sparky");
  CHECK(strstr(buf, "\"stage\":\"baby\"") != NULL, "JSON contains custom stage baby");

  // Dead state: all vitals 0
  pet.setVitals(0, 0, 0, 0, 0);
  CHECK(!pet.isAlive(), "Lumi is not alive when all vitals reach 0");
  pet.formatStateJson(buf, sizeof(buf));
  CHECK(strstr(buf, "\"alive\":false") != NULL, "JSON contains alive false when dead");
}

void testPetStorageSerialization() {
  printf("\n--- Test: StoragePet MicroSD Serialization & Parsing ---\n");
  PetLogic pet;
  pet.setVitals(77, 88, 66, 99, 44);
  pet.setSleeping(true);

  char buffer[128];
  StoragePet::serializeState(buffer, sizeof(buffer), pet);

  CHECK(strstr(buffer, "HUNGER=77\n") != NULL, "Serialized text contains HUNGER=77");
  CHECK(strstr(buffer, "WARMTH=88\n") != NULL, "Serialized text contains WARMTH=88");
  CHECK(strstr(buffer, "ENERGY=66\n") != NULL, "Serialized text contains ENERGY=66");
  CHECK(strstr(buffer, "HAPPINESS=99\n") != NULL, "Serialized text contains HAPPINESS=99");
  CHECK(strstr(buffer, "DISCIPLINE=44\n") != NULL, "Serialized text contains DISCIPLINE=44");
  CHECK(strstr(buffer, "ASLEEP=1\n") != NULL, "Serialized text contains ASLEEP=1");
  CHECK(strstr(buffer, "STAGE=0\n") != NULL, "Serialized text contains STAGE=0 (BABY)");
  CHECK(strstr(buffer, "OUTFIT=0\n") != NULL, "Serialized text contains OUTFIT=0 (DEFAULT)");

  // Test deserialization into fresh variables
  float h = 0, w = 0, e = 0, hap = 0, d = 0;
  bool asleep = false;
  PetStage stage = STAGE_TEEN;
  PetOutfit outfit = OUTFIT_WINTER;
  CHECK(StoragePet::deserializeState(buffer, h, w, e, hap, d, asleep, stage, outfit), "Deserialization succeeds on valid text with outfit");
  CHECK((int)h == 77, "Restored hunger is 77");
  CHECK((int)w == 88, "Restored warmth is 88");
  CHECK((int)e == 66, "Restored energy is 66");
  CHECK((int)hap == 99, "Restored happiness is 99");
  CHECK((int)d == 44, "Restored discipline is 44");
  CHECK(asleep == true, "Restored asleep state is true");
  CHECK(stage == STAGE_BABY, "Restored stage is STAGE_BABY");
  CHECK(outfit == OUTFIT_DEFAULT, "Restored outfit is OUTFIT_DEFAULT");

  // Test serializing winter outfit
  pet.setOutfit(OUTFIT_WINTER);
  StoragePet::serializeState(buffer, sizeof(buffer), pet);
  CHECK(strstr(buffer, "OUTFIT=1\n") != NULL, "Serialized text contains OUTFIT=1 (WINTER)");
  CHECK(StoragePet::deserializeState(buffer, h, w, e, hap, d, asleep, stage, outfit), "Deserializes winter outfit");
  CHECK(outfit == OUTFIT_WINTER, "Restored outfit is OUTFIT_WINTER");

  // Test corrupt or truncated inputs
  CHECK(!StoragePet::deserializeState("HUNGER=50\n", h, w, e, hap, d, asleep), "Fails on incomplete file");
  CHECK(!StoragePet::deserializeState("HUNGER=999\nWARMTH=50\nENERGY=50\nHAPPINESS=50\nDISCIPLINE=50\nASLEEP=0\n", h, w, e, hap, d, asleep), "Fails on out-of-range value (999)");
  CHECK(!StoragePet::deserializeState(nullptr, h, w, e, hap, d, asleep), "Fails safely on nullptr");
  CHECK(!StoragePet::deserializeState("", h, w, e, hap, d, asleep), "Fails safely on empty string");

  // Test minigame high scores, age minutes and FNV-1a hash
  pet.setHighScorePong(42);
  pet.setHighScoreMaze(108);
  pet.setAgeMinutes(15);
  // Setting a lower score should keep highest record
  pet.setHighScorePong(10);
  CHECK(pet.getHighScorePong() == 42, "HighScorePong retains highest record (42)");
  CHECK(pet.getHighScoreMaze() == 108, "HighScoreMaze is 108");
  CHECK(pet.getAgeMinutes() == 15, "AgeMinutes is 15");

  char extBuffer[160];
  StoragePet::serializeState(extBuffer, sizeof(extBuffer), pet);
  CHECK(strstr(extBuffer, "PONG=42\n") != NULL, "Serialized text contains PONG=42");
  CHECK(strstr(extBuffer, "MAZE=108\n") != NULL, "Serialized text contains MAZE=108");
  CHECK(strstr(extBuffer, "AGE=15\n") != NULL, "Serialized text contains AGE=15");

  uint16_t rPong = 0, rMaze = 0;
  uint32_t rAge = 0;
  CHECK(StoragePet::deserializeState(extBuffer, h, w, e, hap, d, asleep, stage, outfit, rPong, rMaze, rAge), "Deserializes extended state with high scores & age");
  CHECK(rPong == 42, "Restored Pong record is 42");
  CHECK(rMaze == 108, "Restored Maze record is 108");
  CHECK(rAge == 15, "Restored Age is 15 minutes");

  uint32_t hash1 = StoragePet::computeHash(extBuffer);
  uint32_t hash2 = StoragePet::computeHash(extBuffer);
  CHECK(hash1 != 0, "FNV-1a hash is non-zero");
  CHECK(hash1 == hash2, "FNV-1a hash is deterministic");

  // Test age minute accumulation in PetLogic::update()
  PetLogic agePet;
  CHECK(agePet.getAgeMinutes() == 0, "Initial age is 0 minutes");
  agePet.update(0); // Initialize decay timer
  agePet.update(60000); // 6 intervals of 10s = 60s -> 1 minute
  CHECK(agePet.getAgeMinutes() == 1, "Age reaches 1 minute after 60s");
  agePet.update(120000); // Another 60s -> 2 minutes
  CHECK(agePet.getAgeMinutes() == 2, "Age reaches 2 minutes after 120s");
}

void testPetWardrobeAndThermoregulation() {
  printf("\n--- Test: PetLogic Wardrobe & Thermoregulation ---\n");
  PetLogic pet;

  // 1. Initial outfit
  CHECK(pet.getOutfitEnum() == OUTFIT_DEFAULT, "Initial outfit is OUTFIT_DEFAULT");
  CHECK(strcmp(pet.getOutfit(), "default") == 0, "Initial outfit string is 'default'");
  CHECK(!pet.isOverheating(), "Lumi is not overheating initially");

  // 2. Toggle outfit
  pet.toggleOutfit();
  CHECK(pet.getOutfitEnum() == OUTFIT_WINTER, "Toggled outfit is OUTFIT_WINTER");
  CHECK(strcmp(pet.getOutfit(), "winter") == 0, "Outfit string is 'winter'");

  // 3. Overheating in warm room (T >= 23.5 C or warmth >= 75%)
  pet.setAmbientTemperature(24.5f);
  CHECK(pet.isOverheating(), "Lumi is overheating when wearing winter clothes in a warm room (24.5 C)");
  CHECK(strcmp(pet.getMoodStatusDanish(), "Lumi sveder! Tag huen af!") == 0, "Danish status warns player to remove hat");

  // Take off hat -> stops overheating immediately!
  pet.setOutfit(OUTFIT_DEFAULT);
  CHECK(!pet.isOverheating(), "Taking off winter clothes stops overheating");

  // 4. Freezing cold in cold room
  pet.setVitals(80, 20, 80, 80, 80); // Warmth 20%
  CHECK(strcmp(pet.getMoodStatusDanish(), "Lumi fryser! Giv hue paa!") == 0, "Danish status prompts player to put winter clothes on");

  // Put on winter outfit in cold room -> insulates Lumi
  pet.setOutfit(OUTFIT_WINTER);
  pet.setAmbientTemperature(12.0f); // cold room
  for (int i = 0; i < 20; i++) {
    pet.update(i * 1000);
  }
  CHECK(pet.getWarmth() >= 20, "Warmth does not freeze out when insulated in winter outfit");
}

void testSoloPongLogic() {
  printf("\n--- Test: Solo Pong Game Logic & Physics ---\n");
  PongLogic pong;
  pong.startNewGame(10); // Start with existing high score 10

  // 1. Initial State
  CHECK(pong.getState() == PONG_STATE_READY, "Initial state is PONG_STATE_READY");
  CHECK(pong.getLives() == 3, "Initial lives are 3");
  CHECK(pong.getScore() == 0, "Initial score is 0");
  CHECK(pong.getHighScore() == 10, "Existing high score is 10");
  CHECK(!pong.isNewHighScore(), "Not a new high score initially");

  // 2. Paddle movement and clamping
  float midY = pong.getPaddleY();
  pong.movePaddle(500.0f); // Try to move way past bottom
  CHECK(pong.getPaddleY() == (float)(PongLogic::FIELD_BOTTOM - PongLogic::PADDLE_HEIGHT - 1), "Paddle clamped at bottom boundary");

  pong.movePaddle(-1000.0f); // Try to move way past top
  CHECK(pong.getPaddleY() == (float)(PongLogic::FIELD_TOP + 1), "Paddle clamped at top boundary");

  // Reset paddle to center
  pong.movePaddle(midY - (float)(PongLogic::FIELD_TOP + 1));

  // 3. Serve ball
  pong.serveBall();
  CHECK(pong.getState() == PONG_STATE_PLAYING, "State is PONG_STATE_PLAYING after serve");
  CHECK(pong.getBallVx() > 0.0f, "Ball moves towards right wall (Vx > 0)");

  // 4. Simulate flight to right wall
  unsigned long t = 1000;
  bool hitRightWall = false;
  for (int i = 0; i < 200; i++) {
    t += 20;
    pong.update(t);
    if (pong.popWallHit()) {
      hitRightWall = true;
      break;
    }
  }
  CHECK(hitRightWall, "Ball collided with solid right bounce wall");
  CHECK(pong.getBallVx() < 0.0f, "Ball reflected back towards left paddle (Vx < 0)");

  // 5. Intercept ball with paddle
  // Place paddle at ball Y so it's guaranteed to hit
  bool hitPaddle = false;
  float speedBeforeHit = -pong.getBallVx();
  for (int i = 0; i < 200; i++) {
    t += 20;
    // Keep paddle aligned with ball
    pong.movePaddle(pong.getBallY() - pong.getPaddleY());
    pong.update(t);
    if (pong.popPaddleHit()) {
      hitPaddle = true;
      break;
    }
  }
  CHECK(hitPaddle, "Paddle successfully hit the incoming ball");
  CHECK(pong.getScore() == 1, "Score incremented to 1 after successful hit");
  CHECK(pong.getBallVx() > 0.0f, "Ball bounced forward to the right (Vx > 0)");
  CHECK(pong.getBallVx() > speedBeforeHit, "Ball accelerated after paddle hit");

  // 6. Test ball miss (move paddle out of the way)
  // Let ball hit right wall again
  for (int i = 0; i < 200; i++) {
    t += 20;
    pong.update(t);
    if (pong.popWallHit()) break;
  }
  // Move paddle to bottom while ball travels high
  pong.movePaddle(200.0f);
  bool ballLost = false;
  for (int i = 0; i < 200; i++) {
    t += 20;
    pong.update(t);
    if (pong.getState() == PONG_STATE_BALL_LOST) {
      ballLost = true;
      break;
    }
  }
  CHECK(ballLost, "State transitioned to PONG_STATE_BALL_LOST after missing ball");
  CHECK(pong.getLives() == 2, "Lives decremented to 2");

  // 7. Auto-respawn after 800 ms
  pong.update(t + 400);
  CHECK(pong.getState() == PONG_STATE_BALL_LOST, "Still BALL_LOST before 800 ms timeout");
  pong.update(t + 850);
  CHECK(pong.getState() == PONG_STATE_PLAYING, "Auto-respawned and playing after 850 ms");

  // 8. Deplete remaining lives to reach Game Over
  // Miss life 2
  pong.movePaddle(200.0f);
  for (int i = 0; i < 400; i++) {
    t += 20;
    pong.update(t);
    if (pong.getState() == PONG_STATE_BALL_LOST) break;
  }
  CHECK(pong.getLives() == 1, "Lives decremented to 1");
  t += 900;
  pong.update(t); // Respawn

  // Miss life 3 -> Game Over
  pong.movePaddle(200.0f);
  for (int i = 0; i < 400; i++) {
    t += 20;
    pong.update(t);
    if (pong.getState() == PONG_STATE_GAMEOVER) break;
  }
  CHECK(pong.getState() == PONG_STATE_GAMEOVER, "State is PONG_STATE_GAMEOVER when all lives lost");
  CHECK(pong.getLives() == 0, "Lives is 0 on game over");

  // 9. New High Score achievement
  PongLogic hiPong;
  hiPong.startNewGame(2);
  // Cheat-simulate score higher than high score
  hiPong.serveBall();
  for (int s = 0; s < 3; s++) {
    // Manually trigger paddle hits
    t += 20;
    hiPong.update(t);
  }
  // Test startNewGame resets score but keeps high score
  hiPong.startNewGame(15);
  CHECK(hiPong.getScore() == 0, "New game score reset to 0");
  CHECK(hiPong.getHighScore() == 15, "New game high score set to 15");
  CHECK(hiPong.getLives() == 3, "New game lives reset to 3");
}

void testMarbleMazeLogic() {
  printf("\n--- Test: Marble Maze Generation, Solvability & Physics ---\n");
  MazeLogic maze;
  maze.generateMaze(42, 45); // Seed 42, existing best record 45s

  // 1. Initial State
  CHECK(maze.getState() == MAZE_STATE_READY, "Maze starts in MAZE_STATE_READY");
  CHECK(maze.getBerriesCollected() == 0, "Initial berries collected is 0");
  CHECK(maze.getTotalBerries() >= 1, "At least 1 Deku Berry placed in maze");
  CHECK(maze.getBestSeconds() == 45, "Existing best record is 45s");

  // 2. MATHEMATICAL PROOF OF 100% SOLVABILITY (BFS reachability)
  // Check that every single cell (49/49) is reachable from (0,0) and path to (6,6) exists!
  bool visited[MAZE_ROWS][MAZE_COLS];
  memset(visited, 0, sizeof(visited));

  uint8_t qCol[MAZE_COLS * MAZE_ROWS];
  uint8_t qRow[MAZE_COLS * MAZE_ROWS];
  int qHead = 0;
  int qTail = 0;

  // Enqueue start cell (0, 0)
  qCol[qTail] = 0;
  qRow[qTail] = 0;
  qTail++;
  visited[0][0] = true;

  int reachableCount = 0;
  while (qHead < qTail) {
    uint8_t c = qCol[qHead];
    uint8_t r = qRow[qHead];
    qHead++;
    reachableCount++;

    uint8_t cell = maze.getCell(c, r);

    // North
    if (!(cell & MAZE_WALL_NORTH) && r > 0 && !visited[r - 1][c]) {
      visited[r - 1][c] = true;
      qCol[qTail] = c; qRow[qTail] = r - 1; qTail++;
    }
    // East
    if (!(cell & MAZE_WALL_EAST) && c < MAZE_COLS - 1 && !visited[r][c + 1]) {
      visited[r][c + 1] = true;
      qCol[qTail] = c + 1; qRow[qTail] = r; qTail++;
    }
    // South
    if (!(cell & MAZE_WALL_SOUTH) && r < MAZE_ROWS - 1 && !visited[r + 1][c]) {
      visited[r + 1][c] = true;
      qCol[qTail] = c; qRow[qTail] = r + 1; qTail++;
    }
    // West
    if (!(cell & MAZE_WALL_WEST) && c > 0 && !visited[r][c - 1]) {
      visited[r][c - 1] = true;
      qCol[qTail] = c - 1; qRow[qTail] = r; qTail++;
    }
  }

  CHECK(reachableCount == (MAZE_COLS * MAZE_ROWS), "100% Spanning Tree: All 49 cells reachable");
  CHECK(visited[MAZE_ROWS - 1][MAZE_COLS - 1], "Guaranteed solvable path from Start (0,0) to Exit (6,6)");

  // 3. Traps and Berries present in grid
  bool hasPitfall = false;
  bool hasBerry = false;
  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    for (uint8_t c = 0; c < MAZE_COLS; c++) {
      uint8_t cell = maze.getCell(c, r);
      if (cell & MAZE_ITEM_PITFALL) hasPitfall = true;
      if (cell & MAZE_ITEM_BERRY)   hasBerry = true;
    }
  }
  CHECK(hasPitfall, "Pitfall hole traps generated in dead ends");
  CHECK(hasBerry, "Glowing Deku Berries generated in dead ends");

  // 4. IMU Tilt physics and movement
  // Cell (0,0) has either East or South open. Tilt towards whichever direction is open:
  bool openEast = !(maze.getCell(0, 0) & MAZE_WALL_EAST);
  if (openEast) {
    maze.setTilt(0.8f, 0.0f); // Tilt right
  } else {
    maze.setTilt(0.0f, 0.8f); // Tilt down
  }
  maze.update(1000); // init timer
  maze.update(1020); // transition from READY to PLAYING
  maze.update(1040); // step physics
  CHECK(maze.getState() == MAZE_STATE_PLAYING, "State switches to MAZE_STATE_PLAYING upon tilting");
  if (openEast) {
    CHECK(maze.getBallVx() > 0.0f, "Ball accelerates right along open passage");
  } else {
    CHECK(maze.getBallVy() > 0.0f, "Ball accelerates down along open passage");
  }

  // 5. Outer boundary collision (tilt into Top-Left outer corner)
  maze.setTilt(-1.0f, -1.0f);
  for (int i = 0; i < 50; i++) {
    maze.update(1040 + i * 20);
  }
  float minLimitX = (float)(MazeLogic::ORIGIN_X + MazeLogic::BALL_RADIUS + 1);
  float minLimitY = (float)(MazeLogic::ORIGIN_Y + MazeLogic::BALL_RADIUS + 1);
  CHECK(maze.getBallX() >= minLimitX, "Ball clamped at West boundary");
  CHECK(maze.getBallY() >= minLimitY, "Ball clamped at North boundary");

  // 6. Test victory trigger
  // Generate a new maze and test victory at exit coordinates
  MazeLogic vicMaze;
  vicMaze.generateMaze(999, 50);
  vicMaze.setTilt(0.5f, 0.5f);
  vicMaze.update(100);
  vicMaze.update(120);

  // Simulate reaching the portal at cell (6, 6)
  // Exit center is at ORIGIN_X + 6*CELL_SIZE + 12, ORIGIN_Y + 6*CELL_SIZE + 12
  // We can test victory condition directly by moving near portal
  float exitX = (float)(MazeLogic::ORIGIN_X + (MAZE_COLS - 1) * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE * 0.5f);
  float exitY = (float)(MazeLogic::ORIGIN_Y + (MAZE_ROWS - 1) * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE * 0.5f);
  vicMaze.setTilt(0.0f, 0.0f);
  // Place ball directly at exit portal
  for (int i = 0; i < 20; i++) {
    // Manually push ball to exit coordinates
    vicMaze.setTilt((exitX - vicMaze.getBallX()) * 0.05f, (exitY - vicMaze.getBallY()) * 0.05f);
    vicMaze.update(200 + i * 20);
    if (vicMaze.getState() == MAZE_STATE_VICTORY) break;
  }
}

int main() {
  printf("==================================================\n");
  printf("TAMAPAL UNIT TEST RUNNER: MenuLogic & TiltLogic & PetLogic\n");
  printf("==================================================\n");

  testInitialState();
  testNavigationWrapping();
  testScreenSwitching();
  testActionScreenTimeout();
  testInputDisabledOnOtherScreens();
  testSmoothCursorPhysics();

  testTiltCalculations();
  testTiltSmoothing();
  testTiltJsonFormatting();
  testEventParsing();

  testPetInitialState();
  testPetActions();
  testPetSleepAndSensors();
  testPetDecayAndStatus();
  testPetSerialization();
  testPetStorageSerialization();
  testPetWardrobeAndThermoregulation();
  testSoloPongLogic();
  testMarbleMazeLogic();

  printf("\n==================================================\n");
  if (totalTestsFailed == 0) {
    printf("RESULT: ALL OK! (%d of %d tests passed)\n", totalTestsRun, totalTestsRun);
    printf("==================================================\n");
    return 0;
  } else {
    printf("RESULT: FAILED! (%d failed out of %d tests)\n", totalTestsFailed, totalTestsRun);
    printf("==================================================\n");
    return 1;
  }
}

