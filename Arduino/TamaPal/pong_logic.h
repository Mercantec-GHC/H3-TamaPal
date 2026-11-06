// WHAT: Pure business logic for the Solo Wall-Pong minigame on Arduino Oplà
// WHY:  Separated from hardware and rendering so 100% of game physics can be unit-tested on Mac
// USED BY: TamaPal.ino, ui_pong.cpp, tests/test_main.cpp

#ifndef PONG_LOGIC_H
#define PONG_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "settings.h"

// Game states
enum PongState : uint8_t {
  PONG_STATE_READY = 0,     // Ready to serve, ball held or waiting for start
  PONG_STATE_PLAYING,       // Active gameplay
  PONG_STATE_BALL_LOST,     // Ball went past paddle; short delay before next serve
  PONG_STATE_GAMEOVER       // All lives depleted
};

class PongLogic {
public:
  // Field layout constants (tuned for 240x240 circular ST7789 display)
  static constexpr int16_t FIELD_LEFT   = 28;
  static constexpr int16_t FIELD_RIGHT  = 214;
  static constexpr int16_t FIELD_TOP    = 38;
  static constexpr int16_t FIELD_BOTTOM = 202;

  // Paddle constants
  static constexpr int16_t PADDLE_X      = 32;
  static constexpr int16_t PADDLE_WIDTH  = 5;
  static constexpr int16_t PADDLE_HEIGHT = 34;

  // Ball constants
  static constexpr int16_t BALL_SIZE = 5;
  static constexpr unsigned long RESPAWN_DELAY_MS = 2000;

  PongLogic();

  // Reset / Start a new game session
  void startNewGame(uint16_t existingHighScore = 0);

  // Manual serve / start when in READY or BALL_LOST state
  void serveBall();

  // Update paddle position via IMU tilt (-1.0f .. +1.0f) or direct delta
  void setPaddleTilt(float tiltY);
  void movePaddle(float deltaY);
  void setPaddleY(float y);

  // Physics simulation step
  void update(unsigned long currentTimeMs);

  // State inspection (getters)
  PongState getState() const;
  float getPaddleY() const;
  float getBallX() const;
  float getBallY() const;
  float getBallVx() const;
  float getBallVy() const;
  uint16_t getScore() const;
  uint16_t getHighScore() const;
  uint8_t getLives() const;
  bool isNewHighScore() const;

  // Transient event pop flags
  SoundType popSoundEffect();
  bool popPaddleHit();
  bool popWallHit();

private:
  PongState state;
  float paddleY;
  float ballX;
  float ballY;
  float ballVx;
  float ballVy;

  uint16_t score;
  uint16_t highScore;
  uint8_t lives;
  bool newHighScore;

  unsigned long lastUpdateMs;
  unsigned long stateTimerEndMs;

  SoundType pendingSound;
  bool paddleHitFlag;
  bool wallHitFlag;

  void resetBall();
  void clampPaddle();
};

#endif // PONG_LOGIC_H

