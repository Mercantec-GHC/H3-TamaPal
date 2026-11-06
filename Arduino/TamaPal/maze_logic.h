// WHAT: Pure business logic for the procedural Marble Maze minigame on Arduino Oplà
// WHY:  Separated from hardware and graphics so 100% of generation and physics can be unit-tested on Mac
// USED BY: TamaPal.ino, ui_maze.cpp, tests/test_main.cpp

#ifndef MAZE_LOGIC_H
#define MAZE_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "settings.h"

// Maze dimensions (7x7 fits perfectly inside a 168x168 px bounding box on a 240x240 circular screen)
#define MAZE_COLS 7
#define MAZE_ROWS 7

// Cell wall bitflags (1 = wall present, 0 = open passage)
#define MAZE_WALL_NORTH  0x01
#define MAZE_WALL_EAST   0x02
#define MAZE_WALL_SOUTH  0x04
#define MAZE_WALL_WEST   0x08
#define MAZE_VISITED     0x10

// Cell item bitflags
#define MAZE_ITEM_MASK     0xE0
#define MAZE_ITEM_NONE     0x00
#define MAZE_ITEM_BERRY    0x20  // Glowing Deku Berry (+bonus score & energy)
#define MAZE_ITEM_PITFALL  0x40  // Dark pit hole (respawns ball, +5s penalty)
#define MAZE_ITEM_ICE      0x80  // Slippery ice patch (zero friction speed-up)

// Game states
enum MazeState : uint8_t {
  MAZE_STATE_READY = 0,    // Generated, ready to start
  MAZE_STATE_PLAYING,      // Rolling ball under IMU control
  MAZE_STATE_PAUSED,       // Paused
  MAZE_STATE_PITFALL_FALL, // Ball fell into a hole (short animation/pause)
  MAZE_STATE_VICTORY       // Reached exit portal
};

class MazeLogic {
public:
  // Coordinate and scale constants (in screen pixels)
  static constexpr int16_t ORIGIN_X   = 36;
  static constexpr int16_t ORIGIN_Y   = 36;
  static constexpr int16_t CELL_SIZE  = 24;  // 7 * 24 = 168 px width/height
  static constexpr int16_t BALL_RADIUS = 4;  // 9x9 px ball diameter (radius 4)

  MazeLogic();

  // Generate a new random maze (DFS spanning tree guarantees 100% solvability)
  void generateMaze(uint32_t seed = 0, uint16_t existingBestSeconds = 0);

  // Update physical ball motion from IMU accelerometer tiltX, tiltY (-1.0f .. +1.0f)
  void setTilt(float tiltX, float tiltY);

  // Pause / Resume game
  void setPaused(bool paused, unsigned long currentMs);

  // Physics simulation step
  void update(unsigned long currentTimeMs);

  // State inspection (getters)
  MazeState getState() const;
  uint8_t getCell(uint8_t col, uint8_t row) const;
  float getBallX() const;
  float getBallY() const;
  float getBallVx() const;
  float getBallVy() const;
  uint8_t getBerriesCollected() const;
  uint8_t getTotalBerries() const;
  uint16_t getElapsedSeconds(unsigned long currentMs = 0) const;
  uint16_t getBestSeconds() const;
  bool isNewBestRecord() const;

  // Transient event pop flags
  SoundType popSoundEffect();
  bool popWallCollision();
  bool popBerryCollected();
  bool popPitfallFall();

private:
  uint8_t grid[MAZE_ROWS][MAZE_COLS];

  MazeState state;
  float ballX;
  float ballY;
  float ballVx;
  float ballVy;
  float currentTiltX;
  float currentTiltY;

  uint8_t berriesCollected;
  uint8_t totalBerries;

  unsigned long startTimeMs;
  unsigned long lastUpdateMs;
  unsigned long stateTimerEndMs;
  unsigned long pauseStartMs;
  unsigned long totalPausedMs;

  uint16_t bestSeconds;
  bool newBestRecord;

  SoundType pendingSound;
  bool wallCollidedFlag;
  bool berryCollectedFlag;
  bool pitfallFellFlag;

  // Internal generation helpers
  void carvePassagesDfs(uint32_t& rngState);
  void placeItemsAndTraps(uint32_t& rngState);
  static uint32_t nextRandom(uint32_t& state);
  void resolveWallCollisions();
};

#endif // MAZE_LOGIC_H
