import { draw } from './draw.js';
import { refreshScores } from './scores.js';
// Where the game runs. Both files have the same functions, so only this line changes:
// './fakeGame.js' = fake game in the browser, './network.js' = Oliver's /gamehub (SignalR)
import { getState, startGame, updateGame, setPaddleDirection, isConnected } from './network.js';

const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');
const startButton = document.getElementById('start');
const statusText = document.getElementById('status');

// Start / "Spil igen" button → message 4 StartGame() later
startButton.addEventListener('click', () => startGame());

// Arrow keys move the paddle (temporary, until the Oplà sends input)
const keys = { up: false, down: false };

function onKey(event, pressed) { // pressed: true on keydown, false on keyup
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
let lastStatus = null; // to notice the moment a game ends

refreshScores(); // highscore lists under the game, when the page opens

function frame(now) {
  // Seconds since the last frame; max 0.05 so the ball does not jump after a hidden tab
  const dt = Math.min((now - lastTime) / 1000, 0.05);
  lastTime = now;

  updateGame(dt);
  const state = getState();
  draw(ctx, state);

  // A game just ended → the backend has saved it, so load the lists again
  if (state.status === 'gameover' && lastStatus === 'playing') setTimeout(refreshScores, 1500); // ScoreSaver saves every 0.5 s
  lastStatus = state.status;

  startButton.textContent = state.status === 'gameover' ? 'Spil igen' : 'Start';
  startButton.disabled = state.status === 'playing' || !isConnected();
  statusText.textContent = isConnected() ? '' : 'Ingen forbindelse til serveren';

  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);
