# MVP og beskeder – til diskussion

Dokumentet har tre dele:

1. **Forslag** (fra Andrii 2/11) – et udgangspunkt, ikke en beslutning.
2. **Spørgsmål** – det vi skal blive enige om.
3. **Aftale** – udfyldes sammen, når vi har besluttet. Det er den, vi arbejder efter.

Alle må rette og kommentere. Princip: så få beskeder som muligt og ingen ekstra felter.

---

# Del 1 – Forslag

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

---

# Del 2 – Spørgsmål til diskussion

**Spillet**

1. MVP: Pong mod væggen med én Oplà – eller to spillere fra start?
2. Styring: vippe (IMU) – eller knapper op/ned? Hvad gør knappen?
3. Regler: 3 liv? Skal bolden blive hurtigere? Hvornår slutter spillet?
4. Hvad sker der efter MVP: highscore, spiller 2, lobby, kirkegård, labyrint?

**Kommunikation**

5. MQTT mellem Oplà og backend – eller HTTP? Hvilken broker (Mosquitto lokalt, skolens)?
6. Er topic-navnene og JSON-felterne i Del 1 ok? Mangler der noget?
7. Hvor ofte sender Oplàen input (20/s?), og hvor ofte sender backend `GameState` (30/s?)?
8. Skal Oplàen reagere på `hit`/`miss`/`gameover` (LED/lyd)?

**Teknik og arbejde**

9. Hvem styrer bolden: backend (forslag) eller browseren?
10. Frontend i backendens `wwwroot` eller separat (CORS)?
11. Database fra start (Neon) – eller først efter MVP?
12. Branch-navne `feature/...` og 1 eller 2 godkendelser på Pull Requests?
13. Datoer: første test hele vejen igennem, brugertest, "kode-stop" før 11/11?

---

# Del 3 – Aftale

Udfyldes sammen. Når vi er enige, skriver vi navn og dato nederst.

| # | Emne | Beslutning |
|---|---|---|
| 1 | MVP | |
| 2 | Styring | |
| 3 | Regler | |
| 5 | Protokol og broker | |
| 6 | Topics og JSON | |
| 7 | Frekvens | |
| 8 | Feedback på Oplà | |
| 9 | Hvem styrer bolden | |
| 10 | Hvor ligger frontend | |
| 11 | Database | |
| 12 | Branches og reviews | |
| 13 | Datoer | |

**Godkendt af:**

- [ ] Oleh – dato:
- [ ] Oliver – dato:
- [ ] Andrii – dato:

Ændringer efter aftalen: ret både Del 1 og tabellen her, og skriv det i gruppens chat.
