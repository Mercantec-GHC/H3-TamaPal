// WHAT: Implementation of menu navigation, screen switching, and smooth cursor physics
// WHY:  Implements State Machine pattern cleanly without any hardware dependencies
// USED BY: TamaPal.ino, ui_menu.cpp, tests/test_main.cpp

#include "menu_logic.h"

// Initialize menu starting on HOME screen with first icon (Food) selected
MenuLogic::MenuLogic()
  : activeScreen(SCREEN_HOME),
    memorialReturnScreen(SCREEN_HOME),
    selectedIcon(ICON_FOOD),
    selectedGame(0),
    memorialIndex(0),
    playerId(1),
    cursorPositionX(0.0f),
    targetPositionX(0.0f),
    actionStartTimeMs(0),
    pendingSound(SOUND_NONE) {
  // Set initial position to center of first icon
  targetPositionX = calculateIconCenterX(selectedIcon);
  cursorPositionX = targetPositionX;
}

// Calculate the center X position on screen for any given icon index (0..5)
// Formula: center of screen (120) + offset from visual middle (between icon 2 and 3: 2.5)
float MenuLogic::calculateIconCenterX(MenuIcon icon) {
  const float screenCenterX = SCREEN_WIDTH / 2.0f; // 120.0f
  const float offsetFromCenter = (float)icon - 2.5f;
  return screenCenterX + (offsetFromCenter * MENU_ICON_SPACING);
}

// Return Danish text label for the active icon (used under menu bar)
const char* MenuLogic::getIconLabelDanish(MenuIcon icon) {
  switch (icon) {
    case ICON_FOOD:     return "MAD";
    case ICON_PLAY:     return "LEG";
    case ICON_MEDICIN:  return "MEDICIN";
    case ICON_SCOLD:    return "SKAELD UD";
    case ICON_STATS:    return "STATISTIK";
    case ICON_CONNECT:  return "FORBIND";
    default:            return "";
  }
}

// TOUCH0: Move cursor to the previous icon (wrap around 0 -> 4)
void MenuLogic::onButtonPrev() {
  if (activeScreen == SCREEN_GAMES_MENU) {
    selectedGame = (selectedGame == 0) ? 1 : 0;
    pendingSound = SOUND_CLICK;
    return;
  }
  if (activeScreen != SCREEN_HOME) return;

  if (selectedIcon == 0) {
    selectedIcon = (MenuIcon)(ICON_TOTAL_COUNT - 1); // 0 -> 4
  } else {
    selectedIcon = (MenuIcon)(selectedIcon - 1);
  }

  updateTargetPosition();
  pendingSound = SOUND_CLICK;
}

// TOUCH1: Move cursor to the next icon (wrap around 4 -> 0)
void MenuLogic::onButtonNext() {
  if (activeScreen == SCREEN_GAMES_MENU) {
    selectedGame = (selectedGame == 0) ? 1 : 0;
    pendingSound = SOUND_CLICK;
    return;
  }
  if (activeScreen != SCREEN_HOME) return;

  if (selectedIcon >= ICON_TOTAL_COUNT - 1) {
    selectedIcon = (MenuIcon)0; // 4 -> 0
  } else {
    selectedIcon = (MenuIcon)(selectedIcon + 1);
  }

  updateTargetPosition();
  pendingSound = SOUND_CLICK;
}

// TOUCH2: OK / Select button - activate chosen icon
void MenuLogic::onButtonOk(unsigned long currentTimeMs) {
  if (activeScreen == SCREEN_GAMES_MENU) {
    pendingSound = SOUND_CONFIRM;
    if (selectedGame == 0) {
      activeScreen = SCREEN_MINIGAME_PONG;
    } else {
      activeScreen = SCREEN_MINIGAME_MAZE;
    }
    return;
  }

  if (activeScreen == SCREEN_HOME) {
    pendingSound = SOUND_CONFIRM;

    if (selectedIcon == ICON_STATS) {
      activeScreen = SCREEN_STATS;
    } else if (selectedIcon == ICON_CONNECT) {
      activeScreen = SCREEN_CHOOSE_PLAYER;
    } else if (selectedIcon == ICON_PLAY) {
      activeScreen = SCREEN_GAMES_MENU;
    } else {
      // Food or Scold opens temporary action overlay
      activeScreen = SCREEN_ACTION;
      actionStartTimeMs = currentTimeMs;
    }
  }
}

// Select player role (1 or 2) and advance to controller view
void MenuLogic::selectPlayer(int id) {
  playerId = (id == 2) ? 2 : 1;
  activeScreen = SCREEN_CONTROLLER;
  pendingSound = SOUND_CONFIRM;
}

void MenuLogic::setScreen(ScreenType screen, unsigned long currentTimeMs) {
  activeScreen = screen;
  if (screen == SCREEN_ACTION) {
    actionStartTimeMs = currentTimeMs;
  }
}

int MenuLogic::getPlayerId() const {
  return playerId;
}

uint8_t MenuLogic::getSelectedGame() const {
  return selectedGame;
}

void MenuLogic::selectGame(uint8_t gameIndex) {
  selectedGame = (gameIndex > 1) ? 1 : gameIndex;
}

void MenuLogic::openGamesMenu() {
  activeScreen = SCREEN_GAMES_MENU;
  pendingSound = SOUND_CONFIRM;
}

void MenuLogic::openPongGame() {
  activeScreen = SCREEN_MINIGAME_PONG;
  pendingSound = SOUND_CONFIRM;
}

void MenuLogic::openMazeGame() {
  activeScreen = SCREEN_MINIGAME_MAZE;
  pendingSound = SOUND_CONFIRM;
}

void MenuLogic::openMemorial(uint16_t totalCount, ScreenType returnScreen) {
  memorialReturnScreen = returnScreen;
  memorialIndex = (totalCount > 0) ? (totalCount - 1) : 0;
  activeScreen = SCREEN_MEMORIAL;
  pendingSound = SOUND_CONFIRM;
}

uint16_t MenuLogic::getMemorialIndex() const {
  return memorialIndex;
}

void MenuLogic::setMemorialIndex(uint16_t index) {
  memorialIndex = index;
}

void MenuLogic::memorialNext(uint16_t totalCount) {
  if (totalCount == 0) return;
  if (memorialIndex + 1 < totalCount) {
    memorialIndex++;
    pendingSound = SOUND_CLICK;
  }
}

void MenuLogic::memorialPrev(uint16_t totalCount) {
  if (totalCount == 0) return;
  if (memorialIndex > 0) {
    memorialIndex--;
    pendingSound = SOUND_CLICK;
  }
}

ScreenType MenuLogic::getMemorialReturnScreen() const {
  return memorialReturnScreen;
}

// TOUCH3: Back button - return to HOME screen from any other view
void MenuLogic::onButtonBack() {
  if (activeScreen == SCREEN_HATCHING) {
    return; // Cannot cancel cinematic egg hatching sequence
  }
  if (activeScreen == SCREEN_MINIGAME_PONG || activeScreen == SCREEN_MINIGAME_MAZE) {
    activeScreen = SCREEN_GAMES_MENU;
    pendingSound = SOUND_BACK;
    return;
  }
  if (activeScreen == SCREEN_MEMORIAL) {
    activeScreen = memorialReturnScreen;
    pendingSound = SOUND_BACK;
    return;
  }
  if (activeScreen != SCREEN_HOME) {
    activeScreen = SCREEN_HOME;
    pendingSound = SOUND_BACK;
  }
}

// TOUCH4: Direct shortcut - toggle between HOME and STATS screen
void MenuLogic::onButtonStatsShortcut() {
  if (activeScreen == SCREEN_HATCHING) return;
  if (activeScreen == SCREEN_HOME) {
    activeScreen = SCREEN_STATS;
    pendingSound = SOUND_CLICK;
  } else if (activeScreen == SCREEN_STATS) {
    activeScreen = SCREEN_HOME;
    pendingSound = SOUND_BACK;
  }
}

// Update target X position whenever selected icon changes
void MenuLogic::updateTargetPosition() {
  targetPositionX = calculateIconCenterX(selectedIcon);
}

// Advance physics animation and handle temporary action timeout
void MenuLogic::update(unsigned long currentTimeMs) {
  // 1. Smoothly slide selection frame towards target (Lerp: 25% distance per frame)
  float distanceToTarget = targetPositionX - cursorPositionX;
  if (distanceToTarget > 0.5f || distanceToTarget < -0.5f) {
    cursorPositionX += distanceToTarget * MENU_SLIDE_SPEED;
  } else {
    cursorPositionX = targetPositionX; // Snap when close enough
  }

  // 2. Auto-return from temporary action overlay back to HOME
  if (activeScreen == SCREEN_ACTION) {
    if (currentTimeMs - actionStartTimeMs >= ACTION_MESSAGE_DURATION_MS) {
      activeScreen = SCREEN_HOME;
    }
  }
}

// Getter: which screen is currently active
ScreenType MenuLogic::getActiveScreen() const {
  return activeScreen;
}

// Getter: which icon is currently selected
MenuIcon MenuLogic::getSelectedIcon() const {
  return selectedIcon;
}

// Getter: current animated X coordinate of the selection frame
float MenuLogic::getCursorPositionX() const {
  return cursorPositionX;
}

// Getter: final target X coordinate of the selected icon
float MenuLogic::getTargetPositionX() const {
  return targetPositionX;
}

// Return true if cursor animation is still moving
bool MenuLogic::isCursorMoving() const {
  float distance = targetPositionX - cursorPositionX;
  return (distance > 0.5f || distance < -0.5f);
}

// Retrieve any sound effect triggered by button presses, then clear it
SoundType MenuLogic::popSoundEffect() {
  SoundType effect = pendingSound;
  pendingSound = SOUND_NONE;
  return effect;
}

