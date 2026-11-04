# MVP og beskeder – til diskussion

Dokumentet har fem dele:

1. **Del 1 – Forslag fra Andrii (2/11):** MVP og dataflow for Pong-spillet.
2. **Del 1b – Forslag fra Oleh (2/11):** Oplà som kæledyr + controller, 5 behov, nye MQTT-beskeder, sensorer og 16 GB SD-kort.
3. **Del 1c – Forslag fra Oliver (2/11):** Backendens opbygning, knap og `tilt`, faste mål på spillefeltet, MQTT-broker, falske data, kommentarer til Olehs kæledyrs-beskeder og hvad backenden har brug for fra Arduino og frontend.
4. **Del 2 – Spørgsmål:** Det vi skal blive enige om.
5. **Del 3 – Aftale:** Udfyldes sammen, når vi er enige. Det er den, vi arbejder efter.

Alle må rette og kommentere. Princip: så få beskeder som muligt og ingen ekstra felter.

Datoer, DoD og status for hele projektet: se `docs/PLAN.md`.

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

> **Ændret 4/11:** Brokeren kræver, at alle topics starter med gruppens navn.
> Topics er nu `StormLamaerne/Tamapal/...` – store og små bogstaver præcis sådan.
> Client ID: Oplà `tamapal-opla-1`, backend `tamapal-backend`. Brokerens adresse står i
> `arduino_secrets.h` (ikke i git). Ældre forslag i Del 1b og 1c bruger stadig `tamapal/...`.

### 1. Arduino → backend (MQTT), 20 gange i sekundet

Topic: `StormLamaerne/Tamapal/opla-1/input`

```json
{ "tilt": 0.35, "button": false }
```

`tilt` går fra -1 (op) til 1 (ned). Arduinoen læser bare IMU og sender.

> **Afvigelse (4/11):** Olehs kode i `dev` sender flere felter end aftalt – både knapper og tilt:
> `{ "up": false, "down": false, "button": false, "tiltX": 0.00, "tiltY": 0.35, "tilt": 0.35 }`
> `tilt` og `button` er der stadig, så backenden kan bruge besked 1 som den står.
> Vi beslutter sammen, om vi styrer med knapper (`up`/`down`) eller `tilt` (Del 2, spørgsmål 2), og retter besked 1 bagefter.

### 2. Backend → Arduino (MQTT), kun ved hændelser

Topic: `StormLamaerne/Tamapal/opla-1/event`

Backenden lytter på `StormLamaerne/Tamapal/+/input` (`+` = alle Oplàer).

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

## Mine svar på Del 2 (frontend)

Forslag til diskussionen. Del 3 udfylder vi sammen.

| # | Mit forslag |
|---|---|
| 4 | Efter MVP: highscore først. Kirkegården i frontend kommer først, når Pong virker hele vejen (Oplà → backend → browser). |
| 9 | Backenden styrer bolden – enig med Oliver. Browseren tegner kun. |
| 10 | Enig: backenden viser siden fra `Frontend/`. |

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

# Del 1c – Forslag og observationer fra Oliver (Backend)

Her er mine kommentarer som backend-ansvarlig efter at have læst Del 1, Olehs forslag (Del 1b), noterne fra mødet 2/11 og vores GDD.
Formålet er, at backenden passer præcis til Oplàen og browseren, så ingen skal gætte. Jeg bruger samme opbygning som Oleh: **hvad er problemet**, **mit forslag**, **konsekvens** og et **eksempel**.

---

### 1. Sådan bygger jeg backenden (mindste version)
* **Hvad mangler?**  
  `Backend/` er tom endnu, og Del 1 siger ikke, hvordan backenden er bygget. DoD punkt 5 siger, at backenden skal styre spillets tilstand og reglerne (point og liv). Derfor er jeg enig i Del 1: backenden styrer bolden (spørgsmål 9).
* **Mit forslag:**  
  Ét ASP.NET Core-projekt i `Backend/` med fire dele:
  1. `MqttWorker`: lytter på `tamapal/+/input` og sender `event` til Oplàen (med biblioteket MQTTnet).
  2. `GameLoop`: kører 30 gange i sekundet, flytter bolden, tjekker vægge og ketsjer og tæller point og liv.
  3. `GameHub` på `/gamehub`: sender `GameState` til browseren og modtager `StartGame()`.
  4. `FakeInput`: en falsk Oplà til test (se punkt 6).

  Bolden flyttes efter den tid, der faktisk er gået siden sidste opdatering. Timeren i Windows er ikke helt præcis, så en opdatering kan komme lidt for sent, men bolden har stadig samme fart.
* **Konsekvens:**  
  Al logik ligger ét sted. Oplàen sender kun input, og browseren tegner kun det, den får. Hvis noget er galt med point eller liv, ligger fejlen i backenden.
* **Eksempel:**  
  Bolden rammer ketsjeren → `GameLoop` giver +1 point og lidt mere fart → `MqttWorker` sender `hit` til Oplàen → næste `GameState` viser den nye score i browseren.

---

### 2. Knappen: hvornår starter et spil?
* **Hvad er problemet?**  
  Oplàen sender `button` 20 gange i sekundet. Et normalt tryk varer ca. 0,2 sekunder, så backenden ser `true` 4 gange i træk. Et meget kort tryk kan omvendt falde mellem to beskeder og blive væk.
* **Mit forslag:**  
  - `button: true` betyder **"der er trykket siden sidste besked"**. Oplàen husker trykket og sender `true` i præcis én besked.
  - Backenden starter kun et spil, når `status` er `waiting` eller `gameover`. Det gælder både knappen og `StartGame()` fra browseren. Under et spil bliver de ignoreret.
* **Konsekvens:**  
  Ét tryk = én handling. Spillet kan ikke starte to gange, og et tryk bliver aldrig væk.
* **Eksempel:**  
  Spilleren trykker på Oplàen og på Start i browseren næsten samtidig. Backenden starter spillet på det første og ignorerer det andet, fordi `status` allerede er `playing`.

---

### 3. `tilt`: hvad betyder tallet præcist?
* **Hvad er problemet?**  
  Del 1 siger, at `tilt` går fra -1 (op) til 1 (ned), men ikke hvor meget man skal vippe, eller om `tilt` er ketsjerens plads eller fart.
* **Mit forslag:**  
  - **Oleh:** vippet 30° op = `-1`, vandret = `0`, vippet 30° ned = `1`. Mere end 30° tæller som -1 eller 1.
  - **Backend:** `tilt` er ketsjerens **plads**, ikke fart: `paddle.y = (tilt + 1) / 2`. Jeg udglatter tallet en smule, så ketsjeren ikke ryster, og holder den inde på banen.
  - De 30° kan vi justere efter brugertesten.
* **Konsekvens:**  
  Ketsjeren følger hånden direkte, og det er nemt at forstå for en ny spiller. Oplàen sender kun en vinkel, og kun backenden laver den om til en plads på banen.
* **Eksempel:**  
  Oplàen vippes 15° op → `tilt: -0.5` → `paddle.y = 0.25` (en fjerdedel nede fra toppen).

---

### 4. Faste mål på spillefeltet (til Andrii)
* **Hvad er problemet?**  
  `GameState` siger, hvor bolden og ketsjeren er, men ikke hvor store de er, hvor (0, 0) er, eller om `paddle.y` er toppen eller midten af ketsjeren. Hvis Andrii og jeg gætter forskelligt, ser det ud, som om bolden går igennem ketsjeren.
* **Mit forslag:**  
  Ingen nye felter. Vi skriver faste mål her i dokumentet, og begge bruger dem:
  - **Spillefelt:** kvadratisk (fx 600 × 600 px). x og y går fra 0 til 1.
  - **(0, 0)** er øverst til venstre, ligesom på canvas.
  - **Bold:** radius 0.02. `ball.x` og `ball.y` er boldens midte.
  - **Ketsjer:** 0.02 bred og 0.2 høj, med midten ved x = 0.03. `paddle.y` er ketsjerens midte.
* **Konsekvens:**  
  Det, browseren tegner, passer præcis med det, backenden regner med. Med et kvadratisk felt er bolden rund og lige hurtig i alle retninger, uden at nogen skal regne om.
* **Eksempel:**  
  `paddle.y = 0.5` på et canvas på 600 px → Andrii tegner ketsjeren fra y = 240 px til y = 360 px.

---

### 5. MQTT-broker og netværk (spørgsmål 5)
* **Hvad er problemet?**  
  Vi har ikke valgt broker endnu, og netværket er den største risiko i hele projektet:
  1. Mosquitto (version 2) tager som standard kun imod forbindelser fra den samme computer, så Oplàen bliver afvist.
  2. Windows Firewall blokerer port 1883.
  3. Skolens Wi-Fi blokerer måske trafik mellem to enheder, og Oplàen kan kun bruge 2,4 GHz Wi-Fi.
* **Mit forslag:**  
  - MQTT, ikke HTTP: det er lavet til mange små beskeder, og vi kan følge alt i MQTT Explorer.
  - Mosquitto på min bærbare, port 1883, med `listener 1883` og `allow_anonymous true` i `mosquitto.conf`. Den kører kun på lokalnettet, så ingen kodeord i første version.
  - Enhedens navn står i topic, ikke i JSON. Backenden lytter på `tamapal/+/input` og kan derfor tage imod spiller 2 (`opla-2`) senere uden at ændre noget.
  - QoS 0 for `input` (det gør ikke noget, hvis én ud af 20 beskeder forsvinder). QoS 1 for `event` og Olehs kæledyrs-beskeder (de skal frem).
  - Vi tester Oplà → Mosquitto → backend på skolens Wi-Fi så hurtigt som muligt. Plan B til fremlæggelsen: et mobil-hotspot.
* **Konsekvens:**  
  Hvis vi først opdager netværksproblemer på dagen, virker intet. Hvis vi tester tidligt, har vi tid til plan B.
* **Eksempel:**  
  Oplàen kan ikke forbinde på skolens Wi-Fi. Vi tænder et hotspot på en telefon, sætter bærbar og Oplà på det, og Oleh skifter IP-adressen i `arduino_secrets.h`.

---

### 6. Falske data: alle kan teste uden de andre
* **Hvad er problemet?**  
  Andrii kan først tegne rigtige data, når Oplàen virker, og Oleh kan først se ketsjeren bevæge sig, når browseren virker.
* **Mit forslag:**  
  - Backenden får en test-indstilling `FakeInput`. Når den er slået til, styrer computeren selv ketsjeren. Den følger bolden, men ikke perfekt, så den også misser og mister liv.
  - Oleh kan teste uden browser: backenden logger hvert `tilt`, og `event`-beskederne kan ses i MQTT Explorer.
  - Jeg kan selv sende `tilt` fra MQTT Explorer uden en Oplà.
* **Konsekvens:**  
  Ingen venter på hinanden, og det passer med trin 1 i "Rækkefølge for arbejdet".
* **Eksempel:**  
  Andrii åbner browseren, trykker Start og ser et helt spil med bold, ketsjer, point, liv og game over, selvom Oplàen ligger i tasken.

---

### 7. Kommentarer til Olehs kæledyrs-beskeder (Del 1b, punkt 3)
* **Hvad er problemet?**  
  Det er gode beskeder, og jeg kan gemme dem. Men:
  1. `pet/state` har intet ID. Når Lumi dør, og en ny Lumi bliver født på den samme Oplà, kan databasen ikke se forskel, og kirkegården kan ikke vise to gravsten.
  2. Kirkegården skal vise levetid (fase 7 i mit game loop i `TIL-DISKUSSION.md`), men beskeden har ingen alder.
  3. Med `pet/reward` skal backenden bestemme, hvor glad Lumi bliver. Men kæledyret bor på Oplàen.
* **Mit forslag:**  
  - Tilføj `"id"` (en kort tilfældig kode, der laves ved fødslen og gemmes på SD-kortet) og `"ageMinutes"` til `pet/state`.
  - Drop `pet/reward`. Send i stedet scoren med i `gameover`: `{ "type": "gameover", "score": 7 }`. Oplàen bestemmer selv, hvor meget glæde det giver.
  - Backenden gemmer hver Lumi under sit `id`. Kommer `"alive": false` to gange for samme `id`, bliver der stadig kun lavet én gravsten.
  - Alt det her kommer efter MVP.
* **Konsekvens:**  
  Én besked mindre, og alle regler for kæledyret ligger ét sted (på Oplàen). Kirkegården virker også, når en Oplà har haft flere Lumi'er.
* **Eksempel:**  
  ```json
  { "id": "a3f9", "name": "Lumi", "stage": "teen", "ageMinutes": 12, "hunger": 80, "warmth": 5, "energy": 70, "happy": 85, "discipline": 60, "alive": false, "cause": "cold" }
  ```
  Backenden gemmer en gravsten: "Lumi, teenager, levede 12 minutter, frøs ihjel".

---

### Mine svar på Del 2 (backend)

Forslag til diskussionen. Del 3 udfylder vi sammen.

| # | Mit forslag |
|---|---|
| 5 | MQTT med Mosquitto på min bærbare (punkt 5). |
| 7 | Input 20/s og `GameState` 30/s er fint. En `GameState` er ca. 100 bytes, altså ca. 3 KB i sekundet. Backenden bruger altid det nyeste `tilt` og gemmer ikke gamle. |
| 9 | Backenden styrer bolden (punkt 1). |
| 10 | Andriis filer bliver i `Frontend/`, og backenden viser dem på sin egen adresse (fx `http://localhost:5000`). Så ligger siden og `/gamehub` samme sted, vi slipper for CORS, og der er kun ét program at starte. |
| 11 | Database efter MVP. Først highscore (scoren gemmes ved `gameover`), så kirkegården. Neon PostgreSQL. Forbindelsen med kodeord ligger i `dotnet user-secrets`, aldrig i `appsettings.json`. |
| 12 | `feature/...` og 1 godkendelse. Vi er kun tre, så med 2 godkendelser skal alle læse alle Pull Requests. |
| 13 | Backend med falske data: tirsdag 3/11. Første test hele vejen: torsdag 5/11. Brugertest: fredag 6/11 (eller til åbent hus 7/11). Kode-stop: mandag 9/11, derefter kun fejlrettelser. Datoerne skal tjekkes i Notion. |

### Det har jeg brug for fra jer

Kun det, jeg skal bruge for at bygge MVP'en. Skriv svaret i kolonnen **Svar** eller i gruppens chat.

| # | Fra | Hvad jeg har brug for | Hvorfor | Svar |
|---|---|---|---|---|
| 1 | Oleh | Ja eller nej til punkt 2 (knappen) og punkt 3 (30° = 1). | Backenden skal læse `button` og `tilt` på samme måde, som Oplàen sender dem. | |
| 2 | Oleh | Det client ID, Oplàen bruger til MQTT. Backenden bruger `tamapal-backend`. | To klienter med samme client ID smider hinanden af brokeren. | |
| 3 | Andrii | Ja eller nej til målene i punkt 4. | Det, du tegner, skal passe med det, backenden regner med. | **Ja.** Canvas er 600 × 600, og `draw.js` bruger præcis disse mål (Andrii, 3/11). |
| 4 | Andrii | Må backenden vise siden fra `Frontend/`? Hvis ikke: hvilken adresse og port kører den på? | Ellers skal jeg slå CORS til for den adresse. | **Ja.** Indtil backenden kører, tester jeg med `python3 -m http.server 5500` i `Frontend/` (Andrii, 3/11). |

Fra mig får I brokerens IP-adresse og port (1883), så snart Mosquitto kører på min bærbare.

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
14. Hvad sker der, hvis forbindelsen afbrydes (Oplà eller browser)? Pause, besked på skærmen – eller game over?

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
| 12 | Branches og reviews | Push direkte til `dev` uden review. `main` kun via Pull Request + 1 godkendelse. `feature/...` til branches. (aftalt 4/11) |
| 13 | Datoer | |
| 14 | Afbrudt forbindelse | |

**Godkendt af:**

- [ ] Oleh – dato:
- [ ] Oliver – dato:
- [ ] Andrii – dato:

Ændringer efter aftalen: ret både Del 1 og tabellen her, og skriv det i gruppens chat.
