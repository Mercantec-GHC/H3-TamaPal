# 🐣 TamaPal

A modern Tamagotchi that lives on an **Arduino Oplà (MKR IoT Carrier)** and reacts
to the real world: cold, darkness and movement. When two TamaPals connect, they
move from the Oplà screen into the **browser** and play mini games together.

H3 middle project (Mercantec, autumn 2026). Full game design:
[docs/TamaPal-game-design-doc.docx](docs/TamaPal-game-design-doc.docx).

---

## Planned architecture (draft)

```
┌─────────────┐  MQTT / HTTP   ┌──────────────────────┐  SignalR   ┌─────────────┐
│ Arduino     │ ─────────────▶ │  Backend (.NET API)  │ ─────────▶ │  Browser    │
│ Oplà        │ ◀───────────── │  + database          │ ◀───────── │  mini games │
│ (the pet)   │    events      │  (game logic, saves) │   input    │             │
└─────────────┘                └──────────────────────┘            └─────────────┘
```

- **Arduino** – the pet: drawing, needs, growth, sensors (temperature, light, IMU), menu.
- **Backend** – connection between devices and browser, game logic for mini games,
  saves pet state and match results in the database.
- **Frontend** – meeting place for two TamaPals, ping-pong and maze race.

Proposal: MQTT between Oplà and backend, SignalR between backend and browser.
The first mini game is Pong. Message format: [docs/MESSAGES.md](docs/MESSAGES.md).

---

## Project structure

```
H3-TamaPal/
├── Arduino/
│   ├── TamaPal/              # Oplà: menu, controller mode, MQTT input, LED/sound
│   ├── TamagotchiMaxDemo/    # Graphics demo: what the Oplà can draw with code only
│   └── tests/
├── Backend/                  # .NET Web API + SignalR + database (coming)
├── Frontend/                 # Pong on canvas (plain JS, fake data until the backend is ready)
└── docs/
    ├── MESSAGES.md           # contract: the messages between the parts + group agreement
    ├── PLAN.md               # dates, Definition of Done, status, open questions
    ├── MOEDE-2026-11-02.md   # teachers' workflow and requirements
    ├── TIL-DISKUSSION.md     # archive: first questions + Oliver's game loop
    └── TamaPal-game-design-doc.docx
```

Wi-Fi passwords go in `config.h` / `arduino_secrets.h` – these files are
git-ignored and must never be committed.

---

## Team

| Person | Area |
|---|---|
| Oleh | Arduino – the pet, visuals, sensors, menu |
| Oliver | Backend and database |
| Andrii | Frontend in the browser – mini games, statistics |

Start here:
1. [docs/MESSAGES.md](docs/MESSAGES.md) – the messages between the parts (MQTT/SignalR) and our agreement.
2. [docs/PLAN.md](docs/PLAN.md) – dates, requirements, status and open questions.
3. [docs/MOEDE-2026-11-02.md](docs/MOEDE-2026-11-02.md) – workflow: push to `dev`, `main` only via Pull Request.
