// Fixed sizes on the field, 0–1 (agreed in docs/MESSAGES.md, Del 1c punkt 4)
const BALL_RADIUS = 0.02;
const PADDLE_WIDTH = 0.02;
const PADDLE_HEIGHT = 0.2;
const PADDLE_X = 0.03; // center of the paddle

// Draws one frame from a GameState object (format from docs/MESSAGES.md)
// The canvas is square, so one size works for both x and y
export function draw(ctx, state) {
  const size = ctx.canvas.width;

  // Background
  ctx.fillStyle = 'black';
  ctx.fillRect(0, 0, size, size);

  // Paddle (paddle.y = center of the paddle)
  ctx.fillStyle = 'white';
  const paddleLeft = (PADDLE_X - PADDLE_WIDTH / 2) * size;
  const paddleTop = (state.paddle.y - PADDLE_HEIGHT / 2) * size;
  ctx.fillRect(paddleLeft, paddleTop, PADDLE_WIDTH * size, PADDLE_HEIGHT * size);

  // Ball (ball.x, ball.y = center of the ball)
  ctx.beginPath();
  ctx.arc(state.ball.x * size, state.ball.y * size, BALL_RADIUS * size, 0, Math.PI * 2);
  ctx.fill();

  // Score and lives
  ctx.font = '20px monospace';
  ctx.fillText(`Score: ${state.score}`, size - 150, 30);
  ctx.fillText(`Liv: ${state.lives}`, size - 150, 55);
}
