// WHAT: Implementation of procedural maze generation and physics simulation
// WHY:  DFS spanning tree guarantees 100% solvability with traps, berries, and zero heap allocations
// USED BY: TamaPal.ino, ui_maze.cpp, tests/test_main.cpp

#include "maze_logic.h"
#include <string.h>
#include <math.h>

MazeLogic::MazeLogic()
  : state(MAZE_STATE_READY),
    ballX(ORIGIN_X + CELL_SIZE * 0.5f),
    ballY(ORIGIN_Y + CELL_SIZE * 0.5f),
    ballVx(0.0f),
    ballVy(0.0f),
    currentTiltX(0.0f),
    currentTiltY(0.0f),
    berriesCollected(0),
    totalBerries(0),
    startTimeMs(0),
    lastUpdateMs(0),
    stateTimerEndMs(0),
    pauseStartMs(0),
    totalPausedMs(0),
    bestSeconds(0),
    newBestRecord(false),
    pendingSound(SOUND_NONE),
    wallCollidedFlag(false),
    berryCollectedFlag(false),
    pitfallFellFlag(false) {
  memset(grid, 0, sizeof(grid));
}

uint32_t MazeLogic::nextRandom(uint32_t& rng) {
  if (rng == 0) rng = 0x12345678;
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return rng;
}

void MazeLogic::generateMaze(uint32_t seed, uint16_t existingBestSeconds) {
  uint32_t rng = (seed == 0) ? 0x98765432 : seed;
  bestSeconds = existingBestSeconds;
  newBestRecord = false;
  berriesCollected = 0;
  totalBerries = 0;
  totalPausedMs = 0;
  pauseStartMs = 0;
  startTimeMs = 0;
  lastUpdateMs = 0;
  stateTimerEndMs = 0;
  pendingSound = SOUND_CONFIRM;
  wallCollidedFlag = false;
  berryCollectedFlag = false;
  pitfallFellFlag = false;

  // 1. Initialize all cells with 4 walls
  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    for (uint8_t c = 0; c < MAZE_COLS; c++) {
      grid[r][c] = (MAZE_WALL_NORTH | MAZE_WALL_EAST | MAZE_WALL_SOUTH | MAZE_WALL_WEST);
    }
  }

  // 2. Carve passages using Depth-First Search (DFS) spanning tree
  carvePassagesDfs(rng);

  // 3. Place dead-end traps, bonus berries, and ice patches
  placeItemsAndTraps(rng);

  // 4. Reset ball to start cell (0, 0)
  ballX = ORIGIN_X + CELL_SIZE * 0.5f;
  ballY = ORIGIN_Y + CELL_SIZE * 0.5f;
  ballVx = 0.0f;
  ballVy = 0.0f;
  currentTiltX = 0.0f;
  currentTiltY = 0.0f;
  state = MAZE_STATE_READY;
}

void MazeLogic::carvePassagesDfs(uint32_t& rng) {
  // Explicit stack to prevent microcontroller recursion stack overflow
  uint8_t stackCol[MAZE_COLS * MAZE_ROWS];
  uint8_t stackRow[MAZE_COLS * MAZE_ROWS];
  int16_t stackTop = 0;

  // Start at Top-Left (0, 0)
  grid[0][0] |= MAZE_VISITED;
  stackCol[0] = 0;
  stackRow[0] = 0;

  while (stackTop >= 0) {
    uint8_t curC = stackCol[stackTop];
    uint8_t curR = stackRow[stackTop];

    // Find unvisited neighbors (North, East, South, West)
    uint8_t unvisitedDir[4];
    uint8_t unvisitedCount = 0;

    // North (0)
    if (curR > 0 && !(grid[curR - 1][curC] & MAZE_VISITED)) {
      unvisitedDir[unvisitedCount++] = 0;
    }
    // East (1)
    if (curC < MAZE_COLS - 1 && !(grid[curR][curC + 1] & MAZE_VISITED)) {
      unvisitedDir[unvisitedCount++] = 1;
    }
    // South (2)
    if (curR < MAZE_ROWS - 1 && !(grid[curR + 1][curC] & MAZE_VISITED)) {
      unvisitedDir[unvisitedCount++] = 2;
    }
    // West (3)
    if (curC > 0 && !(grid[curR][curC - 1] & MAZE_VISITED)) {
      unvisitedDir[unvisitedCount++] = 3;
    }

    if (unvisitedCount > 0) {
      // Pick random neighbor
      uint8_t pick = unvisitedDir[nextRandom(rng) % unvisitedCount];
      uint8_t nextC = curC;
      uint8_t nextR = curR;

      if (pick == 0) { // North
        nextR = curR - 1;
        grid[curR][curC] &= ~MAZE_WALL_NORTH;
        grid[nextR][nextC] &= ~MAZE_WALL_SOUTH;
      } else if (pick == 1) { // East
        nextC = curC + 1;
        grid[curR][curC] &= ~MAZE_WALL_EAST;
        grid[nextR][nextC] &= ~MAZE_WALL_WEST;
      } else if (pick == 2) { // South
        nextR = curR + 1;
        grid[curR][curC] &= ~MAZE_WALL_SOUTH;
        grid[nextR][nextC] &= ~MAZE_WALL_NORTH;
      } else if (pick == 3) { // West
        nextC = curC - 1;
        grid[curR][curC] &= ~MAZE_WALL_WEST;
        grid[nextR][nextC] &= ~MAZE_WALL_EAST;
      }

      grid[nextR][nextC] |= MAZE_VISITED;

      // Push to stack
      stackTop++;
      stackCol[stackTop] = nextC;
      stackRow[stackTop] = nextR;
    } else {
      // Backtrack
      stackTop--;
    }
  }

  // Clear MAZE_VISITED flag
  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    for (uint8_t c = 0; c < MAZE_COLS; c++) {
      grid[r][c] &= ~MAZE_VISITED;
    }
  }
}

void MazeLogic::placeItemsAndTraps(uint32_t& rng) {
  // Find dead ends (cells with 3 walls, excluding start (0,0) and exit (6,6))
  uint8_t deadCol[MAZE_COLS * MAZE_ROWS];
  uint8_t deadRow[MAZE_COLS * MAZE_ROWS];
  uint8_t deadCount = 0;

  for (uint8_t r = 0; r < MAZE_ROWS; r++) {
    for (uint8_t c = 0; c < MAZE_COLS; c++) {
      if ((c == 0 && r == 0) || (c == MAZE_COLS - 1 && r == MAZE_ROWS - 1)) {
        continue;
      }
      uint8_t wallCount = 0;
      if (grid[r][c] & MAZE_WALL_NORTH) wallCount++;
      if (grid[r][c] & MAZE_WALL_EAST)  wallCount++;
      if (grid[r][c] & MAZE_WALL_SOUTH) wallCount++;
      if (grid[r][c] & MAZE_WALL_WEST)  wallCount++;

      if (wallCount >= 3) {
        deadCol[deadCount] = c;
        deadRow[deadCount] = r;
        deadCount++;
      }
    }
  }

  // Shuffle dead ends with Fisher-Yates
  for (int i = deadCount - 1; i > 0; i--) {
    int j = nextRandom(rng) % (i + 1);
    uint8_t tc = deadCol[i]; deadCol[i] = deadCol[j]; deadCol[j] = tc;
    uint8_t tr = deadRow[i]; deadRow[i] = deadRow[j]; deadRow[j] = tr;
  }

  // Distribute items among dead ends:
  // 1. Up to 3 Deku Berries (exploration rewards)
  uint8_t berriesToPlace = (deadCount >= 3) ? 3 : (deadCount / 2);
  for (uint8_t i = 0; i < berriesToPlace; i++) {
    grid[deadRow[i]][deadCol[i]] |= MAZE_ITEM_BERRY;
    totalBerries++;
  }

  // 2. Up to 2 Pitfall Holes (traps in false dead ends)
  uint8_t trapsToPlace = (deadCount >= berriesToPlace + 2) ? 2 : 1;
  for (uint8_t i = berriesToPlace; i < berriesToPlace + trapsToPlace && i < deadCount; i++) {
    grid[deadRow[i]][deadCol[i]] |= MAZE_ITEM_PITFALL;
  }

  // 3. Place 1-2 Ice patches in corridors (cells with exactly 2 walls)
  uint8_t icePlaced = 0;
  for (uint8_t r = 1; r < MAZE_ROWS - 1 && icePlaced < 2; r++) {
    for (uint8_t c = 1; c < MAZE_COLS - 1 && icePlaced < 2; c++) {
      if (!(grid[r][c] & MAZE_ITEM_MASK)) {
        uint8_t wallCount = 0;
        if (grid[r][c] & MAZE_WALL_NORTH) wallCount++;
        if (grid[r][c] & MAZE_WALL_EAST)  wallCount++;
        if (grid[r][c] & MAZE_WALL_SOUTH) wallCount++;
        if (grid[r][c] & MAZE_WALL_WEST)  wallCount++;
        if (wallCount == 2 && (nextRandom(rng) % 3 == 0)) {
          grid[r][c] |= MAZE_ITEM_ICE;
          icePlaced++;
        }
      }
    }
  }
}

void MazeLogic::setTilt(float tiltX, float tiltY) {
  currentTiltX = tiltX;
  currentTiltY = tiltY;
}

void MazeLogic::setPaused(bool paused, unsigned long currentMs) {
  if (paused && state == MAZE_STATE_PLAYING) {
    state = MAZE_STATE_PAUSED;
    pauseStartMs = currentMs;
  } else if (!paused && state == MAZE_STATE_PAUSED) {
    state = MAZE_STATE_PLAYING;
    if (pauseStartMs > 0 && currentMs >= pauseStartMs) {
      startTimeMs += (currentMs - pauseStartMs);
    }
    pauseStartMs = 0;
    lastUpdateMs = currentMs;
  }
}

void MazeLogic::update(unsigned long currentTimeMs) {
  if (state == MAZE_STATE_PAUSED) {
    return;
  }

  if (lastUpdateMs == 0) {
    lastUpdateMs = currentTimeMs;
    if (state != MAZE_STATE_READY) {
      startTimeMs = currentTimeMs;
    }
    return;
  }

  unsigned long dtMs = currentTimeMs - lastUpdateMs;
  lastUpdateMs = currentTimeMs;

  if (dtMs > 100) dtMs = 100;
  if (dtMs < 5) dtMs = 5;

  float speedFactor = (float)dtMs / 20.0f;

  if (state == MAZE_STATE_READY) {
    // First tilt or motion starts the game timer
    if (currentTiltX < -0.08f || currentTiltX > 0.08f || currentTiltY < -0.08f || currentTiltY > 0.08f) {
      state = MAZE_STATE_PLAYING;
      startTimeMs = currentTimeMs;
    }
    return;
  }

  if (state == MAZE_STATE_PITFALL_FALL) {
    if (currentTimeMs >= stateTimerEndMs) {
      // Respawn at starting cell
      ballX = ORIGIN_X + CELL_SIZE * 0.5f;
      ballY = ORIGIN_Y + CELL_SIZE * 0.5f;
      ballVx = 0.0f;
      ballVy = 0.0f;
      state = MAZE_STATE_PLAYING;
    }
    return;
  }

  if (state == MAZE_STATE_VICTORY) {
    return;
  }

  // 1. Determine cell under ball to check surface properties (e.g. Ice)
  int curCol = (int)((ballX - ORIGIN_X) / CELL_SIZE);
  int curRow = (int)((ballY - ORIGIN_Y) / CELL_SIZE);
  if (curCol < 0) curCol = 0;
  if (curCol >= MAZE_COLS) curCol = MAZE_COLS - 1;
  if (curRow < 0) curRow = 0;
  if (curRow >= MAZE_ROWS) curRow = MAZE_ROWS - 1;

  bool onIce = (grid[curRow][curCol] & MAZE_ITEM_ICE) != 0;
  float friction = onIce ? 0.98f : 0.86f;
  float accelScale = onIce ? 0.40f : 0.30f;

  // 2. Accelerometer integration
  ballVx = (ballVx + currentTiltX * accelScale * speedFactor) * friction;
  ballVy = (ballVy + currentTiltY * accelScale * speedFactor) * friction;

  // Cap max velocity
  const float maxV = 4.2f;
  if (ballVx > maxV)  ballVx = maxV;
  if (ballVx < -maxV) ballVx = -maxV;
  if (ballVy > maxV)  ballVy = maxV;
  if (ballVy < -maxV) ballVy = -maxV;

  // 3. Step movement
  ballX += ballVx * speedFactor;
  ballY += ballVy * speedFactor;

  // 4. Resolve wall collisions with smooth corner sliding
  resolveWallCollisions();

  // 5. Trigger items (Berries, Pitfalls, Victory exit)
  int c = (int)((ballX - ORIGIN_X) / CELL_SIZE);
  int r = (int)((ballY - ORIGIN_Y) / CELL_SIZE);
  if (c >= 0 && c < MAZE_COLS && r >= 0 && r < MAZE_ROWS) {
    float cellCenterX = ORIGIN_X + c * CELL_SIZE + CELL_SIZE * 0.5f;
    float cellCenterY = ORIGIN_Y + r * CELL_SIZE + CELL_SIZE * 0.5f;
    float dx = ballX - cellCenterX;
    float dy = ballY - cellCenterY;
    float distSq = dx * dx + dy * dy;

    // Item pickup threshold (within 8 pixels of cell center)
    if (distSq <= 64.0f) {
      if (grid[r][c] & MAZE_ITEM_BERRY) {
        grid[r][c] &= ~MAZE_ITEM_BERRY;
        berriesCollected++;
        berryCollectedFlag = true;
        pendingSound = SOUND_CONFIRM;
      } else if (grid[r][c] & MAZE_ITEM_PITFALL) {
        pitfallFellFlag = true;
        pendingSound = SOUND_MISS;
        state = MAZE_STATE_PITFALL_FALL;
        stateTimerEndMs = currentTimeMs + 700; // 700 ms pause before respawn
        totalPausedMs += 5000; // 5-second penalty added to timer!
      }
    }

    // Victory condition: reach exit cell (MAZE_COLS - 1, MAZE_ROWS - 1)
    if (c == MAZE_COLS - 1 && r == MAZE_ROWS - 1 && distSq <= 81.0f) {
      state = MAZE_STATE_VICTORY;
      pendingSound = SOUND_CONFIRM;
      uint16_t elapsed = getElapsedSeconds();
      if (bestSeconds == 0 || elapsed < bestSeconds) {
        bestSeconds = elapsed;
        newBestRecord = true;
      }
    }
  }
}

void MazeLogic::resolveWallCollisions() {
  const float r = (float)BALL_RADIUS;
  int c = (int)((ballX - ORIGIN_X) / CELL_SIZE);
  int row = (int)((ballY - ORIGIN_Y) / CELL_SIZE);
  if (c < 0) c = 0;
  if (c >= MAZE_COLS) c = MAZE_COLS - 1;
  if (row < 0) row = 0;
  if (row >= MAZE_ROWS) row = MAZE_ROWS - 1;

  float cellLeft   = (float)(ORIGIN_X + c * CELL_SIZE);
  float cellRight  = cellLeft + (float)CELL_SIZE;
  float cellTop    = (float)(ORIGIN_Y + row * CELL_SIZE);
  float cellBottom = cellTop + (float)CELL_SIZE;

  // Collision against cell walls (smooth sliding at low impact speed, soft bounce on fast impact):
  // North Wall
  if (grid[row][c] & MAZE_WALL_NORTH) {
    float minY = cellTop + r + 1.0f;
    if (ballY < minY) {
      ballY = minY;
      if (ballVy < 0.0f) {
        if (ballVy < -0.85f) {
          ballVy = -ballVy * 0.2f;
          wallCollidedFlag = true;
        } else {
          ballVy = 0.0f;
        }
      }
    }
  }

  // South Wall
  if (grid[row][c] & MAZE_WALL_SOUTH) {
    float maxY = cellBottom - r - 1.0f;
    if (ballY > maxY) {
      ballY = maxY;
      if (ballVy > 0.0f) {
        if (ballVy > 0.85f) {
          ballVy = -ballVy * 0.2f;
          wallCollidedFlag = true;
        } else {
          ballVy = 0.0f;
        }
      }
    }
  }

  // West Wall
  if (grid[row][c] & MAZE_WALL_WEST) {
    float minX = cellLeft + r + 1.0f;
    if (ballX < minX) {
      ballX = minX;
      if (ballVx < 0.0f) {
        if (ballVx < -0.85f) {
          ballVx = -ballVx * 0.2f;
          wallCollidedFlag = true;
        } else {
          ballVx = 0.0f;
        }
      }
    }
  }

  // East Wall
  if (grid[row][c] & MAZE_WALL_EAST) {
    float maxX = cellRight - r - 1.0f;
    if (ballX > maxX) {
      ballX = maxX;
      if (ballVx > 0.0f) {
        if (ballVx > 0.85f) {
          ballVx = -ballVx * 0.2f;
          wallCollidedFlag = true;
        } else {
          ballVx = 0.0f;
        }
      }
    }
  }

  // 5. Corner pillars (intersections of grid walls)
  const float pillarR = r + 0.4f;
  const float pillarRSq = pillarR * pillarR;
  const float cornerX[4] = { cellLeft, cellRight, cellLeft, cellRight };
  const float cornerY[4] = { cellTop, cellTop, cellBottom, cellBottom };

  for (int i = 0; i < 4; i++) {
    float dx = ballX - cornerX[i];
    float dy = ballY - cornerY[i];
    float dSq = dx * dx + dy * dy;
    if (dSq < pillarRSq && dSq > 0.0001f) {
      float d = sqrtf(dSq);
      float nx = dx / d;
      float ny = dy / d;
      ballX = cornerX[i] + nx * pillarR;
      ballY = cornerY[i] + ny * pillarR;

      float dot = ballVx * nx + ballVy * ny;
      if (dot < 0.0f) {
        ballVx -= (1.2f * dot * nx);
        ballVy -= (1.2f * dot * ny);
        wallCollidedFlag = true;
      }
    }
  }

  // Global outer boundary clamping
  float globalMinX = (float)(ORIGIN_X + r + 1);
  float globalMaxX = (float)(ORIGIN_X + MAZE_COLS * CELL_SIZE - r - 1);
  float globalMinY = (float)(ORIGIN_Y + r + 1);
  float globalMaxY = (float)(ORIGIN_Y + MAZE_ROWS * CELL_SIZE - r - 1);

  if (ballX < globalMinX) ballX = globalMinX;
  if (ballX > globalMaxX) ballX = globalMaxX;
  if (ballY < globalMinY) ballY = globalMinY;
  if (ballY > globalMaxY) ballY = globalMaxY;
}

MazeState MazeLogic::getState() const {
  return state;
}

uint8_t MazeLogic::getCell(uint8_t col, uint8_t r) const {
  if (col >= MAZE_COLS || r >= MAZE_ROWS) return 0;
  return grid[r][col];
}

float MazeLogic::getBallX() const {
  return ballX;
}

float MazeLogic::getBallY() const {
  return ballY;
}

float MazeLogic::getBallVx() const {
  return ballVx;
}

float MazeLogic::getBallVy() const {
  return ballVy;
}

uint8_t MazeLogic::getBerriesCollected() const {
  return berriesCollected;
}

uint8_t MazeLogic::getTotalBerries() const {
  return totalBerries;
}

uint16_t MazeLogic::getElapsedSeconds(unsigned long currentMs) const {
  if (state == MAZE_STATE_READY || startTimeMs == 0) return 0;
  unsigned long endMs = currentMs;
  if (state == MAZE_STATE_PAUSED && pauseStartMs >= startTimeMs) {
    endMs = pauseStartMs;
  } else if (state == MAZE_STATE_VICTORY) {
    endMs = lastUpdateMs;
  } else if (endMs == 0) {
    endMs = (lastUpdateMs >= startTimeMs) ? lastUpdateMs : startTimeMs;
  }
  if (endMs < startTimeMs) return 0;

  unsigned long rawMs = (endMs - startTimeMs) + totalPausedMs;
  unsigned long berryBonusMs = (unsigned long)berriesCollected * 2000UL;
  uint16_t sec = 0;
  if (rawMs > berryBonusMs) {
    sec = (uint16_t)((rawMs - berryBonusMs) / 1000UL);
  }
  if (state == MAZE_STATE_VICTORY && sec == 0) {
    return 1;
  }
  return sec;
}

uint16_t MazeLogic::getBestSeconds() const {
  return bestSeconds;
}

bool MazeLogic::isNewBestRecord() const {
  return newBestRecord;
}

SoundType MazeLogic::popSoundEffect() {
  SoundType s = pendingSound;
  pendingSound = SOUND_NONE;
  return s;
}

bool MazeLogic::popWallCollision() {
  bool c = wallCollidedFlag;
  wallCollidedFlag = false;
  return c;
}

bool MazeLogic::popBerryCollected() {
  bool b = berryCollectedFlag;
  berryCollectedFlag = false;
  return b;
}

bool MazeLogic::popPitfallFall() {
  bool p = pitfallFellFlag;
  pitfallFellFlag = false;
  return p;
}
