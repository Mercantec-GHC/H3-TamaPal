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

Protocol (MQTT or HTTP) and message format are still to be decided.

---

## Project structure

```
H3-TamaPal/
├── Arduino/
│   └── TamagotchiMaxDemo/    # Graphics demo: what the Oplà can draw with code only
├── Backend/                  # .NET Web API + database (coming)
├── Frontend/                 # Browser mini games (coming)
└── docs/
    └── TamaPal-game-design-doc.docx
```

Wi-Fi passwords go in `config.h` / `arduino_secrets.h` – these files are
git-ignored and must never be committed.

---

## Team

| Person | Area (draft) |
|---|---|
| Oleh | Arduino – the pet and visuals |
| Andrii | Backend, database, frontend mini games |
| Oliver | Backend, database, frontend mini games |

Roles and plan are a draft – everything can be discussed.
