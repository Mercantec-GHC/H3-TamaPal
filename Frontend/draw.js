import { BALL_RADIUS, PADDLE_WIDTH, PADDLE_HEIGHT, PADDLE_X } from './field.js';

// Colors in the same style as Oleh's Pong on the Oplà screen (Arduino/TamaPal/ui_pong.cpp)
const COLORS = {
  bgCenter: '#0b1a33',  // dark navy in the middle
  bgEdge: '#02040a',    // almost black at the edges
  grid: 'rgba(30, 65, 115, 0.35)',
  rail: '#00f0ff',      // neon cyan walls
  danger: '#e12d41',    // red line on the left: the ball must not pass here
  paddle: '#50f5ff',
  ball: '#55ff8c',      // lime green like Lumi
  ballDark: '#147d3c',
  gold: '#ffe12d',
  orange: '#ff8214',
  text: '#e6f7ff',
  hint: '#78b9e6',
};

const FONT = '"Press Start 2P", monospace'; // pixel font from Google Fonts (index.html)
const RAIL = 6; // thickness of the walls in pixels

// The server only sends positions, so draw.js remembers a little itself:
// the last state (to notice hit/miss), the ball's trail and the sparks
let previous = null;
const trail = [];   // last ball positions → glowing tail
let sparks = [];    // small particles after a hit
let flashUntil = 0; // red flash after a miss, until this time (ms)
let shakeUntil = 0; // screen shake after a miss

// Draws one frame from a GameState object (format from docs/MESSAGES.md)
// The canvas is square, so one size works for both x and y
export function draw(ctx, state) {
  const size = ctx.canvas.width;
  const now = performance.now();

  noticeEvents(state, size, now);

  ctx.save();
  // Screen shake: move everything a few random pixels for a short moment
  if (now < shakeUntil) {
    ctx.translate((Math.random() - 0.5) * 10, (Math.random() - 0.5) * 10);
  }

  drawField(ctx, size, now);
  drawTrail(ctx, size);
  drawPaddle(ctx, state, size, now);
  drawBall(ctx, state, size);
  drawSparks(ctx);
  ctx.restore();

  // Red flash over the whole field after a miss (fades out over 300 ms)
  if (now < flashUntil) {
    ctx.fillStyle = `rgba(225, 45, 65, ${((flashUntil - now) / 300) * 0.35})`;
    ctx.fillRect(0, 0, size, size);
  }

  drawHud(ctx, state, size);
  drawOverlay(ctx, state, size, now);
}

// Compare with the last frame: score went up = hit, lives went down = miss
function noticeEvents(state, size, now) {
  if (previous && state.status === 'playing') {
    if (state.score > previous.score) {
      addSparks(state.ball.x * size, state.ball.y * size);
    }
    if (state.lives < previous.lives) {
      flashUntil = now + 300;
      shakeUntil = now + 250;
    }
  }
  if (state.status !== 'playing') trail.length = 0; // no tail on start/game over screens

  previous = { score: state.score, lives: state.lives };
}

function drawField(ctx, size, now) {
  // Background: radial gradient from navy in the center to black at the edges
  const bg = ctx.createRadialGradient(size / 2, size / 2, 0, size / 2, size / 2, size * 0.75);
  bg.addColorStop(0, COLORS.bgCenter);
  bg.addColorStop(1, COLORS.bgEdge);
  ctx.fillStyle = bg;
  ctx.fillRect(0, 0, size, size);

  // Thin grid lines every 40 px
  ctx.strokeStyle = COLORS.grid;
  ctx.lineWidth = 1;
  for (let i = 40; i < size; i += 40) {
    ctx.beginPath();
    ctx.moveTo(i, 0); ctx.lineTo(i, size);
    ctx.moveTo(0, i); ctx.lineTo(size, i);
    ctx.stroke();
  }

  // Dashed line down the middle that slowly moves
  ctx.setLineDash([12, 12]);
  ctx.lineDashOffset = -now / 40;
  ctx.strokeStyle = 'rgba(80, 245, 255, 0.25)';
  ctx.lineWidth = 3;
  ctx.beginPath();
  ctx.moveTo(size / 2, 0); ctx.lineTo(size / 2, size);
  ctx.stroke();
  ctx.setLineDash([]);

  // Neon walls top, right and bottom (the ball bounces off these)
  ctx.shadowColor = COLORS.rail;
  ctx.shadowBlur = 15;
  ctx.fillStyle = COLORS.rail;
  ctx.fillRect(0, 0, size, RAIL);
  ctx.fillRect(0, size - RAIL, size, RAIL);
  ctx.fillRect(size - RAIL, 0, RAIL, size);

  // Red "danger" line on the left, pulses softly
  ctx.shadowColor = COLORS.danger;
  ctx.globalAlpha = 0.5 + 0.3 * Math.sin(now / 300);
  ctx.fillStyle = COLORS.danger;
  ctx.fillRect(0, RAIL, 3, size - RAIL * 2);
  ctx.globalAlpha = 1;
  ctx.shadowBlur = 0;
}

function drawTrail(ctx, size) {
  // Older positions are drawn smaller and more transparent
  trail.forEach((p, i) => {
    const t = (i + 1) / trail.length; // 0 = oldest, 1 = newest
    ctx.fillStyle = `rgba(85, 255, 140, ${t * 0.3})`;
    ctx.beginPath();
    ctx.arc(p.x * size, p.y * size, BALL_RADIUS * size * t, 0, Math.PI * 2);
    ctx.fill();
  });
}

function drawPaddle(ctx, state, size, now) {
  // Paddle (paddle.y = center of the paddle), rounded corners and neon glow
  const x = (PADDLE_X - PADDLE_WIDTH / 2) * size;
  const y = (state.paddle.y - PADDLE_HEIGHT / 2) * size;
  const w = PADDLE_WIDTH * size;
  const h = PADDLE_HEIGHT * size;

  ctx.shadowColor = COLORS.paddle;
  ctx.shadowBlur = 20 + 5 * Math.sin(now / 200); // glow "breathes"
  const grad = ctx.createLinearGradient(x, 0, x + w, 0);
  grad.addColorStop(0, '#ffffff');
  grad.addColorStop(1, COLORS.paddle);
  ctx.fillStyle = grad;
  ctx.beginPath();
  ctx.roundRect(x, y, w, h, w / 2);
  ctx.fill();
  ctx.shadowBlur = 0;
}

function drawBall(ctx, state, size) {
  const x = state.ball.x * size;
  const y = state.ball.y * size;
  const r = BALL_RADIUS * size;

  if (state.status === 'playing') {
    trail.push({ x: state.ball.x, y: state.ball.y });
    if (trail.length > 10) trail.shift();
  }

  // Ball with a light spot top-left, so it looks round (3D)
  const grad = ctx.createRadialGradient(x - r / 3, y - r / 3, r / 6, x, y, r);
  grad.addColorStop(0, '#ffffff');
  grad.addColorStop(0.4, COLORS.ball);
  grad.addColorStop(1, COLORS.ballDark);
  ctx.shadowColor = COLORS.ball;
  ctx.shadowBlur = 25;
  ctx.fillStyle = grad;
  ctx.beginPath();
  ctx.arc(x, y, r, 0, Math.PI * 2);
  ctx.fill();
  ctx.shadowBlur = 0;
}

function addSparks(x, y) {
  const colors = [COLORS.gold, COLORS.orange, COLORS.paddle, '#ffffff'];
  for (let i = 0; i < 18; i++) {
    const angle = Math.random() * Math.PI * 2;
    const speed = 2 + Math.random() * 4;
    sparks.push({
      x, y,
      vx: Math.cos(angle) * speed,
      vy: Math.sin(angle) * speed,
      life: 1, // 1 = new, 0 = gone
      color: colors[i % colors.length],
    });
  }
}

function drawSparks(ctx) {
  // Each frame: move, slow down and fade every spark; remove the dead ones
  for (const s of sparks) {
    s.x += s.vx;
    s.y += s.vy;
    s.vx *= 0.94;
    s.vy *= 0.94;
    s.life -= 0.03;
    ctx.globalAlpha = Math.max(s.life, 0);
    ctx.fillStyle = s.color;
    ctx.fillRect(s.x - 2, s.y - 2, 4, 4);
  }
  ctx.globalAlpha = 1;
  sparks = sparks.filter((s) => s.life > 0);
}

function drawHud(ctx, state, size) {
  // Score top-right, lives as hearts top-left (pixel font)
  ctx.font = `16px ${FONT}`;
  ctx.textBaseline = 'top';
  ctx.textAlign = 'right';
  ctx.fillStyle = COLORS.gold;
  ctx.shadowColor = COLORS.gold;
  ctx.shadowBlur = 10;
  ctx.fillText(`SCORE ${state.score}`, size - 24, 24);

  ctx.textAlign = 'left';
  ctx.font = '26px sans-serif'; // the pixel font has no heart, so a normal font here
  ctx.fillStyle = COLORS.danger;
  ctx.shadowColor = COLORS.danger;
  ctx.fillText('♥'.repeat(Math.max(state.lives, 0)), 20, 18);
  ctx.shadowBlur = 0;
  ctx.textBaseline = 'alphabetic';
}

function drawOverlay(ctx, state, size, now) {
  if (state.status === 'playing') return;

  // Dark glass over the field
  ctx.fillStyle = 'rgba(2, 4, 10, 0.6)';
  ctx.fillRect(0, 0, size, size);
  ctx.textAlign = 'center';

  if (state.status === 'waiting') {
    title(ctx, 'TAMAPAL PONG', size / 2, size / 2 - 50, COLORS.paddle, 28);
    // Hint blinks slowly (opacity goes up and down)
    ctx.globalAlpha = 0.5 + 0.5 * Math.sin(now / 350);
    ctx.font = `14px ${FONT}`;
    ctx.fillStyle = COLORS.hint;
    ctx.fillText('TRYK PÅ START', size / 2, size / 2 + 10);
    ctx.globalAlpha = 1;
  }

  if (state.status === 'gameover') {
    title(ctx, 'GAME OVER', size / 2, size / 2 - 50, COLORS.danger, 36);
    ctx.font = `18px ${FONT}`;
    ctx.fillStyle = COLORS.gold;
    ctx.fillText(`SCORE ${state.score}`, size / 2, size / 2 + 5);
    ctx.font = `12px ${FONT}`;
    ctx.fillStyle = COLORS.hint;
    ctx.fillText('TRYK PÅ "SPIL IGEN"', size / 2, size / 2 + 45);
  }
  ctx.textAlign = 'left'; // back to normal for the next frame
}

// Big glowing text
function title(ctx, text, x, y, color, px) {
  ctx.font = `${px}px ${FONT}`;
  ctx.fillStyle = color;
  ctx.shadowColor = color;
  ctx.shadowBlur = 25;
  ctx.fillText(text, x, y);
  ctx.shadowBlur = 0;
}
