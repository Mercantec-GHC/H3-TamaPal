// WHAT: Global configuration constants and enum types for TamaPal
// WHY:  Keep all magic numbers, timings, and settings in one single place
// USED BY: All logic, UI, and hardware modules in TamaPal

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

// ============================================================================
// DISPLAY & SCREEN GEOMETRY (Round 240x240 ST7789 TFT)
// ============================================================================
// Screen width and height in pixels
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;

// Top menu horizontal bar settings
// Spacing 30 px ensures the 5 icons span X=48..192 (144 px total), matching the circular cutout perfectly!
const int MENU_BAR_CENTER_Y = 36;
const int MENU_ICON_SIZE = 24;         // Width & height of each icon in pixels
const int MENU_ICON_SPACING = 30;      // Distance between centers of neighboring icons (Centers: 60, 90, 120, 150, 180)
const int MENU_TOTAL_WIDTH = 144;      // Fits safely within the round bezel

// Sliding frame animation speed (Lerp factor: 0.25 = 25% closer to target each frame)
const float MENU_SLIDE_SPEED = 0.25f;

// Duration of action confirmation overlay (e.g. "Mad!", "Leg!") in milliseconds
const unsigned long ACTION_MESSAGE_DURATION_MS = 1500;

// ============================================================================
// TOUCH SENSORS & AUDIO SETTINGS
// ============================================================================
// Set to false when using bare board (carrier.noCase()), true if inside plastic box
const bool USE_PLASTIC_CASE = false;

// Capacitive touch threshold (8-bit: 1..254).
// Higher value = LESS sensitive (requires firm finger press, ignores hovering finger).
// IMPORTANT: Must be set BEFORE carrier.begin()!
const int TOUCH_SENSITIVITY_THRESHOLD = 250;

// Minimum pause between button triggers in milliseconds (prevents accidental double-taps)
const unsigned long TOUCH_COOLDOWN_MS = 180;

// Start with sound enabled (can be toggled at any time by pressing TOUCH0 + TOUCH4 together)
const bool START_WITH_SOUND_MUTED = false;

// ============================================================================
// CONTROLLER & MQTT SETTINGS (Pong and Minigames)
// ============================================================================
// Maximum tilt angle in degrees for full deflection (30 degrees = 1.0 or -1.0)
const float MAX_TILT_DEG = 30.0f;

// Deadzone around center: ignore tilt angles below 5% to avoid hand jitters
const float TILT_DEADZONE = 0.05f;

// Exponential moving average factor: 30% new sensor data + 70% smoothed history
const float TILT_SMOOTH_FACTOR = 0.30f;

// MQTT publish frequency: 50 ms = 20 messages per second (20 Hz)
const unsigned long MQTT_PUBLISH_INTERVAL_MS = 50;

// Reconnect interval if Wi-Fi or MQTT broker drops (5 seconds)
const unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000;

// MQTT Client ID and Topics (as defined in docs/MESSAGES.md contract)
#define MQTT_CLIENT_ID "tamapal-opla-1"
#define MQTT_TOPIC_INPUT "tamapal/opla-1/input"
#define MQTT_TOPIC_EVENT "tamapal/opla-1/event"

// ============================================================================
// ENUMS (Application states and menu items)
// ============================================================================

// Which screen is currently visible to the player
enum ScreenType {
  SCREEN_HOME = 0,         // Main screen: Lumi character + top menu bar
  SCREEN_STATS,            // Fullscreen statistics dashboard (5 vital bars)
  SCREEN_CHOOSE_PLAYER,    // Select Player 1 (Venstre) vs Player 2 (Hoejre)
  SCREEN_CONTROLLER,       // Pong controller mode (sends tilt & button to backend)
  SCREEN_ACTION            // Temporary action banner (e.g., feeding food animation)
};

// The 5 icons in the top navigation menu
enum MenuIcon {
  ICON_FOOD = 0,           // 0: Mad (Feed berry to Lumi)
  ICON_PLAY,               // 1: Leg (Play minigame / pet)
  ICON_SCOLD,              // 2: Skaeld ud (Discipline when complaining)
  ICON_STATS,              // 3: Statistik (Open 5 vitals dashboard)
  ICON_CONNECT,            // 4: Forbind (Connect to Pong multiplayer)
  ICON_TOTAL_COUNT         // Helper constant = 5 total icons
};

// Hardware touch buttons on the Arduino Oplà MKR Carrier
enum TouchPad {
  TOUCH_PAD_PREV = 0,      // TOUCH0: Move menu selection left (<)
  TOUCH_PAD_NEXT = 1,      // TOUCH1: Move menu selection right (>)
  TOUCH_PAD_OK = 2,        // TOUCH2: Confirm / select action (in Pong: serve ball)
  TOUCH_PAD_BACK = 3,      // TOUCH3: Back / cancel / return to HOME
  TOUCH_PAD_STATS = 4      // TOUCH4: Direct shortcut to statistics screen
};

// Sound effect types for buzzer
enum SoundType {
  SOUND_NONE = 0,
  SOUND_CLICK,             // Soft click when browsing icons (20 ms)
  SOUND_CONFIRM,           // Cheerful 2-tone chirp on OK selection
  SOUND_BACK,              // Low tone when returning back
  SOUND_HIT,               // High ping when hitting ball in Pong (1200 Hz, 80 ms)
  SOUND_MISS,              // Low buzz when missing ball (220 Hz, 150 ms)
  SOUND_GAMEOVER           // 3-note descending game over jingle
};

// Events received from backend over MQTT (tamapal/opla-1/event)
enum GameEvent {
  EVENT_NONE = 0,
  EVENT_HIT,               // Paddle hit the ball (+1 point)
  EVENT_MISS,              // Missed ball (-1 life)
  EVENT_GAMEOVER           // Game finished (0 lives)
};

#endif // SETTINGS_H

