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

  // Move right: 0 -> 1 -> 2 -> 3 -> 4 -> 5 -> 0 (wrap)
  menu.onButtonNext();
  CHECK(menu.getSelectedIcon() == ICON_PLAY, "Next moves from 0 to 1 (PLAY)");

  menu.onButtonNext(); // 2 (MEDICIN)
  CHECK(menu.getSelectedIcon() == ICON_MEDICIN, "Next moves from 1 to 2 (MEDICIN)");
  menu.onButtonNext(); // 3 (SCOLD)
  menu.onButtonNext(); // 4 (STATS)
  menu.onButtonNext(); // 5 (CONNECT)
  CHECK(menu.getSelectedIcon() == ICON_CONNECT, "Reached last icon (CONNECT)");

  menu.onButtonNext(); // Wrap to 0
  CHECK(menu.getSelectedIcon() == ICON_FOOD, "Next from 5 wraps to 0 (FOOD)");

  // Move left: 0 -> 5 (wrap)
  menu.onButtonPrev();
  CHECK(menu.getSelectedIcon() == ICON_CONNECT, "Prev from 0 wraps to 5 (CONNECT)");

  menu.onButtonPrev();
  CHECK(menu.getSelectedIcon() == ICON_STATS, "Prev from 5 moves to 4 (STATS)");
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
  menu.onButtonNext(); // 1 (PLAY)
  menu.onButtonNext(); // 2 (MEDICIN)
  menu.onButtonNext(); // 3 (SCOLD)
  menu.onButtonNext(); // 4 (STATS)
  menu.onButtonOk(1000);
  CHECK(menu.getActiveScreen() == SCREEN_STATS, "OK on STATS icon opens SCREEN_STATS");

  // 3. TOUCH3 (Back) returns from STATS to HOME
  menu.onButtonBack();
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "TOUCH3 returns from STATS to HOME");

  // 4. Select CONNECT -> SCREEN_CHOOSE_PLAYER -> selectPlayer(1) -> CONTROLLER
  menu.onButtonNext(); // 5 (CONNECT)
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
  // Setting a lower score should keep highest record in Pong
  pet.setHighScorePong(10);
  CHECK(pet.getHighScorePong() == 42, "HighScorePong retains highest record (42)");
  CHECK(pet.getHighScoreMaze() == 108, "HighScoreMaze is 108");
  CHECK(pet.getAgeMinutes() == 15, "AgeMinutes is 15");

  // In Maze, lower completion time is a better record!
  pet.setHighScoreMaze(45);
  CHECK(pet.getHighScoreMaze() == 45, "HighScoreMaze updated to 45s (better than 108s)");
  pet.setHighScoreMaze(30);
  CHECK(pet.getHighScoreMaze() == 30, "HighScoreMaze updated to 30s (faster time wins!)");
  pet.setHighScoreMaze(50);
  CHECK(pet.getHighScoreMaze() == 30, "HighScoreMaze ignored slower 50s time and stayed 30s");

  char extBuffer[160];
  StoragePet::serializeState(extBuffer, sizeof(extBuffer), pet);
  CHECK(strstr(extBuffer, "PONG=42\n") != NULL, "Serialized text contains PONG=42");
  CHECK(strstr(extBuffer, "MAZE=30\n") != NULL, "Serialized text contains MAZE=30");
  CHECK(strstr(extBuffer, "AGE=15\n") != NULL, "Serialized text contains AGE=15");

  uint16_t rPong = 0, rMaze = 0;
  uint32_t rAge = 0;
  CHECK(StoragePet::deserializeState(extBuffer, h, w, e, hap, d, asleep, stage, outfit, rPong, rMaze, rAge), "Deserializes extended state with high scores & age");
  CHECK(rPong == 42, "Restored Pong record is 42");
  CHECK(rMaze == 30, "Restored Maze record is 30");
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
  CHECK(pong.getPaddleY() == (float)(PongLogic::FIELD_BOTTOM - PongLogic::PADDLE_HEIGHT - 2), "Paddle clamped at bottom boundary");

  pong.movePaddle(-1000.0f); // Try to move way past top
  CHECK(pong.getPaddleY() == (float)(PongLogic::FIELD_TOP + 2), "Paddle clamped at top boundary");

  // Reset paddle to center
  pong.movePaddle(midY - (float)(PongLogic::FIELD_TOP + 2));

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

  // 7. Auto-respawn after 2000 ms
  pong.update(t + 500);
  CHECK(pong.getState() == PONG_STATE_BALL_LOST, "Still BALL_LOST before 2000 ms timeout");
  pong.update(t + 2050);
  CHECK(pong.getState() == PONG_STATE_PLAYING, "Auto-respawned and playing after 2050 ms");

  // 8. Deplete remaining lives to reach Game Over
  // Miss life 2
  pong.movePaddle(200.0f);
  for (int i = 0; i < 400; i++) {
    t += 20;
    pong.update(t);
    if (pong.getState() == PONG_STATE_BALL_LOST) break;
  }
  CHECK(pong.getLives() == 1, "Lives decremented to 1");
  // Test early manual serve during BALL_LOST
  pong.serveBall();
  CHECK(pong.getState() == PONG_STATE_PLAYING, "Manual serve immediately resumes playing from BALL_LOST");

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
  hiPong.startNewGame(1);
  CHECK(!hiPong.isNewHighScore(), "Pong: isNewHighScore false at start");
  hiPong.serveBall();
  t = 10000;
  hiPong.update(t);
  for (int step = 0; step < 600 && hiPong.getScore() < 2; step++) {
    t += 20;
    hiPong.setPaddleY(hiPong.getBallY() - PongLogic::PADDLE_HEIGHT * 0.5f);
    hiPong.update(t);
  }
  CHECK(hiPong.getScore() >= 2, "Pong: score reaches 2 to beat initial high score 1");
  CHECK(hiPong.isNewHighScore(), "Pong: isNewHighScore becomes true after beating high score");
  CHECK(hiPong.getHighScore() == hiPong.getScore(), "Pong: highScore updated dynamically");
  // Test startNewGame resets score but keeps high score
  hiPong.startNewGame(15);
  CHECK(hiPong.getScore() == 0, "New game score reset to 0");
  CHECK(hiPong.getHighScore() == 15, "New game high score set to 15");
  CHECK(hiPong.getLives() == 3, "New game lives reset to 3");

  // 10. Test swept collision prevents paddle tunneling on large timesteps
  PongLogic sweptPong;
  sweptPong.startNewGame();
  sweptPong.serveBall();
  unsigned long st = 1000;
  sweptPong.update(st);
  for (int i = 0; i < 200; i++) {
    st += 20;
    sweptPong.update(st);
    if (sweptPong.popWallHit()) break;
  }
  // Ball is returning towards paddle. Let it get near x=40
  for (int i = 0; i < 200 && sweptPong.getBallX() > 42.0f; i++) {
    st += 10;
    sweptPong.movePaddle(sweptPong.getBallY() - sweptPong.getPaddleY());
    sweptPong.update(st);
  }
  // Step with large dt (60ms) where ball jumps past paddle without swept collision
  st += 60;
  sweptPong.movePaddle(sweptPong.getBallY() - sweptPong.getPaddleY());
  sweptPong.update(st);
  CHECK(sweptPong.getScore() == 1, "Swept collision successfully hits paddle on large timestep (no tunneling)");
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
  vicMaze.setTilt(0.5f, 0.0f);
  vicMaze.update(100);
  vicMaze.update(120);

  // BFS pathfinding from (0,0) to exit portal (MAZE_COLS - 1, MAZE_ROWS - 1)
  int vCol[MAZE_ROWS][MAZE_COLS];
  int vRow[MAZE_ROWS][MAZE_COLS];
  memset(vCol, -1, sizeof(vCol));
  memset(vRow, -1, sizeof(vRow));

  uint8_t vqC[49], vqR[49];
  int vqH = 0, vqT = 0;
  vqC[vqT] = 0; vqR[vqT] = 0; vqT++;
  vCol[0][0] = 0; vRow[0][0] = 0;

  int targetExitC = MAZE_COLS - 1, targetExitR = MAZE_ROWS - 1;
  while (vqH < vqT) {
    uint8_t c = vqC[vqH];
    uint8_t r = vqR[vqH];
    vqH++;
    if (c == targetExitC && r == targetExitR) break;
    uint8_t cell = vicMaze.getCell(c, r);
    if (!(cell & MAZE_WALL_NORTH) && r > 0 && vCol[r - 1][c] == -1) {
      vCol[r - 1][c] = c; vRow[r - 1][c] = r; vqC[vqT] = c; vqR[vqT] = r - 1; vqT++;
    }
    if (!(cell & MAZE_WALL_EAST) && c < MAZE_COLS - 1 && vCol[r][c + 1] == -1) {
      vCol[r][c + 1] = c; vRow[r][c + 1] = r; vqC[vqT] = c + 1; vqR[vqT] = r; vqT++;
    }
    if (!(cell & MAZE_WALL_SOUTH) && r < MAZE_ROWS - 1 && vCol[r + 1][c] == -1) {
      vCol[r + 1][c] = c; vRow[r + 1][c] = r; vqC[vqT] = c; vqR[vqT] = r + 1; vqT++;
    }
    if (!(cell & MAZE_WALL_WEST) && c > 0 && vCol[r][c - 1] == -1) {
      vCol[r][c - 1] = c; vRow[r][c - 1] = r; vqC[vqT] = c - 1; vqR[vqT] = r; vqT++;
    }
  }

  uint8_t vpathC[49], vpathR[49];
  int vpathLen = 0;
  int vcurrC = targetExitC, vcurrR = targetExitR;
  while (!(vcurrC == 0 && vcurrR == 0) && vpathLen < 49 && vcurrC >= 0 && vcurrR >= 0) {
    vpathC[vpathLen] = vcurrC;
    vpathR[vpathLen] = vcurrR;
    vpathLen++;
    int prevC = vCol[vcurrR][vcurrC];
    int prevR = vRow[vcurrR][vcurrC];
    vcurrC = prevC;
    vcurrR = prevR;
  }
  for (int i = 0; i < vpathLen / 2; i++) {
    uint8_t tc = vpathC[i]; vpathC[i] = vpathC[vpathLen - 1 - i]; vpathC[vpathLen - 1 - i] = tc;
    uint8_t tr = vpathR[i]; vpathR[i] = vpathR[vpathLen - 1 - i]; vpathR[vpathLen - 1 - i] = tr;
  }

  unsigned long vtSim = 120;
  for (int p = 0; p < vpathLen && vicMaze.getState() != MAZE_STATE_VICTORY; p++) {
    float targetX = (float)(MazeLogic::ORIGIN_X + vpathC[p] * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE * 0.5f);
    float targetY = (float)(MazeLogic::ORIGIN_Y + vpathR[p] * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE * 0.5f);
    for (int step = 0; step < 50; step++) {
      float dx = targetX - vicMaze.getBallX();
      float dy = targetY - vicMaze.getBallY();
      float tx = (dx > 2.0f) ? 0.9f : ((dx < -2.0f) ? -0.9f : 0.0f);
      float ty = (dy > 2.0f) ? 0.9f : ((dy < -2.0f) ? -0.9f : 0.0f);
      vicMaze.setTilt(tx, ty);
      vtSim += 20;
      vicMaze.update(vtSim);
      if (vicMaze.getState() == MAZE_STATE_VICTORY) break;
      if (fabs(dx) <= 5.0f && fabs(dy) <= 5.0f) break;
    }
  }

  CHECK(vicMaze.getState() == MAZE_STATE_VICTORY, "Maze: reaches MAZE_STATE_VICTORY at exit portal");
  CHECK(vicMaze.isNewBestRecord(), "Maze: sets newBestRecord when beating previous bestSeconds");
  CHECK(vicMaze.getBestSeconds() >= 1 && vicMaze.getBestSeconds() < 50, "Maze: updates bestSeconds (>= 1s) on victory");

  // 7. Test pause functionality and timer freezing
  MazeLogic pauseMaze;
  pauseMaze.generateMaze(123, 60);
  pauseMaze.setTilt(0.5f, 0.0f);
  pauseMaze.update(1000);
  pauseMaze.update(1020); // Playing, startTimeMs = 1020
  pauseMaze.update(3020); // 2 seconds elapsed
  CHECK(pauseMaze.getElapsedSeconds(3020) == 2, "2 seconds elapsed before pause");
  pauseMaze.setPaused(true, 3020);
  CHECK(pauseMaze.getState() == MAZE_STATE_PAUSED, "Maze state is MAZE_STATE_PAUSED");
  pauseMaze.update(7020); // 4 seconds passed in pause, physics doesn't step
  CHECK(pauseMaze.getElapsedSeconds(7020) == 2, "Elapsed seconds frozen during pause");
  pauseMaze.setPaused(false, 8020); // Resumed after 5 seconds
  CHECK(pauseMaze.getState() == MAZE_STATE_PLAYING, "Maze state resumed to PLAYING");
  CHECK(pauseMaze.getElapsedSeconds(8020) == 2, "Elapsed seconds remained 2s right after resume");
  pauseMaze.update(9020); // 1s more of play
  CHECK(pauseMaze.getElapsedSeconds(9020) == 3, "Elapsed seconds advanced to 3s after active play resumed");

  // 8. Test timer in READY state, pitfall penalty (+5s) and berry bonus (-2s)
  MazeLogic readyMaze;
  readyMaze.generateMaze(777, 40);
  readyMaze.update(1000);
  readyMaze.update(5000);
  CHECK(readyMaze.getState() == MAZE_STATE_READY, "Maze remains in READY before tilting");
  CHECK(readyMaze.getElapsedSeconds(5000) == 0, "Timer shows 0s while waiting in READY");

  readyMaze.setTilt(0.5f, 0.0f);
  readyMaze.update(6000); // Transitions to PLAYING, startTimeMs = 6000
  readyMaze.update(10000); // 4 seconds elapsed
  CHECK(readyMaze.getElapsedSeconds(10000) == 4, "4 seconds elapsed in active play");

  // BFS pathfinding to find an reachable pitfall cell along open passages
  int pCol[MAZE_ROWS][MAZE_COLS];
  int pRow[MAZE_ROWS][MAZE_COLS];
  memset(pCol, -1, sizeof(pCol));
  memset(pRow, -1, sizeof(pRow));

  uint8_t qC[49], qR[49];
  int qH = 0, qT = 0;
  qC[qT] = 0; qR[qT] = 0; qT++;
  pCol[0][0] = 0; pRow[0][0] = 0;

  int targetPitC = -1, targetPitR = -1;
  while (qH < qT && targetPitC == -1) {
    uint8_t c = qC[qH];
    uint8_t r = qR[qH];
    qH++;
    if (readyMaze.getCell(c, r) & MAZE_ITEM_PITFALL) {
      targetPitC = c; targetPitR = r;
      break;
    }
    uint8_t cell = readyMaze.getCell(c, r);
    if (!(cell & MAZE_WALL_NORTH) && r > 0 && pCol[r - 1][c] == -1) {
      pCol[r - 1][c] = c; pRow[r - 1][c] = r; qC[qT] = c; qR[qT] = r - 1; qT++;
    }
    if (!(cell & MAZE_WALL_EAST) && c < MAZE_COLS - 1 && pCol[r][c + 1] == -1) {
      pCol[r][c + 1] = c; pRow[r][c + 1] = r; qC[qT] = c + 1; qR[qT] = r; qT++;
    }
    if (!(cell & MAZE_WALL_SOUTH) && r < MAZE_ROWS - 1 && pCol[r + 1][c] == -1) {
      pCol[r + 1][c] = c; pRow[r + 1][c] = r; qC[qT] = c; qR[qT] = r + 1; qT++;
    }
    if (!(cell & MAZE_WALL_WEST) && c > 0 && pCol[r][c - 1] == -1) {
      pCol[r][c - 1] = c; pRow[r][c - 1] = r; qC[qT] = c - 1; qR[qT] = r; qT++;
    }
  }

  uint8_t pathC[49], pathR[49];
  int pathLen = 0;
  int currC = targetPitC, currR = targetPitR;
  while (!(currC == 0 && currR == 0) && pathLen < 49 && currC >= 0 && currR >= 0) {
    pathC[pathLen] = currC;
    pathR[pathLen] = currR;
    pathLen++;
    int prevC = pCol[currR][currC];
    int prevR = pRow[currR][currC];
    currC = prevC;
    currR = prevR;
  }
  for (int i = 0; i < pathLen / 2; i++) {
    uint8_t tc = pathC[i]; pathC[i] = pathC[pathLen - 1 - i]; pathC[pathLen - 1 - i] = tc;
    uint8_t tr = pathR[i]; pathR[i] = pathR[pathLen - 1 - i]; pathR[pathLen - 1 - i] = tr;
  }

  unsigned long tSim = 10000;
  bool hitPit = false;
  for (int p = 0; p < pathLen && !hitPit; p++) {
    float targetX = (float)(MazeLogic::ORIGIN_X + pathC[p] * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE * 0.5f);
    float targetY = (float)(MazeLogic::ORIGIN_Y + pathR[p] * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE * 0.5f);
    for (int step = 0; step < 50; step++) {
      float dx = targetX - readyMaze.getBallX();
      float dy = targetY - readyMaze.getBallY();
      float tx = (dx > 2.0f) ? 0.9f : ((dx < -2.0f) ? -0.9f : 0.0f);
      float ty = (dy > 2.0f) ? 0.9f : ((dy < -2.0f) ? -0.9f : 0.0f);
      readyMaze.setTilt(tx, ty);
      tSim += 20;
      readyMaze.update(tSim);
      if (readyMaze.popPitfallFall()) {
        hitPit = true;
        break;
      }
      if (fabs(dx) <= 5.0f && fabs(dy) <= 5.0f) {
        break;
      }
    }
  }

  CHECK(hitPit, "Ball triggered pitfall collision");
  CHECK(readyMaze.getElapsedSeconds(tSim) >= 9, "Pitfall adds +5s penalty to elapsed timer");

  // Test berry pickup and bonus (-2s)
  MazeLogic berryMaze;
  berryMaze.generateMaze(101, 60);
  berryMaze.setTilt(0.5f, 0.0f);
  berryMaze.update(1000);
  berryMaze.update(1020); // Playing state

  // Find reachable berry avoiding pitfalls and exit portal via BFS
  int bCol[MAZE_ROWS][MAZE_COLS];
  int bRow[MAZE_ROWS][MAZE_COLS];
  memset(bCol, -1, sizeof(bCol));
  memset(bRow, -1, sizeof(bRow));

  uint8_t bqC[49], bqR[49];
  int bqH = 0, bqT = 0;
  bqC[bqT] = 0; bqR[bqT] = 0; bqT++;
  bCol[0][0] = 0; bRow[0][0] = 0;

  int targetBerryC = -1, targetBerryR = -1;
  while (bqH < bqT && targetBerryC == -1) {
    uint8_t c = bqC[bqH];
    uint8_t r = bqR[bqH];
    bqH++;
    if (berryMaze.getCell(c, r) & MAZE_ITEM_BERRY) {
      targetBerryC = c; targetBerryR = r;
      break;
    }
    uint8_t cell = berryMaze.getCell(c, r);
    if (!(cell & MAZE_WALL_NORTH) && r > 0 && bCol[r - 1][c] == -1 && !(berryMaze.getCell(c, r - 1) & MAZE_ITEM_PITFALL) && !(c == MAZE_COLS - 1 && r - 1 == MAZE_ROWS - 1)) {
      bCol[r - 1][c] = c; bRow[r - 1][c] = r; bqC[bqT] = c; bqR[bqT] = r - 1; bqT++;
    }
    if (!(cell & MAZE_WALL_EAST) && c < MAZE_COLS - 1 && bCol[r][c + 1] == -1 && !(berryMaze.getCell(c + 1, r) & MAZE_ITEM_PITFALL) && !(c + 1 == MAZE_COLS - 1 && r == MAZE_ROWS - 1)) {
      bCol[r][c + 1] = c; bRow[r][c + 1] = r; bqC[bqT] = c + 1; bqR[bqT] = r; bqT++;
    }
    if (!(cell & MAZE_WALL_SOUTH) && r < MAZE_ROWS - 1 && bCol[r + 1][c] == -1 && !(berryMaze.getCell(c, r + 1) & MAZE_ITEM_PITFALL) && !(c == MAZE_COLS - 1 && r + 1 == MAZE_ROWS - 1)) {
      bCol[r + 1][c] = c; bRow[r + 1][c] = r; bqC[bqT] = c; bqR[bqT] = r + 1; bqT++;
    }
    if (!(cell & MAZE_WALL_WEST) && c > 0 && bCol[r][c - 1] == -1 && !(berryMaze.getCell(c - 1, r) & MAZE_ITEM_PITFALL) && !(c - 1 == MAZE_COLS - 1 && r == MAZE_ROWS - 1)) {
      bCol[r][c - 1] = c; bRow[r][c - 1] = r; bqC[bqT] = c - 1; bqR[bqT] = r; bqT++;
    }
  }

  uint8_t bpathC[49], bpathR[49];
  int bpathLen = 0;
  int bcurrC = targetBerryC, bcurrR = targetBerryR;
  while (!(bcurrC == 0 && bcurrR == 0) && bpathLen < 49 && bcurrC >= 0 && bcurrR >= 0) {
    bpathC[bpathLen] = bcurrC;
    bpathR[bpathLen] = bcurrR;
    bpathLen++;
    int prevC = bCol[bcurrR][bcurrC];
    int prevR = bRow[bcurrR][bcurrC];
    bcurrC = prevC;
    bcurrR = prevR;
  }
  for (int i = 0; i < bpathLen / 2; i++) {
    uint8_t tc = bpathC[i]; bpathC[i] = bpathC[bpathLen - 1 - i]; bpathC[bpathLen - 1 - i] = tc;
    uint8_t tr = bpathR[i]; bpathR[i] = bpathR[bpathLen - 1 - i]; bpathR[bpathLen - 1 - i] = tr;
  }

  // Fast forward time to 10 seconds before collecting berry
  unsigned long btSim = 11020; // 10.0 seconds of gameplay
  berryMaze.update(btSim);

  for (int p = 0; p < bpathLen && berryMaze.getBerriesCollected() == 0; p++) {
    float targetX = (float)(MazeLogic::ORIGIN_X + bpathC[p] * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE * 0.5f);
    float targetY = (float)(MazeLogic::ORIGIN_Y + bpathR[p] * MazeLogic::CELL_SIZE + MazeLogic::CELL_SIZE * 0.5f);
    for (int step = 0; step < 50; step++) {
      float dx = targetX - berryMaze.getBallX();
      float dy = targetY - berryMaze.getBallY();
      float tx = (dx > 2.0f) ? 0.9f : ((dx < -2.0f) ? -0.9f : 0.0f);
      float ty = (dy > 2.0f) ? 0.9f : ((dy < -2.0f) ? -0.9f : 0.0f);
      berryMaze.setTilt(tx, ty);
      btSim += 20;
      berryMaze.update(btSim);
      if (berryMaze.getBerriesCollected() > 0) break;
      if (fabs(dx) <= 5.0f && fabs(dy) <= 5.0f) break;
    }
  }

  uint16_t rawElapsed = (uint16_t)((btSim - 1020) / 1000);
  CHECK(berryMaze.getBerriesCollected() >= 1, "Maze: collects berry along path");
  CHECK((berryMaze.getCell(targetBerryC, targetBerryR) & MAZE_ITEM_BERRY) == 0, "Maze: berry bit cleared from cell after pickup");
  CHECK(berryMaze.getElapsedSeconds() == rawElapsed - 2, "Maze: berry bonus reduces elapsed time by 2s");
}

void testPetDeathAndRebirth() {
  printf("\n--- Test: Pet Lifecycle, Death Causes, Memorial Chronicle & Rebirth ---\n");
  PetLogic pet;

  // 1. Initial State
  CHECK(pet.isAlive(), "Pet is initially alive");
  CHECK(pet.getGeneration() == 1, "Initial generation is 1");
  CHECK(pet.getDeathCause() == DEATH_NONE, "Initial death cause is DEATH_NONE");

  // 2. Manual death triggering & actions disabled
  pet.triggerDeath(DEATH_HUNGER);
  CHECK(!pet.isAlive(), "Pet is dead after triggerDeath");
  CHECK(pet.getDeathCause() == DEATH_HUNGER, "Death cause is DEATH_HUNGER");
  CHECK(strcmp(pet.getDeathCauseName(), "Doede af sult") == 0, "Danish cause is 'Doede af sult'");
  CHECK(strcmp(pet.getDeathEpitaph(), "Maven rumlede til sidst...") == 0, "Danish epitaph is hungry rumble");

  // Actions must fail when dead
  CHECK(!pet.feed(), "Cannot feed a deceased pet");
  CHECK(!pet.play(), "Cannot play with a deceased pet");
  CHECK(!pet.scold(), "Cannot scold a deceased pet");

  // 3. Test all other death causes and epitaphs
  pet.triggerDeath(DEATH_COLD);
  CHECK(strcmp(pet.getDeathCauseName(), "Froes ihjel") == 0, "Danish cause is 'Froes ihjel'");
  CHECK(strcmp(pet.getDeathEpitaph(), "Vinteren var for haard...") == 0, "Epitaph for freezing death");

  pet.triggerDeath(DEATH_HEAT);
  CHECK(strcmp(pet.getDeathCauseName(), "Doede af hedeslag") == 0, "Danish cause is 'Doede af hedeslag'");
  CHECK(strcmp(pet.getDeathEpitaph(), "Smeltede i pejsens varme!") == 0, "Epitaph for heatstroke");

  pet.triggerDeath(DEATH_BOREDOM);
  CHECK(strcmp(pet.getDeathCauseName(), "Doede af ensomhed") == 0, "Danish cause is 'Doede af ensomhed'");
  CHECK(strcmp(pet.getDeathEpitaph(), "Hjertet visnede af savn.") == 0, "Epitaph for boredom");

  pet.triggerDeath(DEATH_EXHAUSTION);
  CHECK(strcmp(pet.getDeathCauseName(), "Doede af udmattelse") == 0, "Danish cause is 'Doede af udmattelse'");
  CHECK(strcmp(pet.getDeathEpitaph(), "Fik aldrig sovet ud...") == 0, "Epitaph for exhaustion");

  pet.triggerDeath(DEATH_OLD_AGE);
  CHECK(strcmp(pet.getDeathCauseName(), "Doede af alderdom") == 0, "Danish cause is 'Doede af alderdom'");
  CHECK(strcmp(pet.getDeathEpitaph(), "Levede et lykkeligt liv!") == 0, "Epitaph for happy old age");

  // 4. Test Rebirth
  pet.setHighScorePong(25);
  pet.setHighScoreMaze(42);
  pet.rebirth();
  CHECK(pet.isAlive(), "Pet is alive after rebirth");
  CHECK(pet.getGeneration() == 2, "Generation incremented to 2");
  CHECK(pet.getDeathCause() == DEATH_NONE, "Death cause reset to DEATH_NONE");
  CHECK(pet.getStageEnum() == STAGE_BABY, "Reborn as cute baby");
  CHECK(pet.getHighScorePong() == 25, "Pong high score preserved across generations");
  CHECK(pet.getHighScoreMaze() == 42, "Maze high score preserved across generations");
  CHECK(pet.getOverallHealth() > 70, "Health restored on rebirth");

  // Second rebirth
  pet.triggerDeath(DEATH_COLD);
  pet.rebirth();
  CHECK(pet.getGeneration() == 3, "Generation incremented to 3");

  // 5. Test automatic death triggers via sustained decay
  PetLogic dyingPet;
  dyingPet.setVitals(0, 50, 50, 50, 50); // 0% hunger
  // Simulate sustained starvation (12 intervals * 10000ms = 120s)
  dyingPet.update(10000); // init decay
  for (int i = 1; i <= 15; i++) {
    dyingPet.update(10000 + i * 10000);
  }
  CHECK(!dyingPet.isAlive(), "Sustained starvation automatically triggers death");
  CHECK(dyingPet.getDeathCause() == DEATH_HUNGER, "Death cause is hunger");

  // 6. Test Serialization of Generation, Alive, and DeathCause
  char buf[256];
  StoragePet::serializeState(buf, sizeof(buf), dyingPet);
  CHECK(strstr(buf, "GEN=1") != NULL, "Serialized text contains GEN=1");
  CHECK(strstr(buf, "ALIVE=0") != NULL, "Serialized text contains ALIVE=0");
  CHECK(strstr(buf, "DEATH=1") != NULL, "Serialized text contains DEATH=1 (HUNGER)");

  float h, w, e, hap, d;
  bool asleep;
  PetStage stage;
  PetOutfit outfit;
  uint16_t pong, maze, gen;
  uint32_t age;
  bool alive;
  DeathCause dc;
  bool ok = StoragePet::deserializeState(buf, h, w, e, hap, d, asleep, stage, outfit, pong, maze, age, gen, alive, dc);
  CHECK(ok, "Deserializes full state with generation and death status");
  CHECK(gen == 1, "Deserialized generation is 1");
  CHECK(!alive, "Deserialized alive is false");
  CHECK(dc == DEATH_HUNGER, "Deserialized death cause is DEATH_HUNGER");

  // 7. Test Memorial Chronicle (grave.txt) record string formatting
  char graveRecord[220];
  snprintf(graveRecord, sizeof(graveRecord),
           "GEN %u | Alder: %lu min | Doed: %s | Pong: %u | Labyrint: %u sek | \"%s\"\n",
           (unsigned int)dyingPet.getGeneration(),
           (unsigned long)dyingPet.getAgeMinutes(),
           dyingPet.getDeathCauseName(),
           (unsigned int)dyingPet.getHighScorePong(),
           (unsigned int)dyingPet.getHighScoreMaze(),
           dyingPet.getDeathEpitaph());
  CHECK(strstr(graveRecord, "GEN 1 |") != NULL, "Grave record has GEN 1");
  CHECK(strstr(graveRecord, "Doed: Doede af sult") != NULL, "Grave record has cause");
  CHECK(strstr(graveRecord, "Maven rumlede til sidst...") != NULL, "Grave record has epitaph");
}

void testMemorialGallery() {
  printf("\n--- Test: Memorial Gallery & grave.txt Parser ---\n");

  // 1. Test pure string parser for grave.txt line
  GraveRecord rec;
  const char* lineHunger = "GEN 1 | Alder: 145 min | Doed: Doede af sult | Pong: 12 | Labyrint: 45 sek | \"Maven rumlede til sidst...\"\n";
  bool ok1 = StoragePet::parseGraveLine(lineHunger, rec);
  CHECK(ok1, "parseGraveLine succeeds for hunger record");
  CHECK(rec.generation == 1, "Parsed generation is 1");
  CHECK(rec.ageMinutes == 145, "Parsed age is 145 minutes");
  CHECK(rec.deathCause == DEATH_HUNGER, "Parsed cause is DEATH_HUNGER");
  CHECK(rec.highScorePong == 12, "Parsed Pong score is 12");
  CHECK(rec.highScoreMaze == 45, "Parsed Maze time is 45");
  CHECK(strcmp(rec.epitaph, "Maven rumlede til sidst...") == 0, "Parsed epitaph matches exactly");

  const char* lineCold = "GEN 2 | Alder: 320 min | Doed: Froes ihjel | Pong: 30 | Labyrint: 28 sek | \"Vinteren var for haard...\"\n";
  bool ok2 = StoragePet::parseGraveLine(lineCold, rec);
  CHECK(ok2, "parseGraveLine succeeds for cold record");
  CHECK(rec.generation == 2, "Parsed generation is 2");
  CHECK(rec.deathCause == DEATH_COLD, "Parsed cause is DEATH_COLD");

  const char* lineHeat = "GEN 3 | Alder: 80 min | Doed: Doede af hedeslag | Pong: 5 | Labyrint: 0 sek | \"Smeltede i pejsens varme!\"\n";
  bool ok3 = StoragePet::parseGraveLine(lineHeat, rec);
  CHECK(ok3, "parseGraveLine succeeds for heat record");
  CHECK(rec.deathCause == DEATH_HEAT, "Parsed cause is DEATH_HEAT");

  const char* lineBoredom = "GEN 4 | Alder: 500 min | Doed: Doede af ensomhed | Pong: 10 | Labyrint: 60 sek | \"Hjertet visnede af savn.\"\n";
  bool ok4 = StoragePet::parseGraveLine(lineBoredom, rec);
  CHECK(ok4, "parseGraveLine succeeds for boredom record");
  CHECK(rec.deathCause == DEATH_BOREDOM, "Parsed cause is DEATH_BOREDOM");

  const char* lineExhaustion = "GEN 5 | Alder: 600 min | Doed: Doede af udmattelse | Pong: 20 | Labyrint: 50 sek | \"Fik aldrig sovet ud...\"\n";
  bool ok5 = StoragePet::parseGraveLine(lineExhaustion, rec);
  CHECK(ok5, "parseGraveLine succeeds for exhaustion record");
  CHECK(rec.deathCause == DEATH_EXHAUSTION, "Parsed cause is DEATH_EXHAUSTION");

  const char* lineOldAge = "GEN 6 | Alder: 1440 min | Doed: Doede af alderdom | Pong: 88 | Labyrint: 19 sek | \"Levede et lykkeligt liv!\"\n";
  bool ok6 = StoragePet::parseGraveLine(lineOldAge, rec);
  CHECK(ok6, "parseGraveLine succeeds for old age record");
  CHECK(rec.deathCause == DEATH_OLD_AGE, "Parsed cause is DEATH_OLD_AGE");

  // Malformed line handling
  CHECK(!StoragePet::parseGraveLine(nullptr, rec), "Safe handling of nullptr line");
  CHECK(!StoragePet::parseGraveLine("", rec), "Safe handling of empty line");
  CHECK(!StoragePet::parseGraveLine("Corrupted text without markers", rec), "Safe handling of corrupted line");

  // 2. Test in-memory storage logging and retrieval fallback
  StoragePet storage;
  PetLogic ancestor1;
  ancestor1.setGeneration(1);
  ancestor1.setHighScorePong(15);
  ancestor1.triggerDeath(DEATH_HUNGER);
  storage.logGraveRecord(ancestor1);

  PetLogic ancestor2;
  ancestor2.setGeneration(2);
  ancestor2.setHighScoreMaze(35);
  ancestor2.triggerDeath(DEATH_OLD_AGE);
  storage.logGraveRecord(ancestor2);

  uint16_t count = storage.getGraveRecordCount();
  CHECK(count >= 2, "Grave record count is at least 2");

  GraveRecord loadedRec;
  bool loadedOk = storage.loadGraveRecord(count - 2, loadedRec);
  CHECK(loadedOk, "Loads ancestor 1 successfully");
  CHECK(loadedRec.generation == 1, "Ancestor 1 has generation 1");

  loadedOk = storage.loadGraveRecord(count - 1, loadedRec);
  CHECK(loadedOk, "Loads ancestor 2 successfully");
  CHECK(loadedRec.generation == 2, "Ancestor 2 has generation 2");
  CHECK(loadedRec.deathCause == DEATH_OLD_AGE, "Ancestor 2 died of old age");

  CHECK(!storage.loadGraveRecord(999, loadedRec), "Out-of-bounds record index returns false");

  // 3. Test MenuLogic memorial state machine and navigation
  MenuLogic menu;
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "Starts on HOME");

  menu.openMemorial(5, SCREEN_HOME);
  CHECK(menu.getActiveScreen() == SCREEN_MEMORIAL, "Switches to SCREEN_MEMORIAL");
  CHECK(menu.getMemorialIndex() == 4, "Initial memorial index defaults to newest ancestor (index 4 of 5)");

  menu.memorialPrev(5);
  CHECK(menu.getMemorialIndex() == 3, "memorialPrev moves from 4 to 3");
  menu.memorialPrev(5);
  menu.memorialPrev(5);
  menu.memorialPrev(5);
  CHECK(menu.getMemorialIndex() == 0, "memorialPrev reached oldest ancestor index 0");
  menu.memorialPrev(5);
  CHECK(menu.getMemorialIndex() == 0, "memorialPrev clamps at index 0");

  menu.memorialNext(5);
  CHECK(menu.getMemorialIndex() == 1, "memorialNext moves from 0 to 1");
  menu.memorialNext(5);
  menu.memorialNext(5);
  menu.memorialNext(5);
  CHECK(menu.getMemorialIndex() == 4, "memorialNext reaches index 4");
  menu.memorialNext(5);
  CHECK(menu.getMemorialIndex() == 4, "memorialNext clamps at index 4");

  menu.onButtonBack();
  CHECK(menu.getActiveScreen() == SCREEN_HOME, "Returns to HOME screen via onButtonBack");

  // Memorial opened from STATS returns back to STATS
  menu.setScreen(SCREEN_STATS);
  menu.openMemorial(3, SCREEN_STATS);
  CHECK(menu.getActiveScreen() == SCREEN_MEMORIAL, "Opened from STATS");
  menu.onButtonBack();
  CHECK(menu.getActiveScreen() == SCREEN_STATS, "Returns to STATS screen via onButtonBack");
}

void testEggHatchingAndPettingLogic() {
  printf("\n--- Test: Egg Hatching Sequence & Proximity Petting ---\n");

  // 1. Egg Hatching timing & transitions
  PetLogic dyingPet;
  dyingPet.setGeneration(1);
  dyingPet.setHighScorePong(18);
  dyingPet.setHighScoreMaze(32);
  dyingPet.triggerDeath(DEATH_HUNGER);
  CHECK(!dyingPet.isAlive(), "Pet is initially dead");
  CHECK(!dyingPet.isHatching(), "Not hatching initially");

  dyingPet.startHatching(1000);
  CHECK(dyingPet.isHatching(), "Hatching is now active");
  CHECK(dyingPet.getHatchingPhase(1000) == 0, "Phase 0 (wobble) at 0ms");
  CHECK(dyingPet.getHatchingPhase(1800) == 0, "Phase 0 (wobble) at 800ms");
  CHECK(dyingPet.getHatchingPhase(2000) == 1, "Phase 1 (crack) at 1000ms");
  CHECK(dyingPet.getHatchingPhase(2700) == 1, "Phase 1 (crack) at 1700ms");
  CHECK(dyingPet.getHatchingPhase(3000) == 2, "Phase 2 (breakout) at 2000ms");
  CHECK(dyingPet.getHatchingPhase(3700) == 2, "Phase 2 (breakout) at 2700ms");

  // Before 2800ms has elapsed: still hatching
  dyingPet.update(3700); // 2700ms elapsed
  CHECK(dyingPet.isHatching(), "Still hatching before 2800ms");
  CHECK(!dyingPet.isAlive(), "Pet not alive until hatching finishes");

  // At 2800ms+: auto-completes to rebirth!
  dyingPet.update(3850); // 2850ms elapsed
  CHECK(!dyingPet.isHatching(), "Hatching finished after 2850ms");
  CHECK(dyingPet.isAlive(), "Pet is now alive as newborn!");
  CHECK(dyingPet.getGeneration() == 2, "Generation incremented to 2");
  CHECK(dyingPet.getStageEnum() == STAGE_BABY, "Reborn as cute Baby");
  CHECK(dyingPet.getHighScorePong() == 18, "Pong score preserved");
  CHECK(dyingPet.getHighScoreMaze() == 32, "Maze score preserved");
  CHECK(dyingPet.getOverallHealth() > 70, "Health restored to full");

  // 2. Proximity Petting & Affection
  PetLogic pet;
  pet.setVitals(80, 80, 80, 50, 50); // Happiness = 50%
  CHECK(!pet.isBeingPetted(1000), "Not being petted initially");

  bool pettedOk = pet.pet(1000);
  CHECK(pettedOk, "Petting succeeds");
  CHECK(pet.isBeingPetted(1500), "Is being petted within 1.8s window");
  CHECK(pet.getHappiness() == 62.0f, "Happiness increased by +12% (50 -> 62)");

  // Spam protection / cooldown
  bool spamOk = pet.pet(1200); // Only 200ms later
  CHECK(!spamOk, "Cooldown prevents spam petting");
  CHECK(pet.getHappiness() == 62.0f, "Happiness unchanged during cooldown");

  // Petting after cooldown (1500ms+)
  bool nextPetOk = pet.pet(2600); // 1600ms later
  CHECK(nextPetOk, "Petting succeeds after 1.5s cooldown");
  CHECK(pet.getHappiness() == 74.0f, "Happiness increased by +12% (62 -> 74)");

  // Reaction window expiration
  CHECK(!pet.isBeingPetted(4500), "Petting reaction window expires after 1.8s");

  // Petting wakes sleeping pet
  pet.setAmbientLight(10, 0); // immediate dark sleep trigger
  CHECK(pet.isSleeping(), "Pet is asleep in the dark");
  pet.pet(10000);
  CHECK(!pet.isSleeping(), "Petting gently wakes sleeping Lumi");

  // Cannot pet deceased pet
  pet.triggerDeath(DEATH_COLD);
  CHECK(!pet.pet(20000), "Cannot pet deceased pet");
}

void testBerryBankAndMedicineLogic() {
  printf("\n--- Test: Berry Bank & Medicine Economics ---\n");
  PetLogic pet;

  CHECK(pet.getBerries() == 0, "Berry bank starts empty (0 berries)");

  pet.addBerries(10);
  CHECK(pet.getBerries() == 10, "Earned 10 berries in wallet");

  CHECK(pet.spendBerries(4), "Spent 4 berries successfully");
  CHECK(pet.getBerries() == 6, "Remaining balance is 6 berries");

  CHECK(!pet.spendBerries(10), "Cannot spend more berries than balance");
  CHECK(pet.getBerries() == 6, "Balance unchanged after failed spend");

  // Preserved across rebirth
  pet.rebirth();
  CHECK(pet.getBerries() == 6, "Berry bank preserved across rebirth into next generation");
}

void testPoopAndBathCleaningLogic() {
  printf("\n--- Test: Poop Generation & Bath Cleaning ---\n");
  PetLogic pet;

  CHECK(!pet.getHasPoop(), "No poop initially");

  pet.setVitals(50.0f, 80.0f, 80.0f, 50.0f, 50.0f);
  bool feedOk = pet.feed();
  CHECK(feedOk, "Pet feeds successfully");
  CHECK(pet.getHasPoop(), "Poop appears on glade after eating");
  CHECK(strcmp(pet.getMoodStatusDanish(), "Poelse paa skaermen! Ryst!") == 0, "Mood warns about poop on screen");

  // Clean poop via shaking bath
  pet.cleanPoop();
  CHECK(!pet.getHasPoop(), "Poop washed away by bath");
  CHECK(pet.getHappiness() == 70.0f, "Cleaning poop boosts happiness by +15% (55 -> 70)");

  // Neglect poop for >= 60 seconds -> infection / sickness
  pet.setHasPoop(true, 1000);
  pet.update(1000); // init decay timer
  CHECK(!pet.isSick(), "Pet not sick immediately after poop appears");

  pet.update(62000); // 61s later
  CHECK(pet.isSick(), "Neglecting poop for > 60s triggers sickness/infection!");
}

void testSicknessProgressionAndCure() {
  printf("\n--- Test: Sickness Progression & Apothecary Cure ---\n");
  PetLogic pet;

  CHECK(!pet.isSick(), "Pet is healthy initially");

  pet.setSick(true);
  CHECK(pet.isSick(), "Pet is now sick");
  CHECK(strcmp(pet.getMoodStatusDanish(), "Lumi er syg! Giv medicin!") == 0, "Mood calls for medicine");

  // Cannot cure with 0 berries
  CHECK(pet.getBerries() == 0, "Starts with 0 berries");
  CHECK(!pet.cureSickness(3), "Cannot buy medicine without sufficient berries");
  CHECK(pet.isSick(), "Pet remains sick when untreated");

  // Cure with 5 berries
  pet.addBerries(5);
  pet.setVitals(60.0f, 60.0f, 60.0f, 40.0f, 50.0f);
  CHECK(pet.cureSickness(3), "Curing pet with 3 berries succeeds!");
  CHECK(!pet.isSick(), "Pet is fully cured and healthy again!");
  CHECK(pet.getBerries() == 2, "3 berries deducted from bank (5 - 3 = 2)");
  CHECK(pet.getHappiness() == 60.0f, "Happiness boosted +20% on healing (40 -> 60)");

  // Cannot cure an already healthy pet
  CHECK(!pet.cureSickness(1), "Cannot cure a healthy pet");

  // Sustained untreated sickness causes death
  pet.setSick(true);
  pet.update(100000); // init decay timer
  for (int i = 0; i < 13; i++) {
    pet.update(100000 + ((i + 1) * 10000));
  }
  CHECK(!pet.isAlive(), "Untreated sickness for 12 ticks causes death");
  CHECK(pet.getDeathCause() == DEATH_EXHAUSTION, "Death cause is exhaustion / severe illness");
}

void testStorageCarePersistence() {
  printf("\n--- Test: MicroSD Storage with Berries, Sickness & Poop ---\n");
  PetLogic pet;
  pet.setBerries(25);
  pet.setSick(true);
  pet.setHasPoop(true);

  char buffer[320];
  StoragePet::serializeState(buffer, sizeof(buffer), pet);

  CHECK(strstr(buffer, "BERRIES=25") != nullptr, "Serialized text contains BERRIES=25");
  CHECK(strstr(buffer, "SICK=1") != nullptr, "Serialized text contains SICK=1");
  CHECK(strstr(buffer, "POOP=1") != nullptr, "Serialized text contains POOP=1");

  float h = 0, w = 0, e = 0, hap = 0, d = 0;
  bool asleep = false;
  PetStage stage = STAGE_BABY;
  PetOutfit outfit = OUTFIT_DEFAULT;
  uint16_t pong = 0, maze = 0, gen = 1, berries = 0;
  uint32_t age = 0;
  bool alive = true, sick = false, poop = false;
  DeathCause dc = DEATH_NONE;

  bool ok = StoragePet::deserializeState(buffer, h, w, e, hap, d, asleep, stage, outfit, pong, maze, age, gen, alive, dc, berries, sick, poop);
  CHECK(ok, "Deserialization succeeds");
  CHECK(berries == 25, "Deserialized berries count is 25");
  CHECK(sick == true, "Deserialized sick status is true");
  CHECK(poop == true, "Deserialized poop status is true");
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
  testPetDeathAndRebirth();
  testMemorialGallery();
  testEggHatchingAndPettingLogic();
  testBerryBankAndMedicineLogic();
  testPoopAndBathCleaningLogic();
  testSicknessProgressionAndCure();
  testStorageCarePersistence();

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

