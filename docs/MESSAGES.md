# Beskeder – forslag til MVP (Pong mod væggen)

Forslag fra Andrii 2/11 – til diskussion og rettelser i gruppen.

Princip: så få beskeder som muligt og ingen ekstra felter.

## MVP

- **Én Oplà** styrer én ketsjer i venstre side. Bolden hopper af de tre andre vægge.
- **Handling 1:** man vipper Oplàen (IMU), og ketsjeren bevæger sig op og ned.
- **Handling 2:** knap = server bolden / start spillet.
- **Point:** +1 hver gang man rammer bolden. Bolden bliver lidt hurtigere for hvert slag.
- **Liv:** 3 liv. Rammer man forbi, mister man et liv. Ved 0 liv er spillet slut, og scoren vises.
- Nem at udvide senere: highscore i databasen eller en spiller nummer 2.

## Projektflow

### Dataflow

```
            1. input (tilt, knap)                3. GameState (30/s)
┌─────────┐ ─────── MQTT ───────► ┌───────────┐ ──── SignalR ────► ┌──────────┐
│  Oplà   │                       │  Backend  │                    │ Browser  │
│ (input) │ ◄────── MQTT ──────── │ (logik)   │ ◄─── SignalR ───── │ (canvas) │
└─────────┘   2. event (hit/miss) └───────────┘   4. StartGame()   └──────────┘
```

### Et spil fra start til slut

1. Browseren viser "Start" (`status: waiting`).
2. Spilleren trykker Start i browseren eller på Oplàens knap → backend starter spillet.
3. Oplàen sender `tilt` 20 gange i sekundet → backend flytter ketsjeren.
4. Backend flytter bolden, tjekker vægge og ketsjer 30 gange i sekundet og sender `GameState`.
5. Ramt → +1 point, `hit` til Oplàen. Forbi → -1 liv, `miss` til Oplàen.
6. 0 liv → `status: gameover`, `gameover` til Oplàen, browseren viser scoren og "Spil igen".

### Rækkefølge for arbejdet

1. **Hver for sig med falske data:** Andrii tegner et opdigtet `GameState`, Oliver laver game loop
   med falsk `tilt`, Oleh sender `tilt` og ser det i MQTT Explorer.
2. **Første forbindelse:** Oplà → MQTT → backend (backend logger `tilt`).
3. **Hele vejen:** Oplà → backend → SignalR → browser. Ketsjeren på skærmen følger Oplàen.
4. **Spillet færdigt:** point, liv, game over, `hit`/`miss` på Oplàen.
5. **Brugertest** med en uden for gruppen + mindst én forbedring.

## Kun 4 beskeder

### 1. Arduino → backend (MQTT), 20 gange i sekundet

Topic: `tamapal/opla-1/input`

```json
{ "tilt": 0.35, "button": false }
```

`tilt` går fra -1 (op) til 1 (ned). Arduinoen læser bare IMU og sender.

### 2. Backend → Arduino (MQTT), kun ved hændelser

Topic: `tamapal/opla-1/event`

```json
{ "type": "hit" }
```

Typer: `hit` (ramt), `miss` (mistet et liv), `gameover`. Oplàen blinker eller bipper.

### 3. Backend → browser (SignalR), 30 gange i sekundet

Hub: `/gamehub`, event `GameState`

```json
{
  "status": "playing",
  "ball":   { "x": 0.5, "y": 0.3 },
  "paddle": { "y": 0.4 },
  "score": 7,
  "lives": 2
}
```

Alle koordinater er mellem 0 og 1. `status` kan være `waiting`, `playing` eller `gameover`.

### 4. Browser → backend (SignalR)

Metode `StartGame()` – knappen "Start" eller "Spil igen".

## Hvem laver hvad

| | Opgave |
|---|---|
| **Oleh** | IMU + knap → besked 1, LED/lyd ved besked 2 |
| **Oliver** | game loop 30 gange i sekundet: bold, vægge, ketsjer, point, liv → besked 3; modtager 1, sender 2 |
| **Andrii** | tegner besked 3 på canvas, Start-knap → besked 4, game over-skærm med score |

## Hvorfor det virker

- **Én besked pr. retning.** Nemt at forstå og debugge.
- **Al logik ligger i backend** og opfylder DoD-punkt 6 (point, liv og regler i backend).
- **Kun én Oplà.** Ingen problemer med to enheder eller pairing, og man kan teste alene.
- **Alle kan starte i dag med falske data.**
