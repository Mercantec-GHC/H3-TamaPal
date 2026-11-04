# Plan og status – TamaPal

Hele projektet på én side: datoer, krav (DoD), status og åbne spørgsmål.
Beskederne mellem delene står i `MESSAGES.md`. Aftaler skrives i Del 3 dér – sammen.

Status: 4/11 (onsdag). Ret filen, når noget ændrer sig.

## Datoer

| Dato | Hvad | Kilde |
|---|---|---|
| tor 5/11 | Første test hele vejen: Oplà → backend → browser | vores plan (MESSAGES Del 1c) |
| fre 6/11 | Brugertest | vores plan |
| man 9/11 | Kode-stop – derefter kun fejlrettelser | vores plan |
| ons 11/11 | Fremlæggelse | Notion / AGENTS.md |

## Status pr. del (4/11)

| Del | Hvem | Status |
|---|---|---|
| Arduino | Oleh | Menu + controller i `dev`. Sender input via MQTT og reagerer på `hit`/`miss`/`gameover`. |
| Backend | Oliver | Ikke startet i repo endnu (går i gang 4/11, database i Neon). |
| Frontend | Andrii | Pong på falske data i `dev` (`fakeGame.js`). Næste: SignalR-klient (#10), når `/gamehub` findes. |

## Definition of Done – sådan opfylder vi den

| # | Krav (kort) | Sådan gør vi | Hvem | Status |
|---|---|---|---|---|
| 1 | Mini-GDD | `TamaPal-game-design-doc.docx` – tjek at det passer med Pong-MVP | Oleh | ⚠️ |
| 2 | Gameplay loop | Ram bolden → +1 point og hurtigere bold; forbi → -1 liv | alle | ✅ designet |
| 3 | Start, spil, slut | `status`: `waiting` → `playing` → `gameover`, "Spil igen" | Oliver + Andrii | ⚠️ kun på falske data |
| 4 | Mindst 2 handlinger på Oplà | Op/ned (knapper eller tilt) + server/start | Oleh | ✅ i koden, ikke testet med backend |
| 5 | Protokol Arduino → backend | MQTT, topics i `MESSAGES.md` | Oleh + Oliver | ⚠️ broker ikke valgt |
| 6 | Backend med spillogik | Game loop: bold, point, liv, regler | Oliver | ❌ |
| 7 | Frontend med feedback | Canvas, score, liv, start- og game over-skærm | Andrii | ✅ på falske data |
| 8 | Integration + forklare dataflow | Test 5/11, diagram i `MESSAGES.md` | alle | ❌ |
| 9 | Brugertest + 1 forbedring | En uden for gruppen tester, vi retter én ting | alle | ❌ |
| 10 | Kode på GitHub, alle bidrager, demo | Alle pusher til `dev`, release `dev` → `main` | alle | ⚠️ |

**Database:** Mathias kalder det et "løst krav" (3/11) – der skal være en database koblet på.
I DoD står den under Nice to Have. Forslag: kun highscore (navn + score) efter game over.

**Nice to Have, vi allerede har i planen:** MQTT, SignalR, Arduino-feedback (LED/lyd), avancerede sensorer (tilt).

## Nyt fra underviserne (3–4/11)

- **Docker er ikke et krav** i mellemprojektet (Mathias 3/11). Det bliver krav i næste, store projekt.
- Neon er ok: én fælles database til hele gruppen. Mathias anbefaler .NET 10 (LTS) og PostgreSQL med EF Core.
- **Mathias har en MQTT-broker**, vi kan bruge – login står i Teams. Topic skal starte med gruppens navn.
- **Multiplayer er Nice to Have** – ikke et krav (Notion, DoD).
- Kaspers Pong-eksempler (4/11): serveren afgør alt, browseren tegner kun. Input er op/stop/ned = -1/0/1.
  Til browserspil anbefaler han WebSocket – SignalR bygger på WebSocket.

## Sådan kører vi det (udkast – udfyldes til testen 5/11)

1. MQTT-broker: Mathias' broker **eller** Mosquitto på Olivers bærbare (port 1883). – *ikke besluttet*
2. Backend: `dotnet run` i `Backend/` – port: *?*
3. Frontend: Live Server på `Frontend/` (port 5500) → backenden skal tillade CORS fra `http://localhost:5500`.
   Alternativ: backenden viser `Frontend/` fra `wwwroot` (samme adresse, ingen CORS).
4. Oplà: Wi-Fi og broker-adresse i `arduino_secrets.h` (ikke i git). Plan B: mobil-hotspot.

## Åbne spørgsmål til mødet

1. Styring: knapper (som Kaspers eksempel) eller tilt? Olehs kode sender begge.
2. Multiplayer: med i MVP eller efter? (Nice to Have)
3. Hvad sker der ved afbrudt forbindelse? (Del 2, spørgsmål 14)
4. Hvilken broker: Mathias' eller vores egen?
5. Hvornår er `/gamehub` med falsk `GameState` klar? (Frontend venter på den)
6. Hvem tester spillet 6/11?
