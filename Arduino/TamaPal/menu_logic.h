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

  // Select player role (1 or 2) and advance to SCREEN_CONTROLLER
  void selectPlayer(int playerId);
  int getPlayerId() const;

  // Direct screen state transition
  void setScreen(ScreenType screen, unsigned long currentTimeMs = 0);

  // Games menu helpers
  uint8_t getSelectedGame() const;
  void selectGame(uint8_t gameIndex);
  void openGamesMenu();
  void openPongGame();
  void openMazeGame();

  // House menu (Lumis Hus: Garderobe & Soevn) helpers
  uint8_t getSelectedHouseOption() const;
  void selectHouseOption(uint8_t optionIndex);
  void openHouseMenu();

  // Wardrobe helpers
  uint8_t getSelectedWardrobeOption() const;
  void selectWardrobeOption(uint8_t outfitIndex);
  void openWardrobeMenu();

  // Memorial gallery navigation
  void openMemorial(uint16_t totalCount, ScreenType returnScreen = SCREEN_HOME);
  uint16_t getMemorialIndex() const;
  void setMemorialIndex(uint16_t index);
  void memorialNext(uint16_t totalCount);
  void memorialPrev(uint16_t totalCount);
  ScreenType getMemorialReturnScreen() const;
  MemorialViewMode getMemorialViewMode() const;
  void setMemorialViewMode(MemorialViewMode mode);

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
  ScreenType memorialReturnScreen;
  MenuIcon selectedIcon;
  uint8_t selectedGame;
  uint8_t selectedHouseOption;
  uint8_t selectedWardrobeOption;
  uint16_t memorialIndex;
  MemorialViewMode memorialViewMode;
  int playerId;

  float cursorPositionX;
  float targetPositionX;

  unsigned long actionStartTimeMs;
  SoundType pendingSound;

  void updateTargetPosition();
};

#endif // MENU_LOGIC_H

