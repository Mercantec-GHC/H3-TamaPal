// WHAT: Pure business logic for the navigation menu and screen state machine
// WHY:  Separated from display & hardware so we can run unit tests on Mac without Arduino
// USED BY: TamaPal.ino, ui_menu.cpp, tests/test_main.cpp

#ifndef MENU_LOGIC_H
#define MENU_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "settings.h"

class MenuLogic {
public:
  MenuLogic();

  // Button input handlers
  void onButtonPrev();
  void onButtonNext();
  void onButtonOk(unsigned long currentTimeMs);
  void onButtonBack();
  void onButtonStatsShortcut();

  // Periodic update for animation interpolation and auto-close timers
  void update(unsigned long currentTimeMs);

  // State inspection (getters)
  ScreenType getActiveScreen() const;
  MenuIcon getSelectedIcon() const;
  float getCursorPositionX() const;
  float getTargetPositionX() const;
  bool isCursorMoving() const;

  // Retrieve pending sound effect and clear it
  SoundType popSoundEffect();

  // Helper: compute target center X in pixels for a given icon (0..4)
  static float calculateIconCenterX(MenuIcon icon);

  // Helper: return uppercase Danish label for the given icon
  static const char* getIconLabelDanish(MenuIcon icon);

private:
  ScreenType activeScreen;
  MenuIcon selectedIcon;

  float cursorPositionX;
  float targetPositionX;

  unsigned long actionStartTimeMs;
  SoundType pendingSound;

  void updateTargetPosition();
};

#endif // MENU_LOGIC_H

