# MVP og beskeder – til diskussion

Dokumentet har fire dele:

1. **Del 1 – Forslag fra Andrii (2/11):** MVP og dataflow for Pong-spillet.
2. **Del 1b – Forslag fra Oleh (2/11):** Oplà som kæledyr + controller, 5 behov, nye MQTT-beskeder, sensorer og 16 GB SD-kort.
3. **Del 2 – Spørgsmål:** Det vi skal blive enige om.
4. **Del 3 – Aftale:** Udfyldes sammen, når vi er enige. Det er den, vi arbejder efter.

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

# Del 1b – Forslag og observationer fra Oleh (Arduino)

Her er mine forslag til projektet efter at have læst vores Game Design Doc (GDD), lærernoterne fra 2/11 og beskederne. 
Formålet er, at Arduino, backend og frontend passer 100% sammen fra start. Alt er forklaret helt enkelt med **hvad det er**, **konsekvens** og et **eksempel**.

---

### 1. MVP: Oplà har to tilstande (Kæledyr + Controller)
* **Hvad er problemet?**  
  I vores GDD er hovedidéen en Tamagotchi på Oplà-skærmen, hvor dyret spiser, sover, fryser og vokser. Men i forslaget til MVP (Del 1) er Oplà kun en controller for en ketsjer i browseren.
* **Mit forslag:**  
  Oplà får to tilstande i koden:
  1. `Kæledyrs-tilstand (Pet Mode)`: Lumi bor på skærmen med menu og behov.
  2. `Controller-tilstand (Pong Mode)`: Når spilleren vælger ikonet "Forbind/Spil" i menuen, skifter Oplà og sender `tilt` og `button` til Pong.
* **Konsekvens:**  
  Hvis vi kun bygger en Pong-controller, glemmer vi hele Tamagotchi-spillet fra vores GDD. Med to tilstande har vi begge dele.
* **Eksempel:**  
  Spilleren passer Lumi på Oplà. Når spilleren vælger "Forbind", viser Oplà en lille ketsjer på skærmen, og man kan vippe den for at spille Pong i browseren.

---

### 2. Kæledyret skal have 5 behov (ikke kun 3)
* **Hvad er problemet?**  
  Vores nuværende grafiske demo viser kun 3 bjælker, men ifølge GDD skal Lumi have **5 behov**:
  1. **Sult** (`Mad` via menuen)
  2. **Varme** (reagerer på kulde og håndvarme via temperatursensoren)
  3. **Søvn / Energi** (reagerer på mørke via lyssensoren)
  4. **Glæde** (stiger når man leger eller vinder Pong)
  5. **Disciplin** (vigtigt: stiger når man bruger "Skæld ud", hvis Lumi klager uden grund)
* **Mit forslag:**  
  Vi laver de rigtige 5 behov på Arduinoen. Ikonerne i toppen styrer pasningen, og man kan åbne en statistik-skærm, der viser de 5 bjælker.
* **Konsekvens:**  
  Hvis vi dropper disciplin og varme, virker temperatursensoren og skæld-ud-mekanikken slet ikke i spillet.
* **Eksempel:**  
  Lumi klager og piber, selvom den er helt mæt. Spilleren vælger "Skæld ud" i menuen. Lumi ser fornærmet ud i 2 sekunder, men dens disciplin stiger.

---

### 3. Nye MQTT-beskeder: Gem kæledyret og send til kirkegård
* **Hvad mangler?**  
  I Del 1 er der kun beskeder for Pong (`input` og `event`). Men i `TIL-DISKUSSION.md` står der, at kæledyrets tilstand skal gemmes i databasen, og når kæledyret dør, skal det vises på kirkegården på hjemmesiden!
* **Mit forslag:**  
  Vi tilføjer 2 enkle beskeder:

  **A. Arduino → Backend (hvert 30. sekund og ved død/stadieskift):**  
  Topic: `tamapal/opla-1/pet/state`
  ```json
  {
    "name": "Lumi",
    "stage": "teen",
    "hunger": 80,
    "warmth": 90,
    "energy": 70,
    "happy": 85,
    "discipline": 60,
    "alive": true
  }
  ```
  *(Hvis Lumi dør, sendes `"alive": false` og fx `"cause": "cold"`).*

  **B. Backend → Arduino (belønning efter minispil):**  
  Topic: `tamapal/opla-1/pet/reward`
  ```json
  { "type": "happy_boost", "amount": 25 }
  ```
* **Konsekvens:**  
  Uden disse beskeder kan Oliver (backend) ikke gemme Lumi i databasen (Neon PostgreSQL), og Andrii (frontend) kan ikke vise den døde Lumi på kirkegården.
* **Eksempel:**  
  Hvis Lumi dør af kulde, modtager backend beskeden og gemmer den i databasen. Hjemmesiden viser straks en ny gravsten på kirkegården med teksten "Lumi frøs ihjel".

---

### 4. Sensorer i virkeligheden og demo-tid (Varme og Lys)
* **Hvad er problemet?**  
  1. Oplà-kassen bliver naturligt varm af processoren (+3 til 5 grader ekstra). Derfor vil den næsten aldrig fryse i et normalt varmt klasselokale.
  2. GDD siger, at lyssensoren skal være dækket i 1 minut for at sove. 1 minut er alt for lang tid under en fremlæggelse for lærerne.
* **Mit forslag:**  
  - **Varme:** Arduino måler start-temperaturen. Når man lægger varme hænder om Oplàen, registrerer den opvarmning. Vi laver også en test-knap til fremlæggelsen, som kan tvinge "kulde" frem, så vi kan vise det på 3 sekunder.
  - **Lys:** I demo-tilstand skal Lumi falde i søvn efter **5 sekunder** i mørke (ikke 60 sekunder).
* **Konsekvens:**  
  Hvis vi ikke tilpasser tiderne, kan vi ikke nå at demonstrere sensorerne for lærerne til fremlæggelsen.
* **Eksempel:**  
  Til fremlæggelsen dækker vi lyssensoren med hånden. Efter 5 sekunder lukker Lumi øjnene, og der kommer en lille dyne over ham.

---

### 5. 16 GB MicroSD-kort og sikkerhed på Arduino
* **Hvad har vi og hvorfor?**  
  Jeg har sat et 16 GB MicroSD-kort i Oplàen. Arduinoen har kun 32 KB RAM og glemmer alt, hvis strømmen forsvinder.
* **Mit forslag:**  
  - Kæledyrets data gemmes sikkert på SD-kortet hvert 30. sekund (i en lille fil med kontrolsum).
  - Hvis man tager USB-kablet ud og sætter det i igen, dør Lumi ikke. Han starter præcis, hvor han slap!
  - **Sikkerhed:** Wi-Fi koder og kodeord ligger KUN i `arduino_secrets.h` (den er skjult i `.gitignore` og kommer aldrig på GitHub).
* **Konsekvens:**  
  Uden SD-kort vil Oplà genstarte forfra som nyfødt baby hver gang USB-stikket rykkes ud.
* **Eksempel:**  
  Du flytter Oplà over til en anden computer. Når du sætter strøm til, indlæser den din teenager-Lumi fra SD-kortet på under 1 sekund.

---

### 6. Git-branches: Vi følger lærernes krav (`feature/...`)
* **Hvad er problemet?**  
  I `TIL-DISKUSSION.md` stod der `arduino/...`, men lærerne (Mathias og Kasper) bad os på mødet 2/11 om altid at bruge `feature/...`.
* **Mit forslag:**  
  Vi bruger alle `feature/` som præfiks for vores opgaver:
  - `feature/arduino-menu` (Oleh)
  - `feature/backend-mqtt` (Oliver)
  - `feature/frontend-pong` (Andrii)
* **Konsekvens:**  
  Lærerne kan se vores opgaver direkte på GitHub Projects boardet, og vi opfylder deres faste krav.
* **Eksempel:**  
  Når jeg laver menuen på Oplà, opretter jeg en branch med navnet `feature/arduino-menu`.

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
