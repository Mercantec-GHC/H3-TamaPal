// WHAT: Implementation of Solo Wall-Pong game physics and state transitions
// WHY:  Zero dependencies on Arduino hardware for cross-platform unit testing
// USED BY: TamaPal.ino, ui_pong.cpp, tests/test_main.cpp

#include "pong_logic.h"
#include <math.h>

PongLogic::PongLogic()
  : state(PONG_STATE_READY),
    paddleY((FIELD_TOP + FIELD_BOTTOM - PADDLE_HEIGHT) * 0.5f),
    ballX((FIELD_LEFT + FIELD_RIGHT) * 0.5f),
    ballY((FIELD_TOP + FIELD_BOTTOM) * 0.5f),
    ballVx(2.4f),
    ballVy(1.0f),
    score(0),
    highScore(0),
    lives(3),
    newHighScore(false),
    lastUpdateMs(0),
    stateTimerEndMs(0),
    pendingSound(SOUND_NONE),
    paddleHitFlag(false),
    wallHitFlag(false) {}

void PongLogic::startNewGame(uint16_t existingHighScore) {
  score = 0;
  lives = 3;
  highScore = existingHighScore;
  newHighScore = false;
  paddleY = (FIELD_TOP + FIELD_BOTTOM - PADDLE_HEIGHT) * 0.5f;
  resetBall();
  state = PONG_STATE_READY;
  lastUpdateMs = 0;
  stateTimerEndMs = 0;
  pendingSound = SOUND_CONFIRM;
  paddleHitFlag = false;
  wallHitFlag = false;
}

void PongLogic::resetBall() {
  ballX = PADDLE_X + PADDLE_WIDTH + 6;
  ballY = paddleY + (PADDLE_HEIGHT - BALL_SIZE) * 0.5f;
  ballVx = 2.4f;
  ballVy = 1.0f;
}

void PongLogic::serveBall() {
  if (state == PONG_STATE_READY || state == PONG_STATE_BALL_LOST) {
    state = PONG_STATE_PLAYING;
    pendingSound = SOUND_CLICK;
  }
}

void PongLogic::setPaddleTilt(float tiltY) {
  if (state == PONG_STATE_GAMEOVER) return;
  // Deadzone so flat board on table does not fight touch buttons
  if (fabs(tiltY) < 0.12f) return;
  // Positive tiltY = move up (negative Y), negative tiltY = move down (positive Y)
  movePaddle(-tiltY * 3.8f);
}

void PongLogic::movePaddle(float deltaY) {
  paddleY += deltaY;
  clampPaddle();
}

void PongLogic::clampPaddle() {
  const float minY = (float)(FIELD_TOP + 2);
  const float maxY = (float)(FIELD_BOTTOM - PADDLE_HEIGHT - 2);
  if (paddleY < minY) paddleY = minY;
  if (paddleY > maxY) paddleY = maxY;
}

void PongLogic::update(unsigned long currentTimeMs) {
  unsigned long dtMs = (lastUpdateMs == 0) ? 20 : (currentTimeMs - lastUpdateMs);
  lastUpdateMs = currentTimeMs;

  if (dtMs > 100) dtMs = 100;
  if (dtMs < 5) dtMs = 5;

  float speedFactor = (float)dtMs / 20.0f;

  if (state == PONG_STATE_READY) {
    // Ball tracks the paddle center until served
    ballX = (float)(PADDLE_X + PADDLE_WIDTH + 4);
    ballY = paddleY + (PADDLE_HEIGHT - BALL_SIZE) * 0.5f;
    return;
  }

  if (state == PONG_STATE_BALL_LOST) {
    if (stateTimerEndMs != 0 && currentTimeMs >= stateTimerEndMs) {
      resetBall();
      state = PONG_STATE_PLAYING;
      stateTimerEndMs = 0;
    }
    return;
  }

  if (state == PONG_STATE_GAMEOVER) {
    return;
  }

  // Active gameplay simulation with swept collision tracking
  float prevBallX = ballX;
  ballX += ballVx * speedFactor;
  ballY += ballVy * speedFactor;

  // 1. Top & Bottom wall bounce (keep 1px inside border lines)
  const float minBallY = (float)(FIELD_TOP + 1);
  const float maxBallY = (float)(FIELD_BOTTOM - BALL_SIZE - 1);
  if (ballY <= minBallY) {
    ballY = minBallY;
    ballVy = fabs(ballVy);
    wallHitFlag = true;
    pendingSound = SOUND_CLICK;
  } else if (ballY >= maxBallY) {
    ballY = maxBallY;
    ballVy = -fabs(ballVy);
    wallHitFlag = true;
    pendingSound = SOUND_CLICK;
  }

  // 2. Right wall collision (keep 1px left of right wall at FIELD_RIGHT - 2)
  const float maxBallX = (float)(FIELD_RIGHT - 2 - BALL_SIZE);
  if (ballX >= maxBallX) {
    ballX = maxBallX;
    ballVx = -fabs(ballVx);
    wallHitFlag = true;
    pendingSound = SOUND_CLICK;
  }

  // 3. Paddle collision (Left paddle - swept check using prevBallX to prevent high-speed tunneling)
  const float paddleRight = (float)(PADDLE_X + PADDLE_WIDTH);
  if (ballVx < 0.0f && ballX <= paddleRight && (prevBallX + BALL_SIZE) >= (float)PADDLE_X && prevBallX >= (float)PADDLE_X) {
    if ((ballY + BALL_SIZE) >= paddleY && ballY <= (paddleY + PADDLE_HEIGHT)) {
      ballX = (float)(PADDLE_X + PADDLE_WIDTH);

      // Compute impact point relative to paddle center (-1.0 top to +1.0 bottom)
      float paddleCenter = paddleY + PADDLE_HEIGHT * 0.5f;
      float ballCenter   = ballY + BALL_SIZE * 0.5f;
      float relHit       = (ballCenter - paddleCenter) / (PADDLE_HEIGHT * 0.5f);

      if (relHit < -1.0f) relHit = -1.0f;
      if (relHit > 1.0f)  relHit = 1.0f;

      // Increase speed slightly with each hit (cap at 5.5f)
      float currentSpeed = -ballVx * 1.05f;
      if (currentSpeed > 5.5f) currentSpeed = 5.5f;
      ballVx = currentSpeed;

      // Vertical spin/angle based on hit point
      ballVy = relHit * 2.8f;
      if (ballVy > -0.3f && ballVy < 0.3f) {
        ballVy = (relHit >= 0.0f) ? 0.6f : -0.6f;
      }

      score++;
      paddleHitFlag = true;
      pendingSound = SOUND_HIT;

      if (score > highScore) {
        highScore = score;
        newHighScore = true;
      }
    }
  }

  // 5. Left boundary miss (Ball passed behind paddle)
  if (ballX < (float)FIELD_LEFT) {
    if (lives > 0) {
      lives--;
    }
    pendingSound = SOUND_MISS;

    if (lives > 0) {
      state = PONG_STATE_BALL_LOST;
      stateTimerEndMs = currentTimeMs + 800; // 800 ms respawn delay
    } else {
      state = PONG_STATE_GAMEOVER;
      pendingSound = SOUND_GAMEOVER;
    }
  }
}

PongState PongLogic::getState() const {
  return state;
}

float PongLogic::getPaddleY() const {
  return paddleY;
}

float PongLogic::getBallX() const {
  return ballX;
}

float PongLogic::getBallY() const {
  return ballY;
}

float PongLogic::getBallVx() const {
  return ballVx;
}

float PongLogic::getBallVy() const {
  return ballVy;
}

uint16_t PongLogic::getScore() const {
  return score;
}

uint16_t PongLogic::getHighScore() const {
  return highScore;
}

uint8_t PongLogic::getLives() const {
  return lives;
}

bool PongLogic::isNewHighScore() const {
  return newHighScore;
}

SoundType PongLogic::popSoundEffect() {
  SoundType s = pendingSound;
  pendingSound = SOUND_NONE;
  return s;
}

bool PongLogic::popPaddleHit() {
  bool hit = paddleHitFlag;
  paddleHitFlag = false;
  return hit;
}

bool PongLogic::popWallHit() {
  bool hit = wallHitFlag;
  wallHitFlag = false;
  return hit;
}

