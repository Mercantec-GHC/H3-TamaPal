// WHAT: Main Arduino entrypoint sketch for TamaPal (setup & non-blocking game loop)
// WHY:  Orchestrates inputs, pure logic, sound effects, and UI rendering cleanly
// USED BY: Arduino MKR WiFi 1010 + MKR IoT Carrier

#include <Arduino_MKRIoTCarrier.h>
#include "settings.h"
#include "menu_logic.h"
#include "ui_menu.h"
#include "sound_led.h"
#include "tilt_logic.h"
#include "ui_controller.h"
#include "net_mqtt.h"

// Hardware carrier instance
MKRIoTCarrier carrier;

// Software layers (Clean Architecture: Logic -> UI -> Hardware)
MenuLogic menuLogic;
UiMenuRenderer uiMenu;
SoundLedController soundLed;
TiltLogic tiltLogic;
UiControllerRenderer uiController;
NetMqttClient netMqtt;

// Forward declaration of incoming game event callback (hit, miss, gameover)
void onGameEventReceived(GameEvent event) {
  soundLed.playEventFeedback(event);
}

// Track screen transitions to redraw viewports only when needed
ScreenType previousScreen = (ScreenType)99;

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

void setup() {
  Serial.begin(115200);

  // IMPORTANT: Set custom touch threshold BEFORE carrier.begin()!
  // Inside MKRIoTCarrier library, touch_sensors_config() runs ONLY during begin(),
  // and carrier.withCase() ensures begin() does not overwrite our custom threshold with 200.
  carrier.withCase();
  carrier.Buttons.updateConfig(TOUCH_SENSITIVITY_THRESHOLD);
  carrier.begin();
  if (!USE_PLASTIC_CASE) {
    carrier.noCase();
  }

  // Set LCD rotation (0 = standard orientation)
  carrier.display.setRotation(0);

  // Initialize UI and audio controllers
  soundLed.begin(&carrier);
  uiMenu.begin(&carrier);
  uiController.begin(&carrier);
  netMqtt.begin(onGameEventReceived);

  // Initial full screen draw
  uiMenu.drawInitialScreen();
  uiMenu.drawMenuBar(menuLogic, true);
  uiMenu.drawPlaceholderScreen("LUMI HOME", "Vaelg et ikon i toppen");

  Serial.println("TamaPal ready! Navigation menu running.");
}

void loop() {
  unsigned long now = millis();

  // 1. INPUT LAYER: Poll capacitive touch pads on the carrier
  carrier.Buttons.update();

  bool touch0 = carrier.Buttons.getTouch(TOUCH0);
  bool touch3 = carrier.Buttons.getTouch(TOUCH3);
  bool touch4 = carrier.Buttons.getTouch(TOUCH4);

  ScreenType currentScreen = menuLogic.getActiveScreen();

  // -------------------------------------------------------------
  // GESTURE 1: TOUCH0 + TOUCH4 combo (Hold both for 300 ms)
  // -------------------------------------------------------------
  if (touch0 && touch4) {
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

  // -------------------------------------------------------------
  // GESTURE 2: Hold TOUCH3 alone for 1.2 seconds on HOME screen
  // -------------------------------------------------------------
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

  // -------------------------------------------------------------
  // Restore screen automatically when mute banner notification expires
  // -------------------------------------------------------------
  if (muteMessageEndTimeMs > 0 && now >= muteMessageEndTimeMs) {
    muteMessageEndTimeMs = 0;
    if (currentScreen == SCREEN_HOME) {
      uiMenu.drawPlaceholderScreen("LUMI HOME", "Tryk T2 for at vaelge");
    }
  }

  // -------------------------------------------------------------
  // Button Navigation
  // -------------------------------------------------------------
  if (currentScreen == SCREEN_CONTROLLER) {
    // In controller mode:
    // TOUCH0, TOUCH1, TOUCH2 are gameplay inputs (read continuously below).
    // TOUCH3 exits back to HOME screen immediately.
    if (carrier.Buttons.onTouchDown(TOUCH3)) {
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
  soundLed.update(now);

  currentScreen = menuLogic.getActiveScreen();

  // 4. SCREEN TRANSITIONS (Detect when entering or exiting controller mode)
  if (currentScreen != previousScreen) {
    if (previousScreen == SCREEN_CONTROLLER) {
      // Exiting controller: disconnect MQTT and redraw main menu
      netMqtt.disconnect();
      uiMenu.drawInitialScreen();
      uiMenu.drawMenuBar(menuLogic, true);
    }

    previousScreen = currentScreen;

    switch (currentScreen) {
      case SCREEN_HOME:
        uiMenu.drawPlaceholderScreen("LUMI HOME", "Tryk T2 for at vaelge");
        break;

      case SCREEN_STATS:
        uiMenu.drawPlaceholderScreen("STATISTIK", "T3 / T4 = Tilbage");
        break;

      case SCREEN_CONTROLLER:
        tiltLogic.reset();
        uiController.drawInitialScreen();
        break;

      case SCREEN_ACTION:
        // Show temporary action banner for 1.5 seconds
        if (menuLogic.getSelectedIcon() == ICON_FOOD) {
          uiMenu.drawPlaceholderScreen("MUMS: MAD!", "Giver Lumi en baer...");
          soundLed.setMoodColor(80, 255, 80); // Green glow
        } else if (menuLogic.getSelectedIcon() == ICON_PLAY) {
          uiMenu.drawPlaceholderScreen("JUBII: LEG!", "Lumi leger og hopper!");
          soundLed.setMoodColor(255, 220, 50); // Gold glow
        } else if (menuLogic.getSelectedIcon() == ICON_SCOLD) {
          uiMenu.drawPlaceholderScreen("SKAELD UD!", "Disciplin stiger!");
          soundLed.setMoodColor(255, 60, 40); // Red alert glow
        }
        break;
    }
  }

  // 5. SCREEN-SPECIFIC RUNTIME LOOPS
  if (currentScreen == SCREEN_CONTROLLER) {
    // 5a. Read accelerometer from IMU (IMUmodule in Carrier v2.x)
    if (carrier.IMUmodule.accelerationAvailable()) {
      float ax = 0.0f, ay = 0.0f, az = 0.0f;
      carrier.IMUmodule.readAcceleration(ax, ay, az);
      tiltLogic.updateAcceleration(ax, ay, az);
    }

    // 5b. Read touch buttons for gameplay
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

    // 5c. Service MQTT network and publish input:
    // - Immediately on button state change (< 1 ms latency!)
    // - Plus 20 Hz periodic heartbeat (~50 ms) for smooth 2D tilt & keep-alive
    netMqtt.update(now);
    char jsonBuffer[96];
    tiltLogic.formatInputJson(jsonBuffer, sizeof(jsonBuffer));
    netMqtt.publishInput(jsonBuffer, now, buttonEdge);

    // 5d. Real-time controller display update (smooth 2D tilt arena + buttons + connection badge)
    uiController.updateDisplay(tiltLogic, netMqtt.isConnected());
  } else {
    // Normal menu modes: update sliding top menu bar
    uiMenu.drawMenuBar(menuLogic, false);
  }

  // 6. Non-blocking FPS diagnostics (printed to Serial once per second)
  frameCount++;
  if (now - lastFpsTimeMs >= 1000) {
    // Optional: Serial.print("FPS: "); Serial.println(frameCount);
    frameCount = 0;
    lastFpsTimeMs = now;
  }
}

