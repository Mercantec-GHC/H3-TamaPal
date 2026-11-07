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
  uint8_t stackDir[MAZE_COLS * MAZE_ROWS];
  int16_t stackTop = 0;

  // Start at Top-Left (0, 0)
  grid[0][0] |= MAZE_VISITED;
  stackCol[0] = 0;
  stackRow[0] = 0;
  stackDir[0] = 255;

  while (stackTop >= 0) {
    uint8_t curC = stackCol[stackTop];
    uint8_t curR = stackRow[stackTop];
    uint8_t lastDir = stackDir[stackTop];

    // Find unvisited neighbors (North=0, East=1, South=2, West=3)
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
      // Turn-biased winding selection: heavily favor 90-degree turns and stepping inward away from outer borders
      // so the labyrinth forms serpentine S-curves instead of straight perimeter highways!
      uint8_t weightedPool[24];
      uint8_t poolSize = 0;
      bool onOuterBorder = (curC == 0 || curC == MAZE_COLS - 1 || curR == 0 || curR == MAZE_ROWS - 1);

      for (uint8_t i = 0; i < unvisitedCount; i++) {
        uint8_t dir = unvisitedDir[i];
        uint8_t nC = (dir == 1) ? (curC + 1) : ((dir == 3) ? (curC - 1) : curC);
        uint8_t nR = (dir == 2) ? (curR + 1) : ((dir == 0) ? (curR - 1) : curR);
        bool nextInInterior = (nC > 0 && nC < MAZE_COLS - 1 && nR > 0 && nR < MAZE_ROWS - 1);

        uint8_t weight = 1;
        if (lastDir != 255 && dir != lastDir) {
          weight += 3; // Strongly prefer turning 90 degrees over straight lines
        }
        if (onOuterBorder && nextInInterior) {
          weight += 2; // Pull paths inward away from outer walls
        }
        for (uint8_t w = 0; w < weight && poolSize < 24; w++) {
          weightedPool[poolSize++] = dir;
        }
      }

      uint8_t pick = weightedPool[nextRandom(rng) % poolSize];
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
      stackDir[stackTop] = pick;
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

  // Open 2 extra internal walls between interior 2-wall corridor cells to create WIDE chambers/plazas
  // (Preserves all 3-wall dead ends for Berries & Black Holes while mixing wide 40px rooms with 20px/13px corridors)
  uint8_t wideOpened = 0;
  for (uint8_t r = 1; r < MAZE_ROWS - 2 && wideOpened < 2; r++) {
    for (uint8_t c = 1; c < MAZE_COLS - 2 && wideOpened < 2; c++) {
      uint8_t w1 = 0;
      if (grid[r][c] & MAZE_WALL_NORTH) w1++;
      if (grid[r][c] & MAZE_WALL_EAST)  w1++;
      if (grid[r][c] & MAZE_WALL_SOUTH) w1++;
      if (grid[r][c] & MAZE_WALL_WEST)  w1++;
      if (w1 != 2) continue;

      if (grid[r][c] & MAZE_WALL_EAST) {
        uint8_t w2 = 0;
        if (grid[r][c + 1] & MAZE_WALL_NORTH) w2++;
        if (grid[r][c + 1] & MAZE_WALL_EAST)  w2++;
        if (grid[r][c + 1] & MAZE_WALL_SOUTH) w2++;
        if (grid[r][c + 1] & MAZE_WALL_WEST)  w2++;
        if (w2 == 2) {
          grid[r][c]     &= ~MAZE_WALL_EAST;
          grid[r][c + 1] &= ~MAZE_WALL_WEST;
          wideOpened++;
          continue;
        }
      }
      if (grid[r][c] & MAZE_WALL_SOUTH) {
        uint8_t w2 = 0;
        if (grid[r + 1][c] & MAZE_WALL_NORTH) w2++;
        if (grid[r + 1][c] & MAZE_WALL_EAST)  w2++;
        if (grid[r + 1][c] & MAZE_WALL_SOUTH) w2++;
        if (grid[r + 1][c] & MAZE_WALL_WEST)  w2++;
        if (w2 == 2) {
          grid[r][c]     &= ~MAZE_WALL_SOUTH;
          grid[r + 1][c] &= ~MAZE_WALL_NORTH;
          wideOpened++;
        }
      }
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

  // Distribute items among dead ends (always reserve at least 1 dead end for a Black Hole trap):
  // 1. Up to 3 Deku Berries (exploration rewards)
  uint8_t berriesToPlace = (deadCount >= 4) ? 3 : ((deadCount >= 3) ? 2 : 1);
  for (uint8_t i = 0; i < berriesToPlace && i < deadCount; i++) {
    grid[deadRow[i]][deadCol[i]] |= MAZE_ITEM_BERRY;
    totalBerries++;
  }

  // 2. Up to 3 Black Holes (MAZE_ITEM_PITFALL) with gravitational pull
  uint8_t trapsToPlace = (deadCount >= berriesToPlace + 3) ? 3 : ((deadCount >= berriesToPlace + 2) ? 2 : 1);
  for (uint8_t i = berriesToPlace; i < berriesToPlace + trapsToPlace && i < deadCount; i++) {
    grid[deadRow[i]][deadCol[i]] |= MAZE_ITEM_PITFALL;
  }

  // 3. Place 2-3 Narrow Pinch Passages (MAZE_ITEM_ICE / MAZE_ITEM_NARROW) on straight corridors
  uint8_t narrowPlaced = 0;
  for (uint8_t r = 0; r < MAZE_ROWS && narrowPlaced < 3; r++) {
    for (uint8_t c = 0; c < MAZE_COLS && narrowPlaced < 3; c++) {
      if ((c == 0 && r == 0) || (c == MAZE_COLS - 1 && r == MAZE_ROWS - 1)) continue;
      if (!(grid[r][c] & MAZE_ITEM_MASK)) {
        uint8_t cell = grid[r][c];
        bool horizCorridor = (cell & MAZE_WALL_NORTH) && (cell & MAZE_WALL_SOUTH) &&
                             !(cell & MAZE_WALL_EAST) && !(cell & MAZE_WALL_WEST);
        bool vertCorridor  = (cell & MAZE_WALL_EAST) && (cell & MAZE_WALL_WEST) &&
                             !(cell & MAZE_WALL_NORTH) && !(cell & MAZE_WALL_SOUTH);
        if ((horizCorridor || vertCorridor) && (nextRandom(rng) % 2 == 0)) {
          grid[r][c] |= MAZE_ITEM_ICE;
          narrowPlaced++;
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
  if (dtMs < 10) {
    return; // Wait until at least 10 ms of real wall-clock time has elapsed (prevents 30x turbo speed in fast loop())
  }
  lastUpdateMs = currentTimeMs;

  if (dtMs > 100) dtMs = 100;

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

  // 1. Determine cell under ball
  int curCol = (int)((ballX - ORIGIN_X) / CELL_SIZE);
  int curRow = (int)((ballY - ORIGIN_Y) / CELL_SIZE);
  if (curCol < 0) curCol = 0;
  if (curCol >= MAZE_COLS) curCol = MAZE_COLS - 1;
  if (curRow < 0) curRow = 0;
  if (curRow >= MAZE_ROWS) curRow = MAZE_ROWS - 1;

  float friction = 0.86f;
  float accelScale = 0.30f;

  // 2. Accelerometer integration
  ballVx = (ballVx + currentTiltX * accelScale * speedFactor) * friction;
  ballVy = (ballVy + currentTiltY * accelScale * speedFactor) * friction;

  // 2b. Black Hole (MAZE_ITEM_PITFALL) gravitational pull:
  // Gently pulls the ball toward nearby Black Holes across open doorways/passages,
  // requiring the player to tilt more strongly (> 40%) away from the hole to escape!
  for (uint8_t pr = 0; pr < MAZE_ROWS; pr++) {
    for (uint8_t pc = 0; pc < MAZE_COLS; pc++) {
      if (grid[pr][pc] & MAZE_ITEM_PITFALL) {
        bool connected = ((int)pc == curCol && (int)pr == curRow);
        if (!connected) {
          uint8_t pCell = grid[pr][pc];
          if ((int)pc == curCol && (int)pr == curRow + 1 && !(pCell & MAZE_WALL_NORTH)) connected = true;
          if ((int)pc == curCol && (int)pr + 1 == curRow && !(pCell & MAZE_WALL_SOUTH)) connected = true;
          if ((int)pc == curCol + 1 && (int)pr == curRow && !(pCell & MAZE_WALL_WEST))  connected = true;
          if ((int)pc + 1 == curCol && (int)pr == curRow && !(pCell & MAZE_WALL_EAST))  connected = true;
        }
        if (connected) {
          float pitX = ORIGIN_X + pc * CELL_SIZE + CELL_SIZE * 0.5f;
          float pitY = ORIGIN_Y + pr * CELL_SIZE + CELL_SIZE * 0.5f;
          float gdx = pitX - ballX;
          float gdy = pitY - ballY;
          float distSq = gdx * gdx + gdy * gdy;
          if (distSq > 1.0f && distSq < 900.0f) { // Within 30 px gravitational horizon
            float dist = sqrtf(distSq);
            float pull = 0.11f * (1.0f - (dist / 32.0f)) * speedFactor;
            ballVx += (gdx / dist) * pull;
            ballVy += (gdy / dist) * pull;
          }
        }
      }
    }
  }

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
  // Narrow Pinch Passages (MAZE_ITEM_ICE / MAZE_ITEM_NARROW) protrude 2.5px inward from each side wall!
  const float wallPad = (grid[row][c] & MAZE_ITEM_ICE) ? (r + 3.5f) : (r + 1.0f);

  // North Wall
  if (grid[row][c] & MAZE_WALL_NORTH) {
    float minY = cellTop + wallPad;
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
    float maxY = cellBottom - wallPad;
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
    float minX = cellLeft + wallPad;
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
    float maxX = cellRight - wallPad;
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
