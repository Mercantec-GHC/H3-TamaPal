# Plan og status – TamaPal

Hele projektet på én side: datoer, krav (DoD), status og åbne spørgsmål.
Beskederne mellem delene står i `MESSAGES.md`. Aftaler skrives i Del 3 dér – sammen.

Status: 9/11 (mandag). Ret filen, når noget ændrer sig.

## Datoer

| Dato | Hvad | Kilde |
|---|---|---|
| tor 5/11 | Første test hele vejen: Oplà → backend → browser | vores plan (MESSAGES Del 1c) – ❌ ikke nået |
| fre 6/11 | Brugertest | vores plan – ❌ ikke nået |
| man 9/11 | Kode-stop – derefter kun fejlrettelser | vores plan |
| tir 10/11 | Projektdag: bliv færdige til fremlæggelsen | Notion |
| ons 11/11 | Fremlæggelse – StormLamaerne kl. 10:00–10:30 | Notion |
| tor 12/11 | Release på GitHub (`dev` → `main`, fx `v0.1.0`) | Notion |

## Status pr. del (9/11)

| Del | Hvem | Status |
|---|---|---|
| Arduino | Oleh | ✅ **Færdig** i `dev`. Controller-tilstand sender `up`/`down`/`button`/`tilt` via MQTT (~20/s) og reagerer på `hit`/`miss`/`gameover`. Derudover Tamagotchi-livscyklus, to offline minispil (Pong, Labyrint) og autonom showcase (se `MESSAGES.md`). Kompilerer (flash 88 %, RAM 28 %), 533/533 enhedstests grønne (målt 9/11). |
| Backend | Oliver | ⚠️ **Delvist.** I `dev`: SignalR-hub `/gamehub` med `StartGame()`, CORS for Live Server, `GameService` (holder `GameState`, `Start()`), `GameLoop` der sender `GameState` ~30/s. **Mangler:** spillogik i loopet, MQTT-lytter, events til Oplà, database – se "Mangler til fremlæggelsen". |
| Frontend | Andrii | ✅ **Færdig** i `dev`. SignalR-klient (#10) testet mod `/gamehub` 7/11 og 9/11 (forbinder, `StartGame()`, genforbinder). Neon arkade-look med effekter ved slag/miss. `fakeGame.js` som backup. Venter kun på, at backenden flytter bolden. |

## Mangler til fremlæggelsen (11/11)

Bolden står stille i browseren, fordi `GameLoop` sender den samme `GameState` hver gang – intet flytter den.

**Backend (Oliver) – i denne rækkefølge:**
- [ ] `GameService.Update(dt)`: bold, vægge, ketsjer, point, liv, `gameover` (samme regler som `Frontend/fakeGame.js`) og kald den i `GameLoop` før `SendAsync` (#26, #28)
- [ ] MQTT-lytter (`MQTTnet` er allerede tilføjet): abonnér på `StormLamaerne/Tamapal/+/input`, sæt ketsjerens retning ud fra `up`/`down` (-1/0/1) (#27)
- [ ] Send `{"type":"hit"|"miss"|"gameover"}` til `.../opla-1/event` (#29)
- [ ] Database (Neon + EF Core): mindst én tabel med highscore efter game over (#30) – "løst krav" fra Mathias

**Alle:**
- [ ] Integrationstest Oplà → backend → browser (#19)
- [ ] Brugertest med én uden for gruppen + én forbedring (#18)
- [ ] Gennemgå Del 3 i `MESSAGES.md` sammen – kun godkendt af Oleh indtil nu
- [ ] Release `dev` → `main` (#17)

**Plan B til demoen:** hvis backenden ikke er klar, skift importen i `Frontend/main.js` til `./fakeGame.js` (spil med piletaster) og vis Oplàs controller i MQTTX/MQTT Explorer + Olehs showcase.

## Definition of Done – sådan opfylder vi den

| # | Krav (kort) | Sådan gør vi | Hvem | Status |
|---|---|---|---|---|
| 1 | Mini-GDD | `TamaPal-game-design-doc.docx` – fuldt dækket med Pong-MVP, Tamagotchi-livscyklus, sensorer og bær | Oleh | ✅ |
| 2 | Gameplay loop | Ram bolden → +1 point og hurtigere bold; forbi → -1 liv | alle | ✅ designet |
| 3 | Start, spil, slut | `status`: `waiting` → `playing` → `gameover`, "Spil igen" | Oliver + Andrii | ⚠️ frontend + hub klar; backend skifter kun `waiting` → `playing` |
| 4 | Mindst 2 handlinger på Oplà | Op/ned (knapper eller tilt) + server/start + sensorer (nærhedsklap, rystevask, temperatur, lys) | Oleh | ✅ 100% implementeret & testet |
| 5 | Protokol Arduino → backend | MQTT, topics i `MESSAGES.md` (universal input, events, pet/state, robust JSON, 1.5s timeout, TLS/MQTTS støtte) | Oleh + Oliver | ✅ Oplà → broker testet 4/11. Backend lytter ikke endnu |
| 6 | Backend med spillogik | Game loop: bold, point, liv, regler | Oliver | ❌ loopet sender, men flytter intet |
| 7 | Frontend med feedback | Canvas, score, liv, start- og game over-skærm | Andrii | ✅ på falske data og forbundet til `/gamehub` |
| 8 | Integration + forklare dataflow | Test af samlet flow: Oplà → MQTT → Backend → SignalR → Browser | alle | ❌ mangler backend-integration |
| 9 | Brugertest + 1 forbedring | En uden for gruppen tester, vi retter én ting | alle | ⚠️ kan testes via Oplàs autonome showcase eller fakeGame.js |
| 10 | Kode på GitHub, alle bidrager, demo | Alle pusher til `dev`, release `dev` → `main` | alle | ⚠️ |

**Database:** Mathias kalder det et "løst krav" (3/11) – der skal være en database koblet på.
I DoD står den under Nice to Have. Forslag: highscore (navn + score) samt forfædre på kirkegården fra `pet/state`.

**Nice to Have, vi allerede har i planen:** MQTT, SignalR, Arduino-feedback (LED/lyd), avancerede sensorer (tilt, APDS-9960, ryst, temperatur, lys), offline minispil på Oplà, autonom showcase.

## Nyt fra underviserne (3–4/11)

- **Docker er ikke et krav** i mellemprojektet (Mathias 3/11). Det bliver krav i næste, store projekt.
- Neon er ok: én fælles database til hele gruppen. Mathias anbefaler .NET 10 (LTS) og PostgreSQL med EF Core.
- **Mathias har en MQTT-broker**, vi kan bruge – login står i Teams. Topic skal starte med gruppens navn (`StormLamaerne/`).
- **Multiplayer er Nice to Have** – ikke et krav (Notion, DoD).
- Kaspers Pong-eksempler (4/11): serveren afgør alt, browseren tegner kun. Input er op/stop/ned = -1/0/1.
  Til browserspil anbefaler han WebSocket – SignalR bygger på WebSocket.

## Sådan kører vi det

1. **Afvikl alle enhedstests på Mac/Linux:**
   ```bash
   bash Arduino/tests/run_tests.sh
   # Resultat: 512 of 512 tests passed!
   ```
2. **Kompilering af Oplà-firmware (SAMD21):**
   ```bash
   "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" compile --warnings all --fqbn arduino:samd:mkrwifi1010 Arduino/TamaPal
   ```
3. **Optagelse af videopræsentation (Autonom Showcase):**
   Sæt Oplà i holderen, tænd kameraet og hold **TOUCH 1 + TOUCH 3 nede i 800 ms** (eller send `'s'` i Serial Monitor ved 115200 baud). Hele spillets univers afvikles 100% automatisk over 70 sekunder uden hænder i billedet!
4. **MQTT-broker:** ekstern broker, port 1883, uden login (eller MQTTS port 8883 med TLS).
   Topics: `StormLamaerne/Tamapal/opla-1/input`, `.../event` og `.../pet/state` (se `MESSAGES.md`).
5. **Backend:** `dotnet run --launch-profile http` i `Backend/BackendTamapal/` → `http://localhost:5243`. Hub: `http://localhost:5243/gamehub`.
6. **Frontend:** Live Server på `Frontend/` (port 5500 – backenden tillader CORS fra `localhost:5500` og `127.0.0.1:5500`).
   Uden backend: skift importen i `Frontend/main.js` til `./fakeGame.js`.

## Åbne spørgsmål til mødet

1. Styring: knapper eller tilt? Olehs kode sender begge i samme payload (`up`, `down`, `tiltX`, `tiltY`, `tilt`).
2. Hvornår er `GameService.Update()` og MQTT-lytteren klar? Skal Oleh eller Andrii hjælpe?
3. Hvem tester spillet til brugertesten?
