import { BALL_RADIUS, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_X } from './field.js';

// Draws one frame from a GameState object (format from docs/MESSAGES.md)
// The canvas is square, so one size works for both x and y
export function draw(ctx, state) {
  const size = ctx.canvas.width;

  // Background
  ctx.fillStyle = 'black';
  ctx.fillRect(0, 0, size, size);

  // Paddle (paddle.y = center of the paddle)
  ctx.fillStyle = 'white'; // paddle, ball and text are white
  // fillRect needs the top-left corner, so go from the center back by half the size
  const paddleLeftX = (PADDLE_X - PADDLE_WIDTH / 2) * size; // (0.03 - 0.01) * 600 = 12 px
  const paddleTopY = (state.paddle.y - PADDLE_HEIGHT / 2) * size; // e.g. paddle.y 0.4: (0.4 - 0.1) * 600 = 180 px
  // x, y, width, height → e.g. fillRect(12, 180, 12, 120): from x 12 to 24, from y 180 to 300
  ctx.fillRect(paddleLeftX, paddleTopY, PADDLE_WIDTH * size, PADDLE_HEIGHT * size);

  // Ball (ball.x, ball.y = center of the ball)
  ctx.beginPath();
  ctx.arc(state.ball.x * size, state.ball.y * size, BALL_RADIUS * size, 0, Math.PI * 2);
  ctx.fill();

  // Score and lives
  ctx.font = '20px monospace';
  ctx.fillText(`Score: ${state.score}`, size - 150, 30);
  ctx.fillText(`Liv: ${state.lives}`, size - 150, 55);

  // Start and game over screens
  ctx.textAlign = 'center';
  if (state.status === 'waiting') {
    ctx.fillText('Tryk på Start', size / 2, size / 2 - 40);
  }
  if (state.status === 'gameover') {
    ctx.font = '40px monospace';
    ctx.fillText('GAME OVER', size / 2, size / 2 - 60);
    ctx.font = '20px monospace';
    ctx.fillText(`Score: ${state.score}`, size / 2, size / 2 - 25);
  }
  ctx.textAlign = 'left'; // back to normal for the next frame
}
