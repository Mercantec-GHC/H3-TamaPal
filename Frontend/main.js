import { draw } from './draw.js';
// Fake backend for now – later replaced by a SignalR connection to /gamehub
import { getState, startGame, updateGame, setPaddleDirection } from './fakeGame.js';

const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');
const startButton = document.getElementById('start');

// Start / "Spil igen" button → message 4 StartGame() later
startButton.addEventListener('click', () => startGame());

// Arrow keys move the paddle (temporary, until the Oplà sends input)
const keys = { up: false, down: false };

function onKey(event, pressed) {
  if (event.key === 'ArrowUp') keys.up = pressed;
  else if (event.key === 'ArrowDown') keys.down = pressed;
  else return;
  event.preventDefault(); // stop the arrows from scrolling the page
  setPaddleDirection((keys.down ? 1 : 0) - (keys.up ? 1 : 0));
}

window.addEventListener('keydown', (event) => onKey(event, true));
window.addEventListener('keyup', (event) => onKey(event, false));

// Game loop: the browser calls frame() before every screen refresh (~60 times a second)
let lastTime = performance.now();

function frame(now) {
  // Seconds since the last frame; max 0.05 so the ball does not jump after a hidden tab
  const dt = Math.min((now - lastTime) / 1000, 0.05);
  lastTime = now;

  updateGame(dt);
  const state = getState();
  draw(ctx, state);

  startButton.textContent = state.status === 'gameover' ? 'Spil igen' : 'Start';
  startButton.disabled = state.status === 'playing';

  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);
