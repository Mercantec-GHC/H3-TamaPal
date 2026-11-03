import { draw } from './draw.js';

const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');

// Fake GameState – same format the backend will send later
const fakeState = {
  status: 'playing',
  ball: { x: 0.5, y: 0.3 },
  paddle: { y: 0.4 },
  score: 7,
  lives: 2,
};

draw(ctx, fakeState);
