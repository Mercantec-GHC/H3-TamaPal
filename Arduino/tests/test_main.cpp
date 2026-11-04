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
  CHECK(strcmp(pet.getMoodStatusDanish(), "Lumi fryser!") == 0, "Reports Danish mood: 'Lumi fryser!'");

  pet.setSleeping(true);
  CHECK(strcmp(pet.getMoodStatusDanish(), "Sover zZz...") == 0, "Reports Danish mood: 'Sover zZz...'");
}

void testPetSerialization() {
  printf("\n--- Test: PetLogic JSON Serialization & Life State ---\n");
  PetLogic pet;
  char buf[256];

  CHECK(pet.isAlive(), "Default Lumi is alive");
  CHECK(strcmp(pet.getStage(), "teen") == 0, "Default stage is teen");

  // Format default state JSON
  pet.formatStateJson(buf, sizeof(buf));
  CHECK(strstr(buf, "\"name\":\"Lumi\"") != NULL, "JSON contains name Lumi");
  CHECK(strstr(buf, "\"stage\":\"teen\"") != NULL, "JSON contains stage teen");
  CHECK(strstr(buf, "\"hunger\":80") != NULL, "JSON contains hunger 80");
  CHECK(strstr(buf, "\"warmth\":85") != NULL, "JSON contains warmth 85");
  CHECK(strstr(buf, "\"energy\":90") != NULL, "JSON contains energy 90");
  CHECK(strstr(buf, "\"happy\":75") != NULL, "JSON contains happy 75");
  CHECK(strstr(buf, "\"discipline\":50") != NULL, "JSON contains discipline 50");
  CHECK(strstr(buf, "\"alive\":true") != NULL, "JSON contains alive true");

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

  // Test deserialization into fresh variables
  float h = 0, w = 0, e = 0, hap = 0, d = 0;
  bool asleep = false;
  CHECK(StoragePet::deserializeState(buffer, h, w, e, hap, d, asleep), "Deserialization succeeds on valid text");
  CHECK((int)h == 77, "Restored hunger is 77");
  CHECK((int)w == 88, "Restored warmth is 88");
  CHECK((int)e == 66, "Restored energy is 66");
  CHECK((int)hap == 99, "Restored happiness is 99");
  CHECK((int)d == 44, "Restored discipline is 44");
  CHECK(asleep == true, "Restored asleep state is true");

  // Test corrupt or truncated inputs
  CHECK(!StoragePet::deserializeState("HUNGER=50\n", h, w, e, hap, d, asleep), "Fails on incomplete file");
  CHECK(!StoragePet::deserializeState("HUNGER=999\nWARMTH=50\nENERGY=50\nHAPPINESS=50\nDISCIPLINE=50\nASLEEP=0\n", h, w, e, hap, d, asleep), "Fails on out-of-range value (999)");
  CHECK(!StoragePet::deserializeState(nullptr, h, w, e, hap, d, asleep), "Fails safely on nullptr");
  CHECK(!StoragePet::deserializeState("", h, w, e, hap, d, asleep), "Fails safely on empty string");
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

