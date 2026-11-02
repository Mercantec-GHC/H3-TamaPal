// WHAT: Unit test suite for MenuLogic class running on host Mac / PC
// WHY:  Test our state machine and math automatically without uploading to hardware
// USED BY: tests/run_tests.sh

#include <cstdio>
#include <cmath>
#include "menu_logic.h"

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

int main() {
  printf("==================================================\n");
  printf("TAMAPAL UNIT TEST RUNNER: MenuLogic\n");
  printf("==================================================\n");

  testInitialState();
  testNavigationWrapping();
  testScreenSwitching();
  testActionScreenTimeout();
  testInputDisabledOnOtherScreens();
  testSmoothCursorPhysics();

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

