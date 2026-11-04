// SignalR connection to Oliver's GameHub (message 3 and 4 in docs/MESSAGES.md).
// Has the same functions as fakeGame.js, so main.js only needs to change one import.
const HUB_URL = 'http://localhost:5243/gamehub';
const RETRY_MS = 3000; // try again every 3 seconds if the server is not running

// Shown until the first GameState arrives
let state = {
  status: 'waiting',
  ball: { x: 0.5, y: 0.5 },
  paddle: { y: 0.5 },
  score: 0,
  lives: 3,
};
let connected = false;

// signalR is a global variable from the <script> tag in index.html
const connection = new signalR.HubConnectionBuilder()
  .withUrl(HUB_URL)
  .withAutomaticReconnect() // reconnects by itself if the connection drops
  .build();

// Message 3: the backend sends GameState ~30 times a second, we just keep the newest
connection.on('GameState', (newState) => {
  state = newState;
});

connection.onreconnecting(() => { connected = false; });
connection.onreconnected(() => { connected = true; });
// Automatic reconnect gave up → start again from scratch
connection.onclose(() => {
  connected = false;
  setTimeout(connect, RETRY_MS);
});

// First connection (withAutomaticReconnect only helps after we were connected once)
async function connect() {
  try {
    await connection.start();
    connected = true;
  } catch (err) {
    console.warn('Ingen forbindelse til', HUB_URL, '– prøver igen om lidt');
    setTimeout(connect, RETRY_MS);
  }
}

connect();

export function getState() {
  return state;
}

export function isConnected() {
  return connected;
}

// Message 4: Start / "Spil igen"
export function startGame() {
  if (!connected) return;
  connection.invoke('StartGame').catch((err) => console.error(err));
}

// The backend runs the game loop, so the browser has nothing to update
export function updateGame(dt) {}

// The paddle is moved by the Oplà, not by the keyboard
export function setPaddleDirection(direction) {}
