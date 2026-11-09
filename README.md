# 🐣 TamaPal

A modern Tamagotchi that lives on an **Arduino Oplà (MKR IoT Carrier)** and reacts
to the real world: cold, darkness and movement. The Oplà is also a **game controller**:
tilt it to play Pong in the **browser**, with the game logic running on a .NET backend.

H3 middle project (Mercantec, autumn 2026). Full game design:
[docs/TamaPal-game-design-doc.docx](docs/TamaPal-game-design-doc.docx).

---

## What works (9/11)

- **Pong, the whole way:** tilt the Oplà → the paddle moves in the browser. The backend moves the
  ball, counts points and lives and ends the game. The Oplà button (T2) or the browser button starts it.
- **On the Oplà alone (offline):** the pet Lumi with 5 needs, life stages, wardrobe, cemetery,
  two mini games (Pong, Labyrinth) and a hands-free showcase for video.
- **Not done yet:** `hit`/`miss`/`gameover` feedback to the Oplà (#29), highscore in the database (#30).

Status, checklist and dates: [docs/PLAN.md](docs/PLAN.md).

---

## Architecture

```
┌─────────────┐  MQTT input (~20/s)  ┌──────────────────────┐  SignalR GameState (~30/s) ┌─────────────┐
│ Arduino     │ ───────────────────▶ │  Backend (.NET 10)   │ ─────────────────────────▶ │  Browser    │
│ Oplà        │ ◀─────────────────── │  game loop + logic   │ ◀───────────────────────── │  canvas     │
│ (controller)│  MQTT events         │                      │  StartGame()               │             │
└─────────────┘                      └──────────────────────┘                            └─────────────┘
```

- **Arduino** – controller mode sends `tilt`/`button` over MQTT; plus the whole pet on the Oplà.
- **Backend** – `MqttWorker` listens to `StormLamaerne/Tamapal/+/input`, `GameService` holds the rules,
  `GameLoop` sends `GameState` to the browser ~30 times a second through `GameHub` (`/gamehub`).
- **Frontend** – only draws the `GameState` it receives and sends `StartGame()`. No game logic.

The browser never talks to the Oplà directly. All messages: [docs/MESSAGES.md](docs/MESSAGES.md).

---

## How to run

**1. Backend** (`Backend/BackendTamapal/`, .NET 10)

The MQTT broker address is not in git. First time on a new computer (address: ask the group or see
`Arduino/TamaPal/arduino_secrets.h`):

```bash
cd Backend/BackendTamapal
dotnet user-secrets set "Mqtt:Host" "<broker-ip>"
dotnet user-secrets set "Mqtt:Port" "1883"
```

Then start it – the log should say `MQTT forbundet og lytter på input`:

```bash
dotnet run --launch-profile http
```

Runs on `http://localhost:5243`, hub at `/gamehub`.

**2. Frontend** (`Frontend/`, plain JS)

Open `Frontend/` with **Live Server** in VS Code (port 5500) → `http://localhost:5500`.
The backend allows CORS from `localhost:5500` and `127.0.0.1:5500`.

No backend? Change the import in `Frontend/main.js` from `./network.js` to `./fakeGame.js`
and play with the arrow keys (do not commit that change).

**3. Oplà** (`Arduino/TamaPal/`)

Copy `arduino_secrets.example.h` to `arduino_secrets.h`, fill in Wi-Fi and broker, and upload
`TamaPal.ino` (board: Arduino MKR WiFi 1010). On the Oplà: **T0 → T2 → T0** opens controller mode.
User guide for the buttons: [docs/BRUGERVEJLEDNING.md](docs/BRUGERVEJLEDNING.md).

Arduino unit tests on Mac/Linux: `bash Arduino/tests/run_tests.sh`

---

## Project structure

```
H3-TamaPal/
├── Arduino/
│   ├── TamaPal/              # Oplà: pet, menu, controller mode (MQTT), mini games, showcase
│   ├── TamagotchiMaxDemo/    # Graphics demo: what the Oplà can draw with code only
│   └── tests/                # unit tests for the logic files
├── Backend/BackendTamapal/   # .NET 10: GameHub (SignalR), GameService, GameLoop, MqttWorker
├── Frontend/                 # Pong on canvas: main.js, draw.js, network.js (+ fakeGame.js as backup)
└── docs/
    ├── PLAN.md               # dates, Definition of Done, status, what is missing
    ├── MESSAGES.md           # contract: the messages between the parts + group agreement
    ├── BRUGERVEJLEDNING.md   # user guide for the Oplà
    ├── MOEDE-2026-11-02.md   # teachers' workflow and requirements
    ├── TIL-DISKUSSION.md     # archive: first questions + Oliver's game loop
    └── TamaPal-game-design-doc.docx
```

**No secrets in git:** Wi-Fi and broker go in `arduino_secrets.h` (git-ignored),
backend settings in `dotnet user-secrets`.

---

## Team

| Person | Area |
|---|---|
| Oleh | Arduino – the pet, visuals, sensors, menu, controller mode |
| Oliver | Backend – game loop, game logic, MQTT, SignalR, database |
| Andrii | Frontend – canvas, start/game over screens, SignalR client |

Workflow: push to `dev`, `main` only via Pull Request ([docs/MOEDE-2026-11-02.md](docs/MOEDE-2026-11-02.md)).
