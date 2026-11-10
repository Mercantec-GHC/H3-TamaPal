// Highscore under the game (message 6 in docs/MESSAGES.md): the backend reads Neon, we only show the lists.
// Live Server (port 5500) → local backend; otherwise the server the page came from (same rule as network.js)
const API = location.port === '5500' ? 'http://localhost:5243/api/scores' : '/api/scores';

// Fills one <ol> with rows; each row is a list of texts that become <span> columns
function fill(listId, rows) {
  const list = document.getElementById(listId);
  list.replaceChildren(); // remove the old rows

  if (rows.length === 0) {
    const li = document.createElement('li');
    li.className = 'empty';
    li.textContent = 'Ingen spil endnu';
    list.append(li);
    return;
  }

  for (const columns of rows) {
    const li = document.createElement('li');
    for (const text of columns) {
      const span = document.createElement('span');
      span.textContent = text; // textContent, not innerHTML: data is shown as text, never as HTML
      li.append(span);
    }
    list.append(li);
  }
}

// "opla-2" → "P2", so it matches P1 / P2 in the game
function player(deviceId) {
  return deviceId.replace('opla-', 'P');
}

// playedAt is UTC from the database → local time, e.g. "12:31"
function time(playedAt) {
  return new Date(playedAt).toLocaleTimeString('da-DK', { hour: '2-digit', minute: '2-digit' });
}

// Called when the page opens and after every game over
export async function refreshScores() {
  try {
    const [top, latest] = await Promise.all([
      fetch(API).then((r) => r.json()),
      fetch(`${API}/latest`).then((r) => r.json()),
    ]);
    fill('top-scores', top.map((r) => [player(r.deviceId), r.score]));
    fill('latest-scores', latest.map((r) => [player(r.deviceId), r.score, r.mode, time(r.playedAt)]));
  } catch (err) {
    // No backend (e.g. fakeGame.js on Live Server) – the game still works, only the lists are empty
    fill('top-scores', []);
    fill('latest-scores', []);
  }
}
