// Fake game loop in the browser – plays the backend's role until Oliver's GameHub is ready.
// It builds the same GameState object as message 3 in docs/MESSAGES.md.
// Open the page with ?duo to test 2 players: you are player 1 (arrows), player 2 is a simple robot.
import { BALL_RADIUS, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_X, PADDLE2_X } from './field.js';

const START_LIVES = 3;
const START_SPEED = 0.5;  // field widths per second
const SPEED_UP = 1.05;    // +5% speed after every hit
const MAX_SPEED = 1.2;    // limit, else the ball jumps through the paddle in one frame
const PADDLE_SPEED = 1.0; // field heights per second (arrow keys)
const WINNING_SCORE = 5;  // duo: first to 5 points wins (same as the backend)
const ROBOT_SPEED = 0.6;  // duo: player 2 is a bit slow, so it can miss
const DUO = new URLSearchParams(location.search).has('duo');

const state = {
  status: 'waiting',
  mode: DUO ? 'duo' : 'solo',
  ball: { x: 0.5, y: 0.5 },
  paddle: { y: 0.5 },
  paddle2: { y: 0.5 },
  score: 0,
  score2: 0,
  lives: START_LIVES,
  winner: 0,
};

// Ball velocity – only the fake loop needs it, it is not part of GameState
let vx = 0;
let vy = 0;

// -1 = up, 0 = stand still, 1 = down
let paddleDirection = 0;

export function getState() {
  return state;
}

export function setPaddleDirection(direction) {
  paddleDirection = direction;
}

// The fake game has no server, so it is always "connected" (network.js has the real one)
export function isConnected() {
  return true;
}

// Same rule as Oliver's backend: only start from waiting or gameover
export function startGame() {
  if (state.status === 'playing') return;
  state.status = 'playing';
  state.score = 0;
  state.score2 = 0;
  state.lives = START_LIVES;
  state.winner = 0;
  state.paddle.y = 0.5;
  state.paddle2.y = 0.5;
  serveBall(1);
}

// Ball starts in the middle at a random angle. direction: 1 = to the right, -1 = to the left
function serveBall(direction) {
  state.ball.x = 0.5;
  state.ball.y = 0.5;
  const angle = (Math.random() - 0.5) * (Math.PI / 2); // -45° to 45°
  vx = Math.cos(angle) * START_SPEED * direction;
  vy = Math.sin(angle) * START_SPEED;
}

// After a hit: +5%, but never faster than MAX_SPEED. direction = where the ball flies now
function bounce(direction) {
  const speed = Math.hypot(vx, vy);
  const factor = Math.min(SPEED_UP, MAX_SPEED / speed);
  vx = Math.abs(vx) * factor * direction;
  vy = vy * factor;
}

// Duo: the player who did NOT miss gets a point – first to 5 wins
function scorePoint(player) {
  if (player === 1) state.score += 1;
  else state.score2 += 1;

  if (Math.max(state.score, state.score2) >= WINNING_SCORE) {
    state.status = 'gameover';
    state.winner = player;
  } else {
    serveBall(player === 1 ? 1 : -1); // towards the one who missed
  }
}

// Moves everything forward by dt seconds
export function updateGame(dt) {
  if (state.status !== 'playing') return;

  // Paddle, kept inside the field
  const half = PADDLE_HEIGHT / 2;
  state.paddle.y += paddleDirection * PADDLE_SPEED * dt;
  state.paddle.y = Math.min(Math.max(state.paddle.y, half), 1 - half);

  // Duo: the robot (player 2) follows the ball, but slower than the ball
  if (DUO) {
    const diff = state.ball.y - state.paddle2.y;
    state.paddle2.y += Math.sign(diff) * Math.min(Math.abs(diff), ROBOT_SPEED * dt);
    state.paddle2.y = Math.min(Math.max(state.paddle2.y, half), 1 - half);
  }

  // Ball
  const ball = state.ball;
  ball.x += vx * dt;
  ball.y += vy * dt;

  // Top and bottom wall
  if (ball.y - BALL_RADIUS < 0) {
    ball.y = BALL_RADIUS;
    vy = Math.abs(vy);
  }
  if (ball.y + BALL_RADIUS > 1) {
    ball.y = 1 - BALL_RADIUS;
    vy = -Math.abs(vy);
  }

  // Right wall – only in solo (in duo player 2 stands there)
  if (!DUO && ball.x + BALL_RADIUS > 1) {
    ball.x = 1 - BALL_RADIUS;
    vx = -Math.abs(vx);
  }

  // Paddle: the ball flies left and touches the paddle's right side
  const paddleRight = PADDLE_X + PADDLE_WIDTH / 2;
  const touchesPaddle =
    vx < 0 &&
    ball.x - BALL_RADIUS <= paddleRight &&
    ball.x > PADDLE_X &&
    Math.abs(ball.y - state.paddle.y) <= half + BALL_RADIUS;
  if (touchesPaddle) {
    ball.x = paddleRight + BALL_RADIUS;
    bounce(1);
    if (!DUO) state.score += 1; // in duo a hit is not a point
  }

  // Duo: player 2's paddle, the ball flies right and touches its left side
  const paddle2Left = PADDLE2_X - PADDLE_WIDTH / 2;
  const touchesPaddle2 =
    DUO &&
    vx > 0 &&
    ball.x + BALL_RADIUS >= paddle2Left &&
    ball.x < PADDLE2_X &&
    Math.abs(ball.y - state.paddle2.y) <= half + BALL_RADIUS;
  if (touchesPaddle2) {
    ball.x = paddle2Left - BALL_RADIUS;
    bounce(-1);
  }

  // Missed on the left side
  if (ball.x + BALL_RADIUS < 0) {
    if (DUO) {
      scorePoint(2);
    } else {
      state.lives -= 1;
      if (state.lives <= 0) {
        state.status = 'gameover';
      } else {
        serveBall(1);
      }
    }
  } else if (DUO && ball.x - BALL_RADIUS > 1) {
    scorePoint(1); // missed on the right side
  }
}
