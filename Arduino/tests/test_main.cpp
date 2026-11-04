// WHAT: Unit test suite for MenuLogic class running on host Mac / PC
// WHY:  Test our state machine and math automatically without uploading to hardware
// USED BY: tests/run_tests.sh

#include <cstdio>
#include <cmath>
#include <cstring>
#include "menu_logic.h"
#include "tilt_logic.h"

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

  // 4. Select CONNECT -> CONTROLLER
  menu.onButtonNext(); // 4 (CONNECT)
  menu.onButtonOk(1500);
  CHECK(menu.getActiveScreen() == SCREEN_CONTROLLER, "OK on CONNECT icon opens SCREEN_CONTROLLER");
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
  CHECK(TiltLogic::parseEventType("unknown") == EVENT_NONE, "Parses 'unknown' -> EVENT_NONE");
  CHECK(TiltLogic::parseEventType(nullptr) == EVENT_NONE, "Safely handles nullptr -> EVENT_NONE");
}

int main() {
  printf("==================================================\n");
  printf("TAMAPAL UNIT TEST RUNNER: MenuLogic & TiltLogic\n");
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

