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

  // 1. Save to local MicroSD card (persists even if Wi-Fi / MQTT is offline!)
  storagePet.savePet(petLogic);

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
  Serial.println(F("Naviger med TOUCH 0 (<) og TOUCH 1 (>)"));
  Serial.println(F("For at forbinde til Pong over MQTT:"));
  Serial.println(F(" 1. Gaa til CONNECT (antenne-ikonet helt til hoejre)"));
  Serial.println(F(" 2. Tryk TOUCH 2 (OK)"));
  Serial.println(F(" 3. Vaelg spiller: TOUCH 0 (Spiller 1) eller TOUCH 1 / TOUCH 4 (Spiller 2)"));
  Serial.println(F("=================================================="));
}

void loop() {
  unsigned long now = millis();

  // Listen for 'c' in Serial Monitor to toggle plastic case mode on-the-fly (non-blocking, no String)
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == 'c' || c == 'C') {
      setCaseMode(!currentCaseMode, true);
    }
  }

  // 1. INPUT LAYER: Poll capacitive touch pads on the carrier
  carrier.Buttons.update();

  bool touch0 = carrier.Buttons.getTouch(TOUCH0);
  bool touch3 = carrier.Buttons.getTouch(TOUCH3);
  bool touch4 = carrier.Buttons.getTouch(TOUCH4);

  ScreenType currentScreen = menuLogic.getActiveScreen();

  // Process Mute gestures (TOUCH0+TOUCH4 combo or hold TOUCH3)
  processMuteGestures(now, touch0, touch3, touch4, currentScreen);

  // -------------------------------------------------------------
  // Restore screen automatically when mute banner notification expires
  // -------------------------------------------------------------
  if (muteMessageEndTimeMs > 0 && now >= muteMessageEndTimeMs) {
    muteMessageEndTimeMs = 0;
    if (currentScreen == SCREEN_HOME) {
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
    bool inAnyCombo = (touch0 && touch4) || combo04Fired || touch3HoldFired;

    if (!inAnyCombo && (now - lastTouchTimeMs >= TOUCH_COOLDOWN_MS)) {
      if (!touch4 && carrier.Buttons.onTouchDown(TOUCH0)) {
        menuLogic.onButtonPrev(); // Previous icon (<)
        lastTouchTimeMs = now;
      }
      else if (carrier.Buttons.onTouchDown(TOUCH1)) {
        menuLogic.onButtonNext(); // Next icon (>)
        lastTouchTimeMs = now;
      }
      else if (carrier.Buttons.onTouchDown(TOUCH2)) {
        menuLogic.onButtonOk(now); // OK / Select
        lastTouchTimeMs = now;
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
  menuLogic.update(now);
  petLogic.update(now);
  soundLed.update(now);

  currentScreen = menuLogic.getActiveScreen();

  // Non-blocking environmental sensor sampling (every 2.5s, paused in Controller mode to keep I2C free for IMU)
  if (currentScreen != SCREEN_CONTROLLER && (now - lastSensorReadTimeMs >= SENSOR_READ_INTERVAL_MS)) {
    lastSensorReadTimeMs = now;
    float ambientTemp = carrier.Env.readTemperature();
    petLogic.setAmbientTemperature(ambientTemp);

    if (carrier.Light.colorAvailable()) {
      int r = 0, g = 0, b = 0, ambient = 0;
      carrier.Light.readColor(r, g, b, ambient);
      petLogic.setAmbientLight(ambient, now);
    }
  }

  // 4. SCREEN TRANSITIONS (Detect when entering or exiting controller, stats, or action mode)
  if (currentScreen != previousScreen) {
    muteMessageEndTimeMs = 0; // Cancel any active mute toast when switching screens

    if (previousScreen == SCREEN_CONTROLLER || previousScreen == SCREEN_CHOOSE_PLAYER || previousScreen == SCREEN_STATS) {
      // Exiting a fullscreen view: restore mood LEDs and redraw full home UI
      soundLed.setMoodColor(0, 150, 220);
      uiMenu.drawInitialScreen();
      uiMenu.drawMenuBar(menuLogic, true);
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
        } else if (menuLogic.getSelectedIcon() == ICON_SCOLD) {
          petLogic.scold();
          uiPet.drawActionScene(petLogic, EXPR_SCOLDED, "SKAELD UD!", "Disciplin stiger!", now);
          soundLed.setMoodColor(255, 60, 40); // Red alert glow
        }
        syncPetStateToBackend(true); // Sync updated stats to backend immediately!
        break;
    }
  }

  // 5. NETWORK & MQTT (Keep connection alive and sync Lumi's 5 vitals every 30s)
  netMqtt.update(now);
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
  } else if (currentScreen == SCREEN_STATS) {
    // Real-time stats dashboard update (dirty updates only when values decay or change)
    uiStats.updateDisplay(petLogic, false);
  } else if (currentScreen != SCREEN_CHOOSE_PLAYER) {
    // Normal menu modes (HOME, ACTION): update sliding top menu bar
    uiMenu.drawMenuBar(menuLogic, false);

    // Real-time 2.5D Lumi character animation on HOME screen (breathing, blinking, Navi, shivering)
    if (currentScreen == SCREEN_HOME && muteMessageEndTimeMs == 0) {
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

