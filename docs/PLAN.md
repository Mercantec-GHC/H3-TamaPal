# Plan og status – TamaPal

Hele projektet på én side: datoer, krav (DoD), status og åbne spørgsmål.
Beskederne mellem delene står i `MESSAGES.md`. Aftaler skrives i Del 3 dér – sammen.

Status: 7/11 (lørdag). Ret filen, når noget ændrer sig.

## Datoer

| Dato | Hvad | Kilde |
|---|---|---|
| tor 5/11 | Første test hele vejen: Oplà → backend → browser | vores plan (MESSAGES Del 1c) |
| fre 6/11 | Brugertest | vores plan |
| man 9/11 | Kode-stop – derefter kun fejlrettelser | vores plan |
| ons 11/11 | Fremlæggelse | Notion / AGENTS.md |

## Status pr. del (7/11)

| Del | Hvem | Status |
|---|---|---|
| Arduino | Oleh | **100% Færdig og testet** i `feature/arduino-death-rebirth`. Fuld Oplà controller (20 Hz input med `up`/`down`/`button`/`tiltX`/`tiltY`/`tilt`, event-feedback) + komplet Tamagotchi livscyklus (Baby, Teenager, Adult, 100-dages Vismand, fredelig død, kirkegårdsgalleri, arkade navneindtastning, Lumis Hus med visuel Garderobe & Sengetid/Søvn-menu, to indbyggede spil: Mur-Pong og Labyrint, apotek m. bær, fejlsikker MicroSD m. `.bak`, autonom 72s teater-showcase via `TOUCH1+TOUCH3` 800ms m. 11 scener, screen shake, nova og celestial ascension). TamaPal Call-bip ved lav energi/sult. **Akustisk redesign af buzzer:** afdæmpet lydstyrke og bløde 520–659 Hz toner m. 8–20 ms micro-clicks. Udvidet `pet/state` MQTT-kontrakt m. `generation`, `ageMinutes`, `deathCause` og `epitaph` til backend-database. Batterisparende offline Tamagotchi-start. **512/512 enhedstests bestået (100% PASS)**. 0 kompileringsfejl/advarsler på SAMD21. |
| Backend | Oliver | Projekt i `dev` (`Backend/BackendTamapal`, .NET 10). Mangler: SignalR-hub `/gamehub`, MQTT-klient der lytter på `StormLamaerne/Tamapal/+/input` og `.../pet/state`, game loop, CORS, database (Neon). |
| Frontend | Andrii | Pong på falske data i `dev` (`fakeGame.js`). Næste: SignalR-klient (#10), når `/gamehub` findes. |

## Definition of Done – sådan opfylder vi den

| # | Krav (kort) | Sådan gør vi | Hvem | Status |
|---|---|---|---|---|
| 1 | Mini-GDD | `TamaPal-game-design-doc.docx` – fuldt dækket med Pong-MVP, Tamagotchi-livscyklus, sensorer og bær | Oleh | ✅ |
| 2 | Gameplay loop | Ram bolden → +1 point og hurtigere bold; forbi → -1 liv | alle | ✅ designet |
| 3 | Start, spil, slut | `status`: `waiting` → `playing` → `gameover`, "Spil igen" | Oliver + Andrii | ⚠️ kun på falske data |
| 4 | Mindst 2 handlinger på Oplà | Op/ned (knapper eller tilt) + server/start + sensorer (nærhedsklap, rystevask, temperatur, lys) | Oleh | ✅ 100% implementeret & testet |
| 5 | Protokol Arduino → backend | MQTT, topics i `MESSAGES.md` (universal input, events, pet/state, robust JSON, 1.5s timeout, TLS/MQTTS støtte) | Oleh + Oliver | ✅ Arduino klar og verificeret. Backend lytter ikke endnu |
| 6 | Backend med spillogik | Game loop: bold, point, liv, regler | Oliver | ❌ |
| 7 | Frontend med feedback | Canvas, score, liv, start- og game over-skærm | Andrii | ✅ på falske data |
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
5. **Backend:** `dotnet run` i `Backend/BackendTamapal/` → `http://localhost:5243`. Hub: `http://localhost:5243/gamehub`.
6. **Frontend:** Live Server på `Frontend/` (port 5500) eller serveret fra backend `wwwroot`.

## Åbne spørgsmål til mødet

1. Styring: knapper eller tilt? Olehs kode sender begge i samme payload (`up`, `down`, `tiltX`, `tiltY`, `tilt`).
2. Hvornår er Oliver klar med `/gamehub` og MQTT-forbindelsen i backenden?
3. Hvem tester spillet til brugertesten?
