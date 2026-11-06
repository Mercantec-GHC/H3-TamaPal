// WHAT: Main Arduino entrypoint sketch for TamaPal (setup & non-blocking game loop)
// WHY:  Orchestrates inputs, pure logic, sound effects, and UI rendering cleanly
// USED BY: Arduino MKR WiFi 1010 + MKR IoT Carrier

#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "menu_logic.h"
#include "ui_menu.h"
#include "sound_led.h"
#include "tilt_logic.h"
#include "ui_controller.h"
#include "net_mqtt.h"
#include "pet_logic.h"
#include "ui_stats.h"
#include "storage_pet.h"
#include "ui_pet.h"
#include "pong_logic.h"
#include "ui_pong.h"
#include "maze_logic.h"
#include "ui_maze.h"
#include "ui_memorial.h"

// Hardware carrier instance
MKRIoTCarrier carrier;

// Software layers (Clean Architecture: Logic -> UI -> Hardware)
MenuLogic menuLogic;
UiMenuRenderer uiMenu;
SoundLedController soundLed;
TiltLogic tiltLogic;
UiControllerRenderer uiController;
NetMqttClient netMqtt;
PetLogic petLogic;
UiStatsRenderer uiStats;
StoragePet storagePet;
UiPetRenderer uiPet;
PongLogic pongLogic;
UiPongRenderer uiPong;
MazeLogic mazeLogic;
UiMazeRenderer uiMaze;
UiMemorialRenderer uiMemorial;

// Forward declarations
void onGameEventReceived(GameEvent event);
void onPetRewardReceived(const char* type, int amount);
void syncPetStateToBackend(bool forceImmediate = false);

// Incoming game event callback (hit, miss, gameover, slot auto-assignment)
void onGameEventReceived(GameEvent event) {
  if (event == EVENT_ASSIGN_P2) {
    if (menuLogic.getPlayerId() != 2) {
      menuLogic.selectPlayer(2);
      netMqtt.setPlayerId(2);
      uiController.drawInitialScreen(2);
      soundLed.setMoodColor(255, 70, 50); // Automatically switch LEDs to Coral/Red for P2
      soundLed.playSound(SOUND_CONFIRM);
    }
    return;
  } else if (event == EVENT_ASSIGN_P1) {
    if (menuLogic.getPlayerId() != 1) {
      menuLogic.selectPlayer(1);
      netMqtt.setPlayerId(1);
      uiController.drawInitialScreen(1);
      soundLed.setMoodColor(0, 180, 255); // Automatically switch LEDs to Cyan/Blue for P1
      soundLed.playSound(SOUND_CONFIRM);
    }
    return;
  }

  soundLed.playEventFeedback(event);
}

// Handler for incoming pet rewards from backend (e.g. happy_boost after Pong point)
void onPetRewardReceived(const char* type, int amount) {
  Serial.print(F("[REWARD] Modtaget belønning fra backend: "));
  Serial.print(type);
  Serial.print(F(" +"));
  Serial.println(amount);

  if (petLogic.applyReward(type, amount)) {
    soundLed.playSound(SOUND_CONFIRM);
    soundLed.flashColor(255, 230, 80, 400); // 400 ms golden reward flash, then restores base color
    syncPetStateToBackend(true);            // Sync immediately to update stats in DB
  }
}

// Track screen transitions to redraw viewports only when needed
ScreenType previousScreen = SCREEN_HOME;

// Performance monitor (measures frames per second without blocking)
unsigned long lastFpsTimeMs = 0;
uint32_t frameCount = 0;

// Debounce / cooldown timer to prevent hypersensitive multi-triggers
unsigned long lastTouchTimeMs = 0;

// Non-blocking Mute toggle state (prevents multi-trigger and screen blinking)
bool combo04Fired = false;
unsigned long combo04StartTime = 0;
bool touch3HoldFired = false;
unsigned long touch3StartTime = 0;
unsigned long muteMessageEndTimeMs = 0;

// Non-blocking Wardrobe toggle state (Hold TOUCH0 + TOUCH1 for 400ms on HOME)
bool combo01Fired = false;
unsigned long combo01StartTime = 0;

// Controller button edge-trigger tracking (for instant low-latency publishing)
bool lastCtrlUp = false;
bool lastCtrlDown = false;
bool lastCtrlAction = false;

// Pet state sync to backend interval (30 seconds or immediately on action)
unsigned long lastPetStatePublishTimeMs = 0;

void syncPetStateToBackend(bool forceImmediate) {
  unsigned long now = millis();
  if (!forceImmediate && (now - lastPetStatePublishTimeMs < MQTT_PET_STATE_INTERVAL_MS)) {
    return;
  }
  lastPetStatePublishTimeMs = now;

  // 1. Save to local MicroSD card (avoid SPI access during active controller gameplay)
  if (forceImmediate || menuLogic.getActiveScreen() != SCREEN_CONTROLLER) {
    storagePet.savePet(petLogic);
  }

  // 2. Publish to MQTT broker if connected
  if (netMqtt.isConnected()) {
    char petJson[160];
    petLogic.formatStateJson(petJson, sizeof(petJson), "Lumi", petLogic.getStage());
    netMqtt.publishPetState(petJson);
  }
}

// Environmental sensor reading interval (every 1.0 second, non-blocking)
unsigned long lastSensorReadTimeMs = 0;
const unsigned long SENSOR_READ_INTERVAL_MS = 1000;

// Hardware case mode (true = with plastic enclosure, false = bare board)
bool currentCaseMode = SECRET_USE_CASE;

void setCaseMode(bool enableCase, bool reinitHardware = false) {
  currentCaseMode = enableCase;
  if (currentCaseMode) {
    carrier.withCase();
    carrier.Buttons.updateConfig(TOUCH_SENSITIVITY_WITH_CASE);
    if (reinitHardware) {
      carrier.Buttons.begin();
    }
    Serial.println(F("[Hardware] Tilstand: MED PLASTIK-CASE (Følsomhed: 60)"));
  } else {
    carrier.noCase();
    carrier.Buttons.updateConfig(TOUCH_SENSITIVITY_BARE_BOARD);
    if (reinitHardware) {
      carrier.Buttons.begin();
    }
    Serial.println(F("[Hardware] Tilstand: UDEN CASE / BARE BOARD (Følsomhed: 200)"));
  }
}

// Non-blocking Mute toggle gesture processing
void processMuteGestures(unsigned long now, bool touch0, bool touch3, bool touch4, ScreenType currentScreen) {
  // GESTURE 1: TOUCH0 + TOUCH4 combo (Hold both for 300 ms on HOME)
  if (currentScreen == SCREEN_HOME && touch0 && touch4) {
    if (combo04StartTime == 0) {
      combo04StartTime = now;
    } else if (!combo04Fired && (now - combo04StartTime >= 300)) {
      soundLed.toggleMute();
      combo04Fired = true;
      muteMessageEndTimeMs = now + 1400; // Toast visible for 1.4 seconds

      if (soundLed.isMuted()) {
        uiMenu.drawPlaceholderScreen("LYD: SLUKKET", "Lyd er slaaet fra");
      } else {
        uiMenu.drawPlaceholderScreen("LYD: TAENDT", "Lyd er slaaet til");
        soundLed.playSound(SOUND_CONFIRM);
      }
    }
  } else if (!touch0 && !touch4) {
    // Reset ONLY when BOTH fingers are lifted off
    combo04Fired = false;
    combo04StartTime = 0;
  }

  // GESTURE 2: Hold TOUCH3 alone for 1.2 seconds on HOME screen
  if (currentScreen == SCREEN_HOME && touch3 && !touch0 && !touch4) {
    if (touch3StartTime == 0) {
      touch3StartTime = now;
    } else if (!touch3HoldFired && (now - touch3StartTime >= 1200)) {
      soundLed.toggleMute();
      touch3HoldFired = true;
      muteMessageEndTimeMs = now + 1400;

      if (soundLed.isMuted()) {
        uiMenu.drawPlaceholderScreen("LYD: SLUKKET", "Lyd er slaaet fra");
      } else {
        uiMenu.drawPlaceholderScreen("LYD: TAENDT", "Lyd er slaaet til");
        soundLed.playSound(SOUND_CONFIRM);
      }
    }
  } else if (!touch3) {
    // Reset ONLY when finger is lifted off TOUCH3
    touch3HoldFired = false;
    touch3StartTime = 0;
  }
}

// Non-blocking Wardrobe toggle gesture processing (Hold TOUCH0 + TOUCH1 for 400ms on HOME)
void processWardrobeGestures(unsigned long now, bool touch0, bool touch1, ScreenType currentScreen) {
  if (currentScreen == SCREEN_HOME && touch0 && touch1) {
    if (combo01StartTime == 0) {
      combo01StartTime = now;
    } else if (!combo01Fired && (now - combo01StartTime >= 400)) {
      petLogic.toggleOutfit();
      combo01Fired = true;
      soundLed.playSound(SOUND_CONFIRM);
      muteMessageEndTimeMs = now + 1400; // Toast visible for 1.4 seconds

      if (petLogic.getOutfitEnum() == OUTFIT_WINTER) {
        uiMenu.drawPlaceholderScreen("GARDEROBE", "Vinterhue & toerklaede PAA");
      } else {
        uiMenu.drawPlaceholderScreen("GARDEROBE", "Standard toej (hue AF)");
      }
      syncPetStateToBackend(true);
    }
  } else if (!touch0 && !touch1) {
    // Reset ONLY when both fingers are lifted
    combo01Fired = false;
    combo01StartTime = 0;
  }
}

void setup() {
  Serial.begin(115200);

  // IMPORTANT: Set case mode and sensitivity BEFORE carrier.begin()!
  // touch_sensors_config() inside Qtouch reads the sensitivity during carrier.begin().
  setCaseMode(SECRET_USE_CASE, false);
  carrier.begin();

  // Initialize MicroSD card persistence and restore Lumi if file exists
  storagePet.begin();
  if (storagePet.loadPet(petLogic)) {
    Serial.println(F("[STORAGE] Lumi fortsætter fra gemt tilstand på MicroSD-kort!"));
  }

  // Set LCD rotation (0 = standard orientation)
  carrier.display.setRotation(0);

  // Initialize UI and audio controllers
  soundLed.begin(&carrier);
  uiMenu.begin(&carrier);
  uiController.begin(&carrier);
  uiStats.begin(&carrier);
  uiPet.begin(&carrier);
  uiPong.begin(&carrier);
  uiMaze.begin(&carrier);
  netMqtt.begin(onGameEventReceived, onPetRewardReceived);

  // Initial full screen draw
  uiMenu.drawInitialScreen();
  uiMenu.drawMenuBar(menuLogic, true);
  uiPet.updateHomeDisplay(petLogic, millis(), true);
  previousScreen = SCREEN_HOME;

  Serial.println(F("=================================================="));
  Serial.println(F("TamaPal klar! (MKR WiFi 1010 + MKR IoT Carrier)"));
  Serial.print(F("Touch-tilstand: "));
  Serial.println(currentCaseMode ? F("PLASTIK-CASE (Følsomhed 60)") : F("BARE BOARD (Følsomhed 200)"));
  Serial.println(F("Tip: Skriv 'c' i Serial Monitor for at skifte mellem case/bare board on-the-fly!"));
  Serial.println(F("Tip: Skriv 's' i Serial Monitor for at skifte stadie (Baby -> Teen -> Adult -> Old)!"));
  Serial.println(F("Tip: Skriv 'w' for at aabne Garderoben (vinterhue & halstoerklaede)!"));
  Serial.println(F("Tip: Skriv 'f' for at simulere frostkulde (12 grader -> Lumi fryser)!"));
  Serial.println(F("Tip: Skriv 'h' for at simulere hedeboelge (28 grader -> Lumi sveder)!"));
  Serial.println(F("Tip: Skriv 't' for at aele og klappe Lumi (+12% glaede <3)!"));
  Serial.println(F("Tip: Skriv 'm' for at aabne Mindelund (Lumis Slægtsbog & Galleri)!"));
  Serial.println(F("Garderobe-genvej: Hold TOUCH 0 (<) og TOUCH 1 (>) i 400 ms!"));
  Serial.println(F("Naviger med TOUCH 0 (<) og TOUCH 1 (>)"));
  Serial.println(F("For at forbinde til Pong over MQTT:"));
  Serial.println(F(" 1. Gaa til CONNECT (antenne-ikonet helt til hoejre)"));
  Serial.println(F(" 2. Tryk TOUCH 2 (OK)"));
  Serial.println(F(" 3. Vaelg spiller: TOUCH 0 (Spiller 1) eller TOUCH 1 / TOUCH 4 (Spiller 2)"));
  Serial.println(F("=================================================="));
}

void loop() {
  unsigned long now = millis();

  // Listen for commands in Serial Monitor:
  // 'c' = toggle plastic case sensitivity
  // 's' = cycle through life stages (Baby -> Teen -> Adult -> Old) on-the-fly!
  // 'w' = toggle wardrobe outfit (standard <-> vintertøj / beanie & scarf)
  // 'f' = simulate freezing cold (test winter clothing warming effect)
  // 'h' = simulate overheating heat wave (test sweating & hat removal)
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == 'c' || c == 'C') {
      setCaseMode(!currentCaseMode, true);
    } else if (c == 's' || c == 'S') {
      int nextStage = ((int)petLogic.getStageEnum() + 1) % 4;
      petLogic.setStage((PetStage)nextStage);
      soundLed.playSound(SOUND_CONFIRM);
      Serial.print(F("[STAGE] Skiftet stadie til: "));
      Serial.println(petLogic.getStage());
      if (menuLogic.getActiveScreen() == SCREEN_HOME) {
        uiPet.updateHomeDisplay(petLogic, now, true);
      }
      syncPetStateToBackend(true);
    } else if (c == 'w' || c == 'W') {
      petLogic.toggleOutfit();
      soundLed.playSound(SOUND_CONFIRM);
      Serial.print(F("[GARDEROBE] Toej skiftet til: "));
      Serial.println(petLogic.getOutfit());
      if (menuLogic.getActiveScreen() == SCREEN_HOME) {
        uiPet.updateHomeDisplay(petLogic, now, true);
      }
      syncPetStateToBackend(true);
    } else if (c == 'f' || c == 'F') {
      petLogic.setVitals(petLogic.getHunger(), 15.0f, petLogic.getEnergy(), petLogic.getHappiness(), petLogic.getDiscipline());
      petLogic.setAmbientTemperature(12.0f);
      soundLed.playSound(SOUND_MISS);
      Serial.println(F("[SIMULERING] Det er iskoldt! Lumi fryser (12 grader, varme 15%)."));
      if (menuLogic.getActiveScreen() == SCREEN_HOME) {
        uiPet.updateHomeDisplay(petLogic, now, true);
      }
      syncPetStateToBackend(true);
    } else if (c == 'h' || c == 'H') {
      petLogic.setVitals(petLogic.getHunger(), 85.0f, petLogic.getEnergy(), petLogic.getHappiness(), petLogic.getDiscipline());
      petLogic.setAmbientTemperature(28.0f);
      soundLed.playSound(SOUND_MISS);
      Serial.println(F("[SIMULERING] Det er hedt! Lumi sveder (28 grader, varme 85%)."));
      if (menuLogic.getActiveScreen() == SCREEN_HOME) {
        uiPet.updateHomeDisplay(petLogic, now, true);
      }
      syncPetStateToBackend(true);
    } else if (c == 'p' || c == 'P') {
      menuLogic.openPongGame();
      soundLed.playSound(SOUND_CONFIRM);
      Serial.println(F("[SPIL] Starter Solo Mur-Pong direkte via Serial Monitor!"));
    } else if (c == 'l' || c == 'L') {
      menuLogic.openMazeGame();
      soundLed.playSound(SOUND_CONFIRM);
      Serial.println(F("[SPIL] Starter Marmor-Labyrint direkte via Serial Monitor!"));
    } else if (c == 'k' || c == 'K') {
      // Demo death cycle (1..6) for teacher/exam presentation
      static uint8_t demoCause = 1;
      DeathCause cause = (DeathCause)demoCause;
      petLogic.triggerDeath(cause);
      soundLed.playSound(SOUND_DEATH);
      storagePet.logGraveRecord(petLogic);
      storagePet.savePet(petLogic);
      Serial.print(F("[LIVSCYKLUS] Lumi er doed! Aarsag: "));
      Serial.print(petLogic.getDeathCauseName());
      Serial.print(F(" - "));
      Serial.println(petLogic.getDeathEpitaph());
      menuLogic.setScreen(SCREEN_HOME, now);
      uiPet.updateHomeDisplay(petLogic, now, true);
      syncPetStateToBackend(true);
      demoCause = (demoCause % 6) + 1; // Cycle next death cause (1..6)
    } else if (c == 'r' || c == 'R') {
      if (!petLogic.isAlive()) {
        petLogic.startHatching(now);
        menuLogic.setScreen(SCREEN_HATCHING, now);
        soundLed.playSound(SOUND_CRACK);
        Serial.print(F("[LIVSCYKLUS] Aegget begynder at klaekke for Generation: "));
        Serial.println((unsigned int)(petLogic.getGeneration() + 1));
      }
    } else if (c == 't' || c == 'T') {
      if (petLogic.isAlive()) {
        if (petLogic.pet(now)) {
          soundLed.playSound(SOUND_PURR);
          soundLed.flashColor(255, 105, 180, 500); // 500ms pink cuddle glow
          Serial.println(F("[INTERAKTION] Du aeler og klapper Lumi virtuelt! (+12% glaede <3)"));
          syncPetStateToBackend(true);
        }
      }
    } else if (c == 'i' || c == 'I') {
      petLogic.setSick(!petLogic.isSick());
      soundLed.playSound(petLogic.isSick() ? SOUND_MISS : SOUND_CONFIRM);
      Serial.print(F("[SYGDOM] Lumi sygestatus skiftet til: "));
      Serial.println(petLogic.isSick() ? F("SYG (Kraever medicin!)") : F("RASK"));
      if (menuLogic.getActiveScreen() == SCREEN_HOME) {
        uiPet.updateHomeDisplay(petLogic, now, true);
      }
      syncPetStateToBackend(true);
    } else if (c == 'x' || c == 'X') {
      petLogic.setHasPoop(true, now);
      soundLed.playSound(SOUND_CLICK);
      Serial.println(F("[RENGØRING] Lumi har lavet en poelse paa skaermen! Ryst braettet!"));
      if (menuLogic.getActiveScreen() == SCREEN_HOME) {
        uiPet.updateHomeDisplay(petLogic, now, true);
      }
      syncPetStateToBackend(true);
    } else if (c == 'b' || c == 'B') {
      petLogic.cleanPoop();
      uiPet.triggerBath(now);
      soundLed.playSound(SOUND_BUBBLES);
      soundLed.flashColor(0, 220, 255, 500);
      Serial.println(F("[RENGØRING] Bad og saebeskum udloest! Poelse vasket vaek!"));
      if (menuLogic.getActiveScreen() == SCREEN_HOME) {
        uiPet.updateHomeDisplay(petLogic, now, true);
      }
      syncPetStateToBackend(true);
    } else if (c == 'g' || c == 'G') {
      petLogic.addBerries(5);
      soundLed.playSound(SOUND_CONFIRM);
      soundLed.flashColor(255, 215, 60, 300);
      Serial.print(F("[BAER-PUNG] +5 gyldne baer tilfoejet! Total i pungen: "));
      Serial.println((unsigned int)petLogic.getBerries());
      syncPetStateToBackend(true);
    }
  }

  // 1. INPUT LAYER: Poll capacitive touch pads on the carrier
  carrier.Buttons.update();

  bool touch0 = carrier.Buttons.getTouch(TOUCH0);
  bool touch1 = carrier.Buttons.getTouch(TOUCH1);
  bool touch3 = carrier.Buttons.getTouch(TOUCH3);
  bool touch4 = carrier.Buttons.getTouch(TOUCH4);

  ScreenType currentScreen = menuLogic.getActiveScreen();

  // Process Mute gestures (TOUCH0+TOUCH4 combo or hold TOUCH3)
  processMuteGestures(now, touch0, touch3, touch4, currentScreen);
  // Process Wardrobe gestures (TOUCH0+TOUCH1 combo: hold both arrows for 400ms)
  processWardrobeGestures(now, touch0, touch1, currentScreen);

  // -------------------------------------------------------------
  // Restore screen automatically when mute banner notification expires
  // -------------------------------------------------------------
  if (muteMessageEndTimeMs > 0 && now >= muteMessageEndTimeMs) {
    muteMessageEndTimeMs = 0;
    if (currentScreen == SCREEN_HOME) {
      uiMenu.drawInitialScreen();
      uiMenu.drawMenuBar(menuLogic, true);
      uiPet.updateHomeDisplay(petLogic, now, true);
    } else if (currentScreen == SCREEN_CONTROLLER) {
      uiController.drawInitialScreen(menuLogic.getPlayerId());
    }
  }

  // -------------------------------------------------------------
  // Button Navigation
  // -------------------------------------------------------------
  if (currentScreen == SCREEN_CHOOSE_PLAYER) {
    // In player role selection screen:
    // TOUCH0: Choose Player 1 (Venstre)
    // TOUCH1 or TOUCH4: Choose Player 2 (Hoejre)
    // TOUCH2: Quick confirm Player 1
    // TOUCH3: Cancel back to Lumi Home
    if (now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) {
      if (carrier.Buttons.onTouchDown(TOUCH0)) {
        menuLogic.selectPlayer(1);
        lastTouchTimeMs = now;
      } else if (carrier.Buttons.onTouchDown(TOUCH1) || carrier.Buttons.onTouchDown(TOUCH4)) {
        menuLogic.selectPlayer(2);
        lastTouchTimeMs = now;
      } else if (carrier.Buttons.onTouchDown(TOUCH2)) {
        menuLogic.selectPlayer(1);
        lastTouchTimeMs = now;
      } else if (carrier.Buttons.onTouchDown(TOUCH3)) {
        menuLogic.onButtonBack();
        lastTouchTimeMs = now;
      }
    }
  } else if (currentScreen == SCREEN_GAMES_MENU) {
    // Minigames selection screen (Pong vs Labyrinth):
    if (now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) {
      if (carrier.Buttons.onTouchDown(TOUCH0)) {
        menuLogic.onButtonPrev();
        uiMenu.drawGamesMenu(menuLogic.getSelectedGame(), petLogic.getHighScorePong(), petLogic.getHighScoreMaze(), false);
        lastTouchTimeMs = now;
      } else if (carrier.Buttons.onTouchDown(TOUCH1)) {
        menuLogic.onButtonNext();
        uiMenu.drawGamesMenu(menuLogic.getSelectedGame(), petLogic.getHighScorePong(), petLogic.getHighScoreMaze(), false);
        lastTouchTimeMs = now;
      } else if (carrier.Buttons.onTouchDown(TOUCH2)) {
        menuLogic.onButtonOk(now);
        lastTouchTimeMs = now;
      } else if (carrier.Buttons.onTouchDown(TOUCH3)) {
        menuLogic.onButtonBack();
        lastTouchTimeMs = now;
      }
    }
  } else if (currentScreen == SCREEN_MINIGAME_PONG) {
    // Solo Wall-Pong game on Oplà:
    if (pongLogic.getState() == PONG_STATE_READY || pongLogic.getState() == PONG_STATE_BALL_LOST) {
      if ((now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) && carrier.Buttons.onTouchDown(TOUCH2)) {
        pongLogic.serveBall();
        lastTouchTimeMs = now;
      } else if ((now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) && carrier.Buttons.onTouchDown(TOUCH3)) {
        menuLogic.onButtonBack();
        lastTouchTimeMs = now;
      }
    } else if (pongLogic.getState() == PONG_STATE_GAMEOVER) {
      if ((now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) && (carrier.Buttons.onTouchDown(TOUCH2) || carrier.Buttons.onTouchDown(TOUCH3))) {
        // Save high score and apply pet effects
        if (pongLogic.getScore() > petLogic.getHighScorePong()) {
          petLogic.setHighScorePong(pongLogic.getScore());
          storagePet.savePet(petLogic);
        }
        petLogic.applyGameReward(pongLogic.getScore());
        uint16_t earnedBerries = pongLogic.getScore() / 5;
        if (earnedBerries > 0) {
          petLogic.addBerries(earnedBerries);
          Serial.print(F("[BAER-PUNG] Pong sejr! +"));
          Serial.print((unsigned int)earnedBerries);
          Serial.println(F(" baer vundet til apoteket!"));
        }
        syncPetStateToBackend(true);

        menuLogic.onButtonBack(); // Return to games menu
        lastTouchTimeMs = now;
      }
    } else {
      // In active play: optional button paddle movement (rate-limited to 50 Hz / 20 ms)
      static unsigned long lastPaddleBtnMs = 0;
      if (now - lastPaddleBtnMs >= 20) {
        lastPaddleBtnMs = now;
        if (touch0) {
          pongLogic.movePaddle(4.5f); // Down
        }
        if (touch1) {
          pongLogic.movePaddle(-4.5f); // Up
        }
      }
      if ((now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) && carrier.Buttons.onTouchDown(TOUCH3)) {
        if (pongLogic.getScore() > 0) {
          if (pongLogic.getScore() > petLogic.getHighScorePong()) {
            petLogic.setHighScorePong(pongLogic.getScore());
            storagePet.savePet(petLogic);
          }
        }
        menuLogic.onButtonBack(); // Exit game early
        lastTouchTimeMs = now;
      }
    }
  } else if (currentScreen == SCREEN_MINIGAME_MAZE) {
    // Procedural Marble Maze: Pure 2D physical IMU tilt control (no arrow buttons)
    if (mazeLogic.getState() == MAZE_STATE_VICTORY) {
      if ((now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) && (carrier.Buttons.onTouchDown(TOUCH2) || carrier.Buttons.onTouchDown(TOUCH3))) {
        // Save best completion time (least seconds) to pet logic and MicroSD
        uint16_t elapsed = mazeLogic.getElapsedSeconds();
        if (petLogic.getHighScoreMaze() == 0 || elapsed < petLogic.getHighScoreMaze()) {
          petLogic.setHighScoreMaze(elapsed);
          storagePet.savePet(petLogic);
        }
        // Boost happiness & discipline, feed berries, spend energy
        uint8_t berries = mazeLogic.getBerriesCollected();
        if (berries > 0) {
          petLogic.addBerries(berries);
          Serial.print(F("[BAER-PUNG] Labyrint gennemfoert! +"));
          Serial.print((unsigned int)berries);
          Serial.println(F(" Deku-baer samlet i pungen!"));
        }
        float newHap = petLogic.getHappiness() + 35.0f + (berries * 5.0f);
        if (newHap > 100.0f) newHap = 100.0f;
        float newDisc = petLogic.getDiscipline() + 15.0f;
        if (newDisc > 100.0f) newDisc = 100.0f;
        float newHunger = petLogic.getHunger() + (berries * 8.0f);
        if (newHunger > 100.0f) newHunger = 100.0f;
        float newEnergy = (petLogic.getEnergy() >= 10) ? (petLogic.getEnergy() - 10.0f) : 0.0f;
        petLogic.setVitals(newHunger, petLogic.getWarmth(), newEnergy, newHap, newDisc);
        syncPetStateToBackend(true);

        menuLogic.onButtonBack(); // Return to games menu
        lastTouchTimeMs = now;
      }
    } else {
      // During active play: only TOUCH3 allows early exit back to menu
      if ((now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) && carrier.Buttons.onTouchDown(TOUCH3)) {
        menuLogic.onButtonBack();
        lastTouchTimeMs = now;
      }
    }
  } else if (currentScreen == SCREEN_CONTROLLER) {
    // In controller mode:
    // TOUCH0, TOUCH1, TOUCH2 are gameplay inputs (read continuously below).
    // TOUCH3 exits back to HOME screen immediately (with debounce).
    if ((now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS) && carrier.Buttons.onTouchDown(TOUCH3)) {
      menuLogic.onButtonBack();
      lastTouchTimeMs = now;
    }
  } else {
    // Normal menu screens (HOME, STATS, ACTION):
    bool inAnyCombo = (touch0 && touch4) || (touch0 && touch1) || combo04Fired || combo01Fired || touch3HoldFired;

    if (currentScreen == SCREEN_HOME && !petLogic.isAlive()) {
      // While Lumi is a ghost/cenotaph:
      // TOUCH2: Egg Hatching & Rebirth
      // TOUCH4: Open Mindelund (Ancestral Memorial)
      if (!inAnyCombo && (now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS)) {
        if (carrier.Buttons.onTouchDown(TOUCH2)) {
          petLogic.startHatching(now);
          menuLogic.setScreen(SCREEN_HATCHING, now);
          soundLed.playSound(SOUND_CRACK);
          Serial.print(F("[LIVSCYKLUS] Aegget begynder at klaekke for Generation: "));
          Serial.println((unsigned int)(petLogic.getGeneration() + 1));
          lastTouchTimeMs = now;
        } else if (carrier.Buttons.onTouchDown(TOUCH4)) {
          menuLogic.openMemorial(storagePet.getGraveRecordCount(), SCREEN_HOME);
          lastTouchTimeMs = now;
        }
      }
    } else if (!inAnyCombo && (now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS)) {
      if (!touch4 && carrier.Buttons.onTouchDown(TOUCH0)) {
        menuLogic.onButtonPrev(); // Previous icon (<)
        lastTouchTimeMs = now;
      }
      else if (carrier.Buttons.onTouchDown(TOUCH1)) {
        menuLogic.onButtonNext(); // Next icon (>)
        lastTouchTimeMs = now;
      }
      else if (carrier.Buttons.onTouchDown(TOUCH2)) {
        if (currentScreen == SCREEN_HOME && menuLogic.getSelectedIcon() == ICON_PLAY) {
          if (petLogic.isSleeping() || petLogic.getEnergy() < 15) {
            menuLogic.setScreen(SCREEN_ACTION, now);
            lastTouchTimeMs = now;
          } else {
            menuLogic.onButtonOk(now); // OK / Select
            lastTouchTimeMs = now;
          }
        } else {
          menuLogic.onButtonOk(now); // OK / Select
          lastTouchTimeMs = now;
        }
      }
      else if (carrier.Buttons.onTouchDown(TOUCH3)) {
        menuLogic.onButtonBack(); // Back to Home
        lastTouchTimeMs = now;
      }
      else if (!touch0 && carrier.Buttons.onTouchDown(TOUCH4)) {
        menuLogic.onButtonStatsShortcut(); // Quick toggle Stats
        lastTouchTimeMs = now;
      }
    }
  }

  // 2. AUDIO LAYER: Play any sound requested by logic
  SoundType pendingSound = menuLogic.popSoundEffect();
  if (pendingSound != SOUND_NONE) {
    soundLed.playSound(pendingSound);
  }

  // 3. LOGIC LAYER: Advance animations and countdown timers
  bool wasAlive = petLogic.isAlive();
  menuLogic.update(now);
  petLogic.update(now);
  soundLed.update(now);

  if (wasAlive && !petLogic.isAlive()) {
    soundLed.playSound(SOUND_DEATH);
    storagePet.logGraveRecord(petLogic);
    storagePet.savePet(petLogic);
    Serial.print(F("[LIVSCYKLUS] Lumi er doed! Aarsag: "));
    Serial.print(petLogic.getDeathCauseName());
    Serial.print(F(" - "));
    Serial.println(petLogic.getDeathEpitaph());
    menuLogic.setScreen(SCREEN_HOME, now);
    uiPet.updateHomeDisplay(petLogic, now, true);
    syncPetStateToBackend(true);
  }

  currentScreen = menuLogic.getActiveScreen();

  // Non-blocking environmental sensor sampling (every 2.5s, paused in Controller and game modes to keep I2C free for IMU)
  if (currentScreen != SCREEN_CONTROLLER && currentScreen != SCREEN_MINIGAME_PONG && currentScreen != SCREEN_MINIGAME_MAZE && (now - lastSensorReadTimeMs >= SENSOR_READ_INTERVAL_MS)) {
    lastSensorReadTimeMs = now;
    float ambientTemp = carrier.Env.readTemperature();
    petLogic.setAmbientTemperature(ambientTemp);

    if (carrier.Light.colorAvailable()) {
      int r = 0, g = 0, b = 0, ambient = 0;
      carrier.Light.readColor(r, g, b, ambient);
      petLogic.setAmbientLight(ambient, now);
    }
  }

  // Real-time hand petting detection via APDS-9960 proximity sensor (every 100 ms on SCREEN_HOME)
  static unsigned long lastProximityCheckMs = 0;
  if (currentScreen == SCREEN_HOME && petLogic.isAlive() && (now - lastProximityCheckMs >= 100)) {
    lastProximityCheckMs = now;
    if (carrier.Light.proximityAvailable()) {
      int prox = carrier.Light.readProximity();
      if (prox >= 0 && prox < 140) {
        if (petLogic.pet(now)) {
          soundLed.playSound(SOUND_PURR);
          soundLed.flashColor(255, 105, 180, 500); // 500ms warm pink cuddle flash
          Serial.println(F("[INTERAKTION] Du aeler og klapper Lumi! (+12% glaede <3)"));
          syncPetStateToBackend(true);
        }
      }
    } else if (carrier.Light.gestureAvailable()) {
      int g = carrier.Light.readGesture();
      if (g != GESTURE_NONE) {
        if (petLogic.pet(now)) {
          soundLed.playSound(SOUND_PURR);
          soundLed.flashColor(255, 105, 180, 500);
          Serial.println(F("[INTERAKTION] Haand-bevaegelse! Lumi spinder af glaede! (+12% <3)"));
          syncPetStateToBackend(true);
        }
      }
    }
  }

  // 4. SCREEN TRANSITIONS (Detect when entering or exiting controller, stats, games, or action mode)
  if (currentScreen != previousScreen) {
    muteMessageEndTimeMs = 0; // Cancel any active mute toast when switching screens

    if (previousScreen == SCREEN_CONTROLLER || previousScreen == SCREEN_CHOOSE_PLAYER || previousScreen == SCREEN_STATS || previousScreen == SCREEN_GAMES_MENU || previousScreen == SCREEN_MINIGAME_PONG || previousScreen == SCREEN_MINIGAME_MAZE || previousScreen == SCREEN_MEMORIAL || previousScreen == SCREEN_HATCHING) {
      // Exiting a fullscreen view: restore mood LEDs and redraw full home UI
      soundLed.setMoodColor(0, 150, 220);
      if (currentScreen == SCREEN_HOME) {
        uiMenu.drawInitialScreen();
        uiMenu.drawMenuBar(menuLogic, true);
      }
    } else if (previousScreen == SCREEN_ACTION) {
      // Exiting temporary center overlay: top menu bar is still intact, only restore LED glow
      soundLed.setMoodColor(0, 150, 220);
    }

    previousScreen = currentScreen;

    switch (currentScreen) {
      case SCREEN_HOME:
        Serial.println(F("[UI] Skærm: HOME (Lumi Tamagotchi)"));
        uiPet.updateHomeDisplay(petLogic, now, true);
        break;

      case SCREEN_STATS:
        Serial.println(F("[UI] Skærm: STATISTIK (5 parametre)"));
        uiStats.updateDisplay(petLogic, true);
        break;

      case SCREEN_HATCHING:
        Serial.println(F("[UI] Skærm: ÆGGEKLÆKNING (Rebirth Cinematic)"));
        break;

      case SCREEN_MEMORIAL:
        Serial.println(F("[UI] Skærm: MINDELUND (Lumis Slægtsbog)"));
        soundLed.setMoodColor(80, 110, 190); // Mystical blue-violet glow
        {
          uint16_t total = storagePet.getGraveRecordCount();
          GraveRecord rec;
          if (total > 0 && storagePet.loadGraveRecord(menuLogic.getMemorialIndex(), rec)) {
            uiMemorial.drawRecord(menuLogic.getMemorialIndex(), total, rec);
          } else {
            uiMemorial.drawEmpty(petLogic.getGeneration());
          }
        }
        break;

      case SCREEN_GAMES_MENU:
        Serial.println(F("[UI] Skærm: MINISPIL MENU (1. Solo Pong, 2. Labyrint)"));
        uiMenu.drawGamesMenu(menuLogic.getSelectedGame(), petLogic.getHighScorePong(), petLogic.getHighScoreMaze());
        soundLed.setMoodColor(0, 160, 220);
        break;

      case SCREEN_MINIGAME_PONG:
        Serial.println(F("[UI] Skærm: SOLO PONG (Mur-Pong) startet!"));
        pongLogic.startNewGame(petLogic.getHighScorePong());
        uiPong.drawInitialScreen(pongLogic);
        soundLed.setMoodColor(0, 220, 255);
        break;

      case SCREEN_MINIGAME_MAZE:
        Serial.println(F("[UI] Skærm: MARMOR-LABYRINT (Procedural IMU Maze) genereret!"));
        mazeLogic.generateMaze((uint32_t)now, petLogic.getHighScoreMaze());
        uiMaze.drawInitialScreen(mazeLogic);
        soundLed.setMoodColor(0, 240, 180);
        break;

      case SCREEN_CHOOSE_PLAYER:
        Serial.println(F("[UI] Skærm: HVEM ER DU? Vælg spillerrolle:"));
        Serial.println(F("     -> TOUCH 0: Spiller 1 (Venstre bat)"));
        Serial.println(F("     -> TOUCH 1 / TOUCH 4: Spiller 2 (Højre bat)"));
        Serial.println(F("     -> TOUCH 3: Tilbage til menu"));
        uiController.drawChoosePlayerScreen();
        break;

      case SCREEN_CONTROLLER:
        Serial.print(F("[UI] Skærm: CONTROLLER aktiv for Spiller "));
        Serial.println(menuLogic.getPlayerId());
        Serial.println(F("[NET] Starter Wi-Fi & MQTT for controller..."));
        netMqtt.setPlayerId(menuLogic.getPlayerId());
        tiltLogic.reset();
        lastCtrlUp = false;
        lastCtrlDown = false;
        lastCtrlAction = false;
        uiController.drawInitialScreen(menuLogic.getPlayerId());
        if (menuLogic.getPlayerId() == 2) {
          soundLed.setMoodColor(255, 70, 50);  // Coral/Red glow for Player 2
        } else {
          soundLed.setMoodColor(0, 180, 255);  // Cyan/Blue glow for Player 1
        }
        break;

      case SCREEN_ACTION:
        Serial.println(F("[UI] Skærm: HANDLING udføres"));
        if (petLogic.isSleeping()) {
          uiPet.drawActionScene(petLogic, EXPR_SLEEPING, "SOVER ZZZ!", "Lumi sover lige nu...", now);
          soundLed.setMoodColor(80, 140, 255); // Soft sleep blue
        } else if (menuLogic.getSelectedIcon() == ICON_FOOD) {
          if (petLogic.feed()) {
            uiPet.drawActionScene(petLogic, EXPR_EATING, "MUMS: MAD!", "Lumi spiser et baer!", now);
            soundLed.setMoodColor(80, 255, 80); // Green glow
          } else {
            uiPet.drawActionScene(petLogic, EXPR_CONTENT, "IKKE SULTEN!", "Lumi er helt maet!", now);
            soundLed.setMoodColor(255, 180, 50); // Orange glow
          }
        } else if (menuLogic.getSelectedIcon() == ICON_PLAY) {
          if (petLogic.play()) {
            uiPet.drawActionScene(petLogic, EXPR_PLAYING, "JUBII: LEG!", "Lumi leger og hopper!", now);
            soundLed.setMoodColor(255, 220, 50); // Gold glow
          } else {
            uiPet.drawActionScene(petLogic, EXPR_EXHAUSTED, "FOR TRAET!", "Lumi mangler energi...", now);
            soundLed.setMoodColor(120, 120, 180);
          }
        } else if (menuLogic.getSelectedIcon() == ICON_MEDICIN) {
          if (!petLogic.isSick()) {
            uiPet.drawActionScene(petLogic, EXPR_CONTENT, "IKKE SYG!", "Lumi har det godt!", now);
            soundLed.setMoodColor(80, 200, 120);
          } else if (petLogic.getBerries() < 3) {
            uiPet.drawActionScene(petLogic, EXPR_SICK, "MANGLER BAER!", "Koster 3 baer i apoteket!", now);
            soundLed.setMoodColor(255, 60, 40);
            soundLed.playSound(SOUND_MISS);
          } else {
            petLogic.cureSickness(3);
            uiPet.drawActionScene(petLogic, EXPR_SUPER_HAPPY, "LUMI RASK!", "-3 baer: Feberen faldt!", now);
            soundLed.playSound(SOUND_HEAL);
            soundLed.setMoodColor(80, 255, 120);
          }
        } else if (menuLogic.getSelectedIcon() == ICON_SCOLD) {
          petLogic.scold();
          uiPet.drawActionScene(petLogic, EXPR_SCOLDED, "SKAELD UD!", "Disciplin stiger!", now);
          soundLed.setMoodColor(255, 60, 40); // Red alert glow
        }
        syncPetStateToBackend(true); // Sync updated stats to backend immediately!
        break;
    }
  }

  // 5. NETWORK & MQTT (Update connection if in Controller mode or already connected)
  if (currentScreen == SCREEN_CONTROLLER || netMqtt.isConnected()) {
    netMqtt.update(now);
  }
  syncPetStateToBackend(false);

  // 6. SCREEN-SPECIFIC RUNTIME LOOPS
  if (currentScreen == SCREEN_CONTROLLER) {
    // 6a. Read accelerometer from IMU (IMUmodule in Carrier v2.x)
    if (carrier.IMUmodule.accelerationAvailable()) {
      float ax = 0.0f, ay = 0.0f, az = 0.0f;
      carrier.IMUmodule.readAcceleration(ax, ay, az);
      tiltLogic.updateAcceleration(ax, ay, az);
    }

    // 6b. Read touch buttons for gameplay
    // TOUCH0 = Up (Pong)
    // TOUCH1 = Down (Pong)
    // TOUCH2 = Action / Serve (Pong & Labyrinth)
    bool btnUp     = carrier.Buttons.getTouch(TOUCH0);
    bool btnDown   = carrier.Buttons.getTouch(TOUCH1);
    bool btnAction = carrier.Buttons.getTouch(TOUCH2);

    // Detect immediate edge change (pressed or released) to eliminate 50ms tap latency
    bool buttonEdge = (btnUp != lastCtrlUp) || (btnDown != lastCtrlDown) || (btnAction != lastCtrlAction);
    lastCtrlUp     = btnUp;
    lastCtrlDown   = btnDown;
    lastCtrlAction = btnAction;

    tiltLogic.setButtons(btnUp, btnDown, btnAction);

    // 6c. Publish controller input at 20 Hz or on button edge:
    if (netMqtt.canPublish(now, buttonEdge)) {
      char jsonBuffer[96];
      tiltLogic.formatInputJson(jsonBuffer, sizeof(jsonBuffer));
      netMqtt.publishInput(jsonBuffer, now, buttonEdge);
    }

    // 6d. Real-time controller display update (smooth 2D tilt arena + buttons + connection badge)
    uiController.updateDisplay(tiltLogic, netMqtt.isConnected());
  } else if (currentScreen == SCREEN_MINIGAME_PONG) {
    // 6a. Read accelerometer for paddle tilt
    if (carrier.IMUmodule.accelerationAvailable()) {
      float ax = 0.0f, ay = 0.0f, az = 0.0f;
      carrier.IMUmodule.readAcceleration(ax, ay, az);
      tiltLogic.updateAcceleration(ax, ay, az);
      pongLogic.setPaddleTilt(tiltLogic.getTiltY());
    }

    // 6b. Step physics
    pongLogic.update(now);

    // 6c. Handle sound & LEDs
    SoundType pongSnd = pongLogic.popSoundEffect();
    if (pongSnd != SOUND_NONE) {
      soundLed.playSound(pongSnd);
    }
    if (pongLogic.popPaddleHit()) {
      soundLed.flashColor(0, 255, 220, 100); // 100 ms cyan flash on paddle hit
    }

    // 6d. Update display (zero-flicker delta-rendering)
    uiPong.updateDisplay(pongLogic, false);
  } else if (currentScreen == SCREEN_MINIGAME_MAZE) {
    // 6a. Read 2D accelerometer for marble tilt physics
    if (carrier.IMUmodule.accelerationAvailable()) {
      float ax = 0.0f, ay = 0.0f, az = 0.0f;
      carrier.IMUmodule.readAcceleration(ax, ay, az);
      tiltLogic.updateAcceleration(ax, ay, az);
      mazeLogic.setTilt(tiltLogic.getTiltX(), tiltLogic.getTiltY());
    }

    // 6b. Step physics simulation
    mazeLogic.update(now);

    // 6c. Handle audio effects and LED sparkles
    SoundType mazeSnd = mazeLogic.popSoundEffect();
    if (mazeSnd != SOUND_NONE) {
      soundLed.playSound(mazeSnd);
    }
    if (mazeLogic.popBerryCollected()) {
      soundLed.flashColor(255, 220, 60, 200); // 200 ms golden sparkle on berry pickup
    }
    if (mazeLogic.popPitfallFall()) {
      soundLed.flashColor(180, 40, 220, 300); // 300 ms purple vortex splash
    }

    // 6d. Update display (zero-flicker delta-rendering)
    uiMaze.updateDisplay(mazeLogic, false);
  } else if (currentScreen == SCREEN_STATS) {
    // Real-time stats dashboard update (dirty updates only when values decay or change)
    uiStats.updateDisplay(petLogic, false);
  } else if (currentScreen == SCREEN_HATCHING) {
    uint8_t phase = petLogic.getHatchingPhase(now);
    uiPet.drawHatchingScene(phase, now, petLogic.getGeneration() + 1);

    static uint8_t lastHatchPhase = 0;
    if (phase != lastHatchPhase) {
      if (phase == 1) {
        soundLed.playSound(SOUND_CRACK);
      } else if (phase == 2) {
        soundLed.playSound(SOUND_REBIRTH);
      }
      lastHatchPhase = phase;
    }

    if (!petLogic.isHatching()) {
      lastHatchPhase = 0;
      storagePet.savePet(petLogic);
      syncPetStateToBackend(true);
      Serial.print(F("[LIVSCYKLUS] Lumi genfoedt! Ny generation: "));
      Serial.println((unsigned int)petLogic.getGeneration());
      menuLogic.setScreen(SCREEN_HOME, now);
      uiMenu.drawInitialScreen();
      uiMenu.drawMenuBar(menuLogic, true);
      uiPet.updateHomeDisplay(petLogic, now, true);
    }
  } else if (currentScreen != SCREEN_CHOOSE_PLAYER && currentScreen != SCREEN_GAMES_MENU && currentScreen != SCREEN_MEMORIAL) {
    // Normal menu modes (HOME, ACTION): update sliding top menu bar (hidden when Lumi is a ghost on HOME)
    if (petLogic.isAlive() || currentScreen != SCREEN_HOME) {
      uiMenu.drawMenuBar(menuLogic, false);
    }

    // Real-time 2.5D Lumi character animation on HOME screen (breathing, blinking, Navi, shivering, ghost cenotaph)
    if (currentScreen == SCREEN_HOME && muteMessageEndTimeMs == 0) {
      // Shake detection via accelerometer to wash away poop with foamy soap bubbles:
      if (carrier.IMUmodule.accelerationAvailable()) {
        float ax = 0.0f, ay = 0.0f, az = 0.0f;
        carrier.IMUmodule.readAcceleration(ax, ay, az);
        static float lastShakeAx = 0.0f, lastShakeAy = 0.0f, lastShakeAz = 1.0f;
        static unsigned long lastShakeCheckMs = 0;
        if (now - lastShakeCheckMs >= 40) {
          lastShakeCheckMs = now;
          float jerk = fabs(ax - lastShakeAx) + fabs(ay - lastShakeAy) + fabs(az - lastShakeAz);
          lastShakeAx = ax;
          lastShakeAy = ay;
          lastShakeAz = az;
          static unsigned long lastBathTriggerMs = 0;
          if (jerk > 2.0f && petLogic.isAlive() && !petLogic.isSleeping() && (now - lastBathTriggerMs > 2000)) {
            lastBathTriggerMs = now;
            petLogic.cleanPoop();
            uiPet.triggerBath(now);
            soundLed.playSound(SOUND_BUBBLES);
            soundLed.flashColor(0, 220, 255, 600); // 600 ms aqua-foam glow
            Serial.println(F("[RYST-BEVÆGELSE] Du rystede Oplà-kortet! Lumi bades i sæbeskum!"));
            syncPetStateToBackend(true);
          }
        }
      }

      uiPet.updateHomeDisplay(petLogic, now, false);
    }
  }

  // 7. Non-blocking FPS diagnostics (printed to Serial once per second)
  frameCount++;
  if (now - lastFpsTimeMs >= 1000) {
    // Optional: Serial.print("FPS: "); Serial.println(frameCount);
    frameCount = 0;
    lastFpsTimeMs = now;
  }
}

