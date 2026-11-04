// Fake game loop in the browser – plays the backend's role until Oliver's GameHub is ready.
// It builds the same GameState object as message 3 in docs/MESSAGES.md.
import { BALL_RADIUS, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_X } from './field.js';

const START_LIVES = 3;
const START_SPEED = 0.5;  // field widths per second
const SPEED_UP = 1.05;    // +5% speed after every hit
const PADDLE_SPEED = 1.0; // field heights per second (arrow keys)

const state = {
  status: 'waiting',
  ball: { x: 0.5, y: 0.5 },
  paddle: { y: 0.5 },
  score: 0,
  lives: START_LIVES,
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

// Same rule as Oliver's backend: only start from waiting or gameover
export function startGame() {
  if (state.status === 'playing') return;
  state.status = 'playing';
  state.score = 0;
  state.lives = START_LIVES;
  state.paddle.y = 0.5;
  serveBall(START_SPEED);
}

// Ball starts in the middle and flies towards the right wall at a random angle
function serveBall(speed) {
  state.ball.x = 0.5;
  state.ball.y = 0.5;
  const angle = (Math.random() - 0.5) * (Math.PI / 2); // -45° to 45°
  vx = Math.cos(angle) * speed;
  vy = Math.sin(angle) * speed;
}

// Moves everything forward by dt seconds
export function updateGame(dt) {
  if (state.status !== 'playing') return;

  // Paddle, kept inside the field
  const half = PADDLE_HEIGHT / 2;
  state.paddle.y += paddleDirection * PADDLE_SPEED * dt;
  state.paddle.y = Math.min(Math.max(state.paddle.y, half), 1 - half);

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

  // Right wall
  if (ball.x + BALL_RADIUS > 1) {
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
    vx = Math.abs(vx) * SPEED_UP;
    vy = vy * SPEED_UP;
    state.score += 1;
  }

  // Missed: the ball left the field on the left side
  if (ball.x + BALL_RADIUS < 0) {
    state.lives -= 1;
    if (state.lives <= 0) {
      state.status = 'gameover';
    } else {
      serveBall(START_SPEED);
    }
  }
}
