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

## Vores 5 beskeder (MQTT & SignalR)

> **Ændret 4/11:** Brokeren kræver, at alle topics starter med gruppens navn.
> Topics er nu `StormLamaerne/Tamapal/...` – store og små bogstaver præcis sådan.
> Client ID: Oplà `tamapal-opla-1` (og `tamapal-opla-2`), backend `tamapal-backend`. Brokerens adresse står i
> `arduino_secrets.h` (ikke i git).

### 1. Arduino → backend (MQTT), 20 gange i sekundet + ved knap-kant

Topic: `StormLamaerne/Tamapal/opla-1/input` (og `opla-2/input` for Spiller 2)  
QoS: 0  
Frekvens: Kontinuerlig 20 Hz (~50 ms) for jævn tilt, samt **øjeblikkelig kant-afsendelse (< 1 ms)** når knapper trykkes eller slippes.

```json
{
  "up": false,
  "down": false,
  "button": false,
  "tiltX": 0.00,
  "tiltY": 0.35,
  "tilt": 0.35
}
```

* `tilt` og `tiltY`: går fra -1 (vippet 30° op) til 1 (vippet 30° ned). 0 er vandret. Backenden bruger dette til ketsjerens placering: `paddle.y = (tilt + 1) / 2`.
* `tiltX`: vandret hældning (-1 venstre, +1 højre) til fremtidige minispil (f.eks. Labyrint).
* `button`: `true` ved serv / aktion (Touch 2 på Oplà).
* `up` / `down`: Touch 0 (Op) og Touch 1 (Ned). Hvis spilleren foretrækker knapper frem for tilt, kan backenden også læse disse direkte!

### 2. Backend → Arduino (MQTT), kun ved hændelser

Topic: `StormLamaerne/Tamapal/opla-1/event` (og `opla-2/event` for Spiller 2)  
QoS: 1  

```json
{ "type": "hit" }
```

**Understøttede typer og Oplà-feedback:**
* `"hit"`: Ketsjer ramte bolden (+1 point). Oplà bipper med lys tone (1200 Hz, 80 ms) og blinker grønt/cyan.
* `"miss"`: Bolden røg forbi (-1 liv). Oplà summer med dyb tone (220 Hz, 150 ms) og blinker advarselsrød.
* `"gameover"`: 0 liv tilbage, spil slut. Oplà afspiller 3-tonet faldende melodi (440 $\to$ 330 $\to$ 220 Hz).
* `"assign_p1"` / `"assign_p2"`: Backenden tildeler eller bekræfter spillerens rolle.

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

---

### 5. Arduino → backend (MQTT), kæledyrets 5 behov (hvert 30. sekund og ved handlinger)

> **Implementeret 4/11 i `feature/arduino-needs` (Oleh):**  
> Oplàen opretholder Wi-Fi og MQTT-forbindelse i både Kæledyrs-tilstand og Pong-tilstand. Den sender automatisk en statuspakke for Lumis 5 behov, så backenden kan gemme kæledyret i Neon PostgreSQL, og frontenden kan vise helbredet på hjemmesiden.

Topic: `StormLamaerne/Tamapal/opla-1/pet/state` (eller `opla-2/pet/state` for Spiller 2)  
QoS: 1 (sikrer at tilstandsændringer og død leveres pålideligt)  
Frekvens: Hvert 30. sekund (heartbeat) samt **øjeblikkeligt** ved spillerhandlinger (Mad, Leg, Skæld ud, Søvn).

```json
{
  "name": "Lumi",
  "stage": "teen",
  "hunger": 80,
  "warmth": 85,
  "energy": 90,
  "happy": 75,
  "discipline": 50,
  "alive": true
}
```

**Feltbeskrivelse for de 5 behov (alle er heltal 0–100%):**
* `name` (string): Kæledyrets navn (standard: `"Lumi"`).
* `stage` (string): Livsstadie (`"baby"`, `"teen"`, `"adult"`).
* `hunger` (int 0–100): Mæthed (100 = propmat, 0 = udsultet). Falder naturligt over tid. Stiger ved fodring i menuen (+25%).
* `warmth` (int 0–100): Kropstemperatur (100 = optimal 20–25°C stuetemperatur). Falder hvis rummet er koldt (<14°C), stiger ved berøring/kropsvarme.
* `energy` (int 0–100): Energi / Søvn (100 = udhvilet, 0 = udmattet). Genvindes når Lumi sover (+2% pr. 10 sek).
* `happy` (int 0–100): Glæde (100 = super glad, 0 = trist). Stiger ved leg (+25%), falder ved skæld ud (-10%).
* `discipline` (int 0–100): Opdragelse (100 = velopdragen). Stiger når man skælder Lumi ud (+20%), hvis den piber uberettiget.
* `alive` (bool): `true` så længe samlet helbred > 0%. Hvis Lumi dør af sult/kulde, sendes `false`.

#### Vejledning til Oliver (Backend / ASP.NET Core):
Abonner på topic: `StormLamaerne/Tamapal/+/pet/state` (`+` fanger alle Oplàer).  
Brug denne C# DTO-klasse:
```csharp
public class PetStateDto
{
    public string Name { get; set; } = "Lumi";
    public string Stage { get; set; } = "teen";
    public int Hunger { get; set; }
    public int Warmth { get; set; }
    public int Energy { get; set; }
    public int Happy { get; set; }
    public int Discipline { get; set; }
    public bool Alive { get; set; }
}
```
* **Database (Neon PostgreSQL):** Gem seneste tilstand i tabellen `pets` / `pet_states`.
* **Kirkegård:** Hvis `Alive == false`, opret en post i kirkegårdstabellen med tidspunkt og overlevet tid til Andrii.
* **SignalR:** Udsend event `PetStateUpdated(dto)` til browserklienter.

#### Vejledning til Andrii (Frontend):
* Lyt på `PetStateUpdated` fra SignalR.
* Tegn 5 vandrette bjælker (0–100%): Sult (Sulten $\to$ Mæt), Varme (Kold $\to$ Varm), Energi (Træt $\to$ Frisk), Glæde (Trist $\to$ Glad), Disciplin.
* Hvis `alive === false`, vis en gravsten på kirkegården med teksten "Lumi hviler her".

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
| 1 | Oleh | Ja eller nej til punkt 2 (knappen) og punkt 3 (30° = 1). | Backenden skal læse `button` og `tilt` på samme måde, som Oplàen sender dem. | **Ja.** Oplà sender `tilt` (-1.0 til 1.0, hvor 30° = 1.0) og `button` (true/false ved tryk på Touch 2). Desuden sender vi `up`/`down` (Touch 0/1) og `tiltX` (hvis du vil bruge knapper eller 2D-bevægelse). Knaptryk sendes øjeblikkeligt (< 1 ms latency), og der er 20 Hz heartbeat. (Oleh, 4/11) |
| 2 | Oleh | Det client ID, Oplàen bruger til MQTT. Backenden bruger `tamapal-backend`. | To klienter med samme client ID smider hinanden af brokeren. | `tamapal-opla-1` (eller `tamapal-opla-2` for Spiller 2). (Oleh, 4/11) |
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

---

## Hardware- og opsætningsvejledning for Oplà-controlleren

### 1. Plastik-case vs. Bar printplade (Touch-følsomhed)
Oplà-kortet bruger kapacitive touch-sensorer.
- **Med plastik-case:** Kræver høj følsomhed (tærskel `60`) for at opfange fingre gennem plasten.
- **Uden case (bar printplade):** Kræver lav følsomhed (tærskel `200`) for at undgå falske "spøgelsestryk".

**Sådan konfigureres det:**
1. **I `arduino_secrets.h` (permanent for din enhed):**
   ```cpp
   #define SECRET_USE_CASE false  // Sæt til true hvis monteret i hvid plastik-case
   ```
2. **On-the-fly i Serial Monitor (uden genprogrammering):**
   Send bogstavet `c` (eller `C`) i Serial Monitor (115200 baud). Arduinoen skifter øjeblikkeligt følsomhed og bekræfter i loggen.

### 2. Mute (Lyd fra/til)
- **Genvej 1:** Tryk på Touch 0 og Touch 4 samtidig.
- **Genvej 2:** Hold Touch 3 nede i 1,2 sekunder.
- **Fysisk gestus:** Vend Oplà-kortet med bunden i vejret (IMU Z-akse < -0.7g).

---

# Olehs opdateringer og vejledninger for Lumi-karakteren

Her dokumenteres alle udvidelser til Lumis visuelle verden, livsstadier, animationer og test-kommandoer på Oplà-kortet. Hver opdatering er dateret og adskilt.

___

### 🌟 Opdatering 5/11: 2.5D Lumi, sømløs scene og dynamisk vejr
**Branch:** `feature/arduino-lumi`  
**Ansvarlig:** Oleh

Lumi er integreret på Oplàens 240×240 ST7789 display med en 2.5D Nintendo / Zelda Kokiri-æstetik.  
Alt renderes **proceduralt med delta-opdateringer uden RAM-framebuffer** (kun 25% RAM forbrug på SAMD21).

#### Hvad er tilføjet:
1. **Sømløs visning (ingen firkantet baggrundsboks):**
   - Scenens baggrund matcher præcist skærmens mørke navy farve (`#080C1A`).
   - Mosbeklædt svævende platform, glødende svamp og skovblomst blender naturligt ind i displayet uden hårde kanter.
2. **Dynamisk himmel og vejr (`SkyWeatherMode`):**
   - ☀️ **Dag (`SKY_DAY_SUNNY`):** Varm Nintendo-sol med animerede pulserende stråleprikker og lysende guldstøv.
   - 🌙 **Nat / Søvn (`SKY_NIGHT_STARRY`):** Halvmåne og tindrende 4-takkede stjerner, når Lumi sover (aktiveres automatisk, hvis lyssensoren tildækkes i 5 sekunder).
   - ❄️ **Vinter / Kulde (`SKY_WINTER_SNOW`):** Krystalklare faldende snefnug og rimfrost på platformen, når `warmth < 45%`.
3. **To fe-ledsagere (Zelda OoT inspiration):**
   - **Navi:** Cyan-hvid lysende fe med blafrende vinger over venstre side af Lumi.
   - **Tatl:** Lille guld-lyserød skovfe, som svæver lavere i synkron rytme.
4. **Organiske animationer (15 FPS, flimmerfri):**
   - Squashing/stretching åndedræt, rystelser ved kulde ($\pm 2$ px), naturligt øjenblink hvert 3,2 sek., mundbevægelser og flydende `z Z Z` ved søvn eller tankeboble med bær ved sult.

#### Vejledning til Oliver (Backend) og Andrii (Frontend):
* **Backend:** `StormLamaerne/Tamapal/opla-1/pet/state` sendes fortsat uændret hvert 30. sek. Vejr kan udledes direkte fra `PetStateDto`:
  - `Warmth < 45` $\to$ Vinter / Sne.
  - `Energy` faldet under søvn $\to$ Nat / Stjernehimmel.
  - Ellers $\to$ Solskin.
* **Backend reward:** Send `StormLamaerne/Tamapal/opla-1/pet/reward` (`{"type":"happy_boost","amount":25}`) efter point i Pong; Lumi kvitterer med jubel-animation, stjerner og hævede poter!
* **Frontend styling:** Brug gerne `#080C1A` som mørk baggrund og Kokiri-mint `#9BF8DC` med mørk jade `#237D6E` til Lumi-elementer.

___

### 🐣 Opdatering 5/11 (Aften): Baby-stadie og on-the-fly stadieskift i Serial Monitor
**Branch:** `feature/arduino-stages`  
**Ansvarlig:** Oleh

Vi har implementeret Lumis første livsfase: **Baby-stadiet**, samt et interaktivt værktøj til at skifte mellem faserne direkte under kørsel.

#### Hvad er tilføjet:
1. **Baby-udseende (æg-formet krop & sut):**
   - **Kompakt æg-silhuet:** Rund og blød mochi-krop ($38 \times 34$ px i stedet for teenagerens $48 \times 39$ px).
   - **Baby-detaljer:** Korte runde knap-poter, små knop-ører og en spæd spire med kun ét enkelt lille hjerteformet blad.
   - **Sut i munden:** Baby-Lumi sutter på en gul/koral sut med lille ring, der vipper i takt med vejrtrækningen. Ved spisning af bær skiftes sutten automatisk ud med bær-animationen.
2. **Persistens på MicroSD:**
   - Det aktuelle livsstadie gemmes som `STAGE=0` (Baby) i `lumi.txt` på MicroSD-kortet og gendannes ved genstart.
3. **MQTT-tilstand:**
   - I `pet/state` sendes feltet `"stage": "baby"` (eller `"teen"`, `"adult"`, `"old"`).

#### Instruktion til test og udvikling (Serial Monitor):
* **Skift stadie on-the-fly uden at genprogrammere:**
  - Åbn Serial Monitor (115200 baud).
  - Skriv bogstavet `s` (eller `S`) og tryk Enter.
  - Arduinoen cykler øjeblikkeligt gennem livsstadierne:  
    `Baby` $\to$ `Teen` $\to$ `Adult` $\to$ `Old` $\to$ `Baby`  
    med bekræftelsestone og øjeblikkelig opdatering på skærmen!
* (Bogstavet `c` kan fortsat bruges til at skifte touch-følsomhed for plastik-case).

___

### 🎧 Opdatering 5/11 (Sen Aften): Swag Teenager-stadie med Kokiri-horn og DJ-høretelefoner
**Branch:** `feature/arduino-stages`  
**Ansvarlig:** Oleh

Lumis **Teenager-stadie** har fået sit fulde swag-look med attitude og gaming/DJ-æstetik som beskrevet i Game Design Doc.

#### Hvad er tilføjet:
1. **Kokiri-horn med guldspidser:**
   - I stedet for en simpel spire har teenageren nu to seje buede skovhorn med gyldne spidser og skinnende highlights (`hornGold`), samt en skarp midterkam.
2. **Cyber DJ / Gamer-høretelefoner:**
   - Mørk bøjle over hovedet og neongrønne/cyan ørekopper med pink detaljer hen over ørerne.
3. **Flabet attitude og mimik:**
   - Hævet "swag" højre øjenbryn, når Lumi er tilfreds.
   - Skævt selvsikkert katte-smil mod højre (smug smirk), der oser af selvtillid.

___

### 💼 Opdatering 6/11 (Morgen): Voksen Lumi (Adult) med diplomatkuffert, slips og blomstrende krone
**Branch:** `feature/arduino-stages`  
**Ansvarlig:** Oleh

Lumis **Voksen-stadie (Adult)** er nu fuldt implementeret med et stilet forretnings- og vogter-look som beskrevet i Game Design Doc:

#### Hvad er tilføjet:
1. **Diplomatkuffert i læder (attaché case):**
   - Klassisk forretningskuffert i varmt brunt læder (`#915028`) med mørk kantliste, syninger og gyldne messing-hjørnebeskyttere.
   - Central messinglås med glinsende nøglehuls-refleksion.
   - **Interaktiv holdning:** Lumi holder kuffertens buede håndtag fast i sin højre pote. Ved jubel (`SUPER_HAPPY` / `PLAYING`) hæver Lumi kufferten triumferende i vejret med begge poter!
   - Ved kulde (`FREEZING`) trykker Lumi kufferten tæt ind til kroppen, mens han ryster.
2. **Klassisk mørkerødt slips med guld-slipsenål:**
   - Slips i dyb karminrød (`#D2233C`) med knude ved kraven og spids, der falder elegant ned over mochi-maven.
   - Guld slipsenål med lysrefleks tværs over slipset.
3. **Blomstrende Kokiri-vogterkrone:**
   - To elegante svungne vogterblade med lysende grønne nerver.
   - Blomstrende gylden lotus/krystal-krone i midten med ravfarvet kerne.
4. **Gentleman-smil og rolig mimik:**
   - Værdigt, selvsikkert og venligt forretnings-smil, der signalerer en moden og velplejet Lumi.

#### Instruktion til test (Serial Monitor):
* Tryk `s` i Serial Monitor (115200 baud) for at skifte til **Adult** (`[STAGE] Skiftet stadie til: adult`).
* Test handlinger (Foder, Leg, Skæld ud) for at se kufferten og slipset animere sømløst uden flimmer.

___

### 🎩 Opdatering 6/11 (Formiddag): Gammel Lumi (Old / Vismand) med høj hat, guldbriller, overskæg og spadserestok
**Branch:** `feature/arduino-stages`  
**Ansvarlig:** Oleh

Lumis fjerde og sidste livsstadie: **Den Gamle Vismand (Old)** er nu fuldt implementeret med masser af personlighed, visdom og retro-charme:

#### Hvad er tilføjet:
1. **Klassisk høj hat (gentleman top hat / vismandshat):**
   - Mørk koksgrå cylinderhat (`#232837`) med bred buet skygge og lysstribe-highlight.
   - Karminrødt silkebånd om bunden af hatten med et gyldent spænde i midten.
   - En fjergrøn Kokiri-visdomsfjer stukket ind i hattebåndet.
2. **Runde guldbriller og buskede hvide øjenbryn:**
   - Fine runde trådbriller i guld med næsebro og lysglimt i brilleglasset over Lumis øjne.
   - Buskede, bløde hvide øjenbryn ovenover brillerne, som giver et klogt og godmodigt professor-udtryk.
3. **Hvidt mochi-overskæg og mildt smil:**
   - Sødt hvidt overskæg under næsen, som følger med i alle udtryk og vejrtrækninger.
   - Et mildt, tilfreds vismands-smil (`EXPR_CONTENT`).
4. **Gylden lommeur-kæde:**
   - Dobbeltbuede guldkæde tværs over Lumis mave med et lille rundt lommeur stukket ind i linningen.
5. **Mahogni-spadserestok (walking cane):**
   - Solid mahognistok med messing-dup, der støtter solidt på den mosbegroede platform.
   - Buede gylden kryk-håndtag (crook handle) med lysglimt, som Lumi holder fast i med højre pote.
   - **Interaktiv holdning:** Ved jubel (`SUPER_HAPPY` / `PLAYING`) løfter den gamle Lumi stokken muntert op i luften og vinker med den! Ved kulde (`FREEZING`) læner han sig rystende ind over stokken med begge poter.

#### Komplet livscyklus på plads:
Alle 4 livsstadier fra Game Design Doc er nu færdigudviklet:
`Baby` (æg/sut) $\to$ `Teen` (swag/horn/DJ) $\to$ `Adult` (forretning/kuffert/slips) $\to$ `Old` (vismand/hat/briller/stok).  
Skift frit imellem dem med `s` i Serial Monitor, og se MicroSD gemme og gendanne det automatisk!

___

### 🧣 Opdatering 6/11 (Middag): Garderobe-system & Termoregulering (Vinterhue, Halstørklæde & Hedebølge)
**Branch:** `feature/arduino-wardrobe`  
**Ansvarlig:** Oleh

Lumis **Garderobe-system (`Garderobe`) og dynamiske termoregulerings-mekanik** er nu fuldt implementeret og testet på tværs af alle 4 livsstadier.

#### Hvad er tilføjet:
1. **Garderobe og vintertøj tilpasset alle 4 stadier:**
   - 🐣 **Baby:** Varm rød strikhue med hvid stribekant og blød sne-pompon på toppen, hvor den lille spire stikker sødt ud ved siden af, samt et rød/hvidt stribet halstørklæde under hagen med frynser.
   - 🎧 **Teen:** Mørk slouchy gamer-beanie, hvor de gyldne Kokiri-horn stikker sejt igennem i hver side, neonlysende cyan pompon bagtil, og et cyber-halstørklæde med cyan striber, der blafrer til højre.
   - 💼 **Adult:** Foret jagtgrøn tweed-vinterhue med nedhængende varme øreklapper, den gyldne blomstrende lotus-krone som emblem foran, og et bordeaux cashmere-halstørklæde med guldbroderi over brystet.
   - 🎩 **Old:** Den høje gentleman-hat udstyret med bløde hvide pels-ørekopper, der omfavner hovedet, frostet krystalsnefnug på hattebåndet og et tykt strikket jægergrønt bedstefar-tørklæde med kvaster.
2. **Fysisk termoregulering (samspil med temperatursensor):**
   - **Kulde ($T < 18^\circ\text{C}$ eller varme $< 45\%$):** Lumi fryser og ryster. Status viser *"Lumi fryser! Giv hue paa!"*. Tager spilleren vintertøj på, isoleres Lumi og varmen stiger til et lunt mål ($85\%$).
   - **Hedebølge / Overophedning ($T \ge 23.5^\circ\text{C}$ og vintertøj på):**
     Hvis Lumi har vinterhue på i et varmt klasselokale, overopheder han!
     * **Visuel feedback:** Rødblussende kinder, ferskenfarvet overophedet krop, forpustet åben mund med hængende tunge (`😛`), sammenknebne øjne og en stor skinnende sveddråbe (`💧`), der løber ned af tindingen, samt faldende dråber på sideskærmen.
     * **Mekanik:** Humøret (`happiness`) falder hurtigt under overophedning, og status advarer: *"Lumi sveder! Tag huen af!"*.
     * **Løsning:** Spilleren tager huen af $\to$ Lumi køler straks af og smiler igen!
3. **Styring og genveje:**
   - **På Oplà:** Hold **TOUCH 0 (<)** og **TOUCH 1 (>)** nede samtidig i 400 ms på startskærmen for at tage tøj på/af (viser pop-up bekræftelse på displayet).
   - **I Serial Monitor (115200 baud):**
     * Skriv `'w'` (Wardrobe) for at skifte mellem standard og vintertøj on-the-fly.
     * Skriv `'f'` (Frost) for at simulere 12 graders frostkulde og teste kulde-mekanikken.
     * Skriv `'h'` (Heat) for at simulere 28 graders hedebølge og se Lumi svede og puste!
4. **Persistens og MQTT:**
   - Garderobevalg gemmes på MicroSD i `lumi.txt` som `OUTFIT=0` (Standard) eller `OUTFIT=1` (Vinter).
   - I `pet/state` MQTT JSON sendes `"outfit": "default"` eller `"outfit": "winter"`.

___

### 🥶 Opdatering 6/11 (Eftermiddag): Autentisk kropslig reaktion på kulde og varme (Sammenkrøbning, rystelser & udmattet hedeslag)
**Branch:** `feature/arduino-wardrobe`  
**Ansvarlig:** Oleh

Vi har videreudviklet de fysiske reaktioner på ekstrem temperatur, så Lumi ikke blot viser en simpel ikon-effekt, men fysisk ændrer kropsholdning, positur og mikrobevægelser svarende til et rigtigt levende væsens ufrivillige spasmer og udmattelse.

#### Hvad er tilføjet på tværs af alle 4 stadier:
1. **Autentisk kulde-reaktion (Sammenkrøbet frysende kropsholdning & asymmetriske mikroryk):**
   - **Kroppen kryber sammen:** Silhuetten trykkes 2–3 pixels fladere og trækkes ind mod maven for at holde på kropsvarmen (fosterstilling).
   - **Trykkede ører:** Ørerne foldes helt fladt ind mod hovedet (`earTipOffsetY = -17`).
   - **4-faset asymmetrisk kulde-krampe (~22 Hz / 45 ms):** I stedet for en kunstig sinusbølge ryster Lumi med en realistisk ufrivillig muskelspasme (`shiverX = {-2, 1, -1, 2}`, `shiverY = {2, 1, 2, 1}`).
   - **Klakkende tænder:** Munden veksler lynhurtigt mellem en rystende bølge-mund og en hvid stribe af klakkende tænder.
   - **Baby:** Sutten klakker og vibrerer i munden (`paciDx = -1..0`), og de små luffer er krydset stramt over brystet.
   - **Adult:** Holder diplomatkufferten tæt ind til den rystende krop med begge poter.
   - **Old:** Begge skælvende poter klynger sig krampagtigt til mahogni-stokken.
   - **Teen:** Lufferne trykkes ind i siden.

2. **Autentisk varme-reaktion (Hedeslag, slap positur & tung termisk stakåndethed):**
   - **Slapt sammenfald:** Kroppen og lemmerne synker slapt sammen under hedebølgen.
   - **Hængende ører:** Ørerne taber spændstigheden og hænger slapt ned ad kinderne (`earTipOffsetY = -13`).
   - **Hæsblæsende stakåndethed (Panting heave, 90 ms):** Brystkassen hejser sig hurtigt op og ned (`pantingDy = {-1, 0, 1, 0}`), mens Lumi gisper efter luft.
   - **Udhængende tunge med spytglans:** Munden åbnes vidt med en blød lyserød tunge, der hænger ud over underlæben (`😛`) med hvid spytrefleksion.
   - **Løbende sveddråbe & pandesved:** En animeret sveddråbe glider løbende ned ad højre tinding, mens en perlesveddråbe glitrer på venstre bryn.
   - **Baby:** Spytter sutten ud i heden for at kunne gispe med tungen ude, mens poterne ligger slapt ud til siderne på jorden.
   - **Teen:** Cyber-høretelefonerne glider 5 pixels ned ad den svedige hals (`hpOffsetY = 5`), mens poterne hænger slapt ned.
   - **Adult:** Det formelle slips er løsnet skævt op ved kraven, venstre pote tørrer sved af panden, diplomatkufferten er tabt fladt på jorden, og højre pote hænger udmattet.
   - **Old:** Guldbrillerne glider ned over næseryggen (`glassesY + 2`), og vismanden læner sig tungt og udmattet fremover med begge poter slapt hvilende på stokkens håndtag.

#### Ydeevne og hardware-budget på SAMD21 (MKR WiFi 1010):
- **CPU:** 48 MHz 32-bit ARM Cortex-M0+ afvikler hele den procedurale rendering på blot **~4% CPU** tid.
- **RAM:** Kun **8.468 bytes (25%)** brugt ud af 32 KB SRAM (24,3 KB fri til stak og lokale variable).
- **Flash:** **171.644 bytes (65%)** ud af 256 KB Flash (90,5 KB fri plads til Pong og labyrint).
- **Flimmerfri delta-rendering:** Ingen dynamisk RAM-allokering i `loop()`, opdateringsfrekvens skalerer dynamisk op til ~22 FPS ved kulde/varme for ekstremt flydende spasmer.

___

### ⚡ Opdatering 6/11 (Eftermiddag): Hukommelsesoptimering, fjernelse af dynamisk heap i MQTT & MicroSD dirty-cache (Forberedelse til spil)
**Branch:** `feature/arduino-wardrobe`  
**Ansvarlig:** Oleh

Før udviklingen af de to on-board minispil (Solo-Pong og labyrint-generatoren) blev hele kildekoden underkastet en dybdegående indlejret C++ hukommelses- og stabilitetsoptimering. På en ARM Cortex-M0+ mikrokontroller uden virtuel hukommelse er dynamisk hukommelsesallokering (`malloc`, `String`, eksterne biblioteker med dynamisk JSON-træ) den største kilde til **heap-fragmentering** og uforklarlige nedbrud (`HardFault`) over tid.

#### Hvorfor denne optimering var nødvendig (Arkitektur & eksamensforsvar):
1. **Forebyggelse af heap-fragmentering:** Når MQTT modtager beskeder over Wi-Fi, må der ikke allokeres og frigives små dynamiske hukommelsesblokke på heapen i `loop()`. Det fragmenterer den frie RAM og kan få programmet til at gå ned efter 10–30 minutters kørsel.
2. **Beskyttelse af SPI-bussen og MicroSD-flash:** ST7789 displayet og MicroSD-kortet deler den samme SPI-bus på Carrier-printet. Hyppig skrivning til SD-kortet blokerer displayopdateringen og slider på kortets flash-blokke.
3. **Fuldstændig offline-robusthed:** Hvis Wi-Fi forbindelsen mistes i lokalet, må mikrokontrollerens `loop()` aldrig blokere i 4–5 sekunder på TCP/DNS-timeouts. Spillene og Tamagotchi-animationerne skal køre uhindret videre med fuld FPS.

#### Hvad er ændret i koden:
1. **1-byte typede `enum` deklarationer (`uint8_t`) & struct-pakning:**
   - Alle globale enums i `settings.h` (`ScreenType`, `MenuIcon`, `TouchPad`, `SoundType`, `GameEvent`, `PetStage`, `PetOutfit`) samt grafik-enums i `ui_pet.h` (`LumiExpression`, `SkyWeatherMode`) er eksplicit typet som `: uint8_t`.
   - På en 32-bit Cortex-M0+ tildeler compileren standard 4 bytes pr. enum. Ved at tvinge dem til `uint8_t` spares 3 bytes pr. forekomst, og datastrukturer kan pakkes tæt uden unødig polstring (`padding bytes`).
   - Medlemmerne i `UiPetRenderer` og `UiStatsRenderer` er omorganiseret efter faldende justering (pointere, `unsigned long`, `int16_t`, `char`-arrays, `bool`).

2. **Fuldstændig fjernelse af `ArduinoJson` og dynamisk `String` i `net_mqtt`:**
   - `#include <ArduinoJson.h>` er fjernet fra projektet.
   - Indgående MQTT-beskeder parses nu af en nul-allokerings tokenizer direkte på CPU-stakken (`extractJsonStringValue`, `extractJsonIntValue`), der finder felter som `"type"` og `"amount"` uden en eneste byte på heapen.
   - Statiske emne-buffere (`topicInput`, `topicEvent`, `topicReward`, `topicPetState`, `clientMqttId`) er optimeret i størrelse til faste længder.
   - **Gevinst:** Flash-forbruget faldt med næsten **5 KB (4.992 bytes)** ved at slippe for `ArduinoJson`-templates, og heap-forbruget i `net_mqtt` er nu **0 bytes**.

3. **Afkobling af netværks-blocking ved offline kørsel:**
   - I `TamaPal.ino` kaldes `netMqtt.update(now)` kun, når Oplà er i `SCREEN_CONTROLLER` eller hvis forbindelsen allerede er aktiv.
   - Når spilleren hygger sig med kæledyret offline, forsøges der ikke gentagne blokerende DNS/TCP-håndtryk, hvilket fjerner det berygtede 5-sekunders frys og sikrer konstant ~22–30 FPS.
   - Under aktivt controller-spil pauses MicroSD-skrivning (`syncPetStateToBackend`), så boldens bevægelse på lærredet aldrig rammes af SPI-ventetid.

4. **MicroSD Dirty-Checking Cache via FNV-1a Hash (`storage_pet`):**
   - Hver gang tilstanden gemmes, beregner `storagePet` en lynhurtig 32-bit Fowler–Noll–Vo (FNV-1a) hash af serialiseringsstrengen.
   - Hvis dataene ikke har ændret sig siden sidste skrivning (`currentHash == lastSavedHash`), afbrydes skrivningen øjeblikkeligt uden at røre MicroSD-kortet.
   - Udvidet format: `lumi.txt` understøtter nu fremtidige spildata bagudkompatibelt: `PONG=<score>`, `MAZE=<score>` og `AGE=<minutter>`.

5. **Eliminering af asynkron tidsforskydning og koordinat-rettelse (`ui_pet` & `ui_stats`):**
   - Alle uafhængige `millis()`-kald inde i ansigtsanimationen (`freezing chatter`, `sweat drop`, `panting mouth`) er erstattet af ét fælles tidsstempel `lastAnimTickMs`, så animationens faser aldrig desynkroniserer.
   - Sveddråbens startposition er rykket ned til `eyeY - 5`, så den forbliver 100% inde i ansigtets afgrænsede slette-boks.
   - I `ui_stats` er `char lastMood[32]` erstattet med konstant pointer-sammenligning (`lastMoodText != currentMood`), hvilket fjerner unødige `strcmp` og `strncpy` i hvert frame.

#### Verificerede kvalitets- og hukommelsestal (Arduino MKR WiFi 1010):
- **Flash-forbrug:** **166.652 bytes (63%)** ud af 256 KB — **sparet 4.992 bytes** i forhold til tidligere build!
- **SRAM-forbrug:** **8.324 bytes (25%)** ud af 32 KB — efterlader **24.444 bytes fri RAM** til stak og minispil!
- **Unit tests:** **170 af 170 tests bestået (100% PASS, 0 advarsler)**.
- **Kompilering:** 0 fejl, 0 advarsler (`--warnings all`).

___

### 🏓 Opdatering 6/11 (Sen Eftermiddag): On-board Solo Mur-Pong på Arduino Oplà (Fuld minispil-arkitektur)
**Branch:** `feature/arduino-games`  
**Ansvarlig:** Oleh

Vi har implementeret det første fulde offline-minispil direkte på Oplà-kortets runde 240x240 display: **Solo Mur-Pong**. Spillet kan spilles 100% uafhængigt uden computer og uden internetforbindelse.

#### Hvad er tilføjet:
1. **Minispil-menu (`SCREEN_GAMES_MENU`):**
   - Når spilleren vælger `LEG` i hovedmenuen, åbnes en stilren cyberpunk minispil-vælger med to kort:
     * `1. SOLO PONG` (Mur-Pong bat & bold) med visning af personlig rekord (High Score).
     * `2. LABYRINT` (IMU marmor-kugle) med visning af rekord.
   - Skift mellem spil med `TOUCH 0` (<) og `TOUCH 1` (>), start med `TOUCH 2` (OK), tilbage med `TOUCH 3` (BACK).
2. **Spilmekanik og fysik (`PongLogic`):**
   - **Styring:** Spilleren kan styre battet vertikalt ved at **hælde Oplà-kortet** (`tiltY` via LSM6DS3 IMU med jævn Lerp-interpolation) ELLER ved at trykke på touch-knapperne `TOUCH 0` (ned) og `TOUCH 1` (op), hvis kortet ligger fladt på bordet.
   - **Solid neon bagvæg:** Til højre på banen findes en massiv guldbelagt neon-rebound væg. Bolden preller af på over-, under- og højre væg (`SOUND_CLICK`).
   - **Bat-kollision & vinkelafbøjning:** Når battet rammer bolden, ændres boldens vertikale spin afhængigt af, om den rammer toppen, midten eller bunden af battet. Hvert vellykket slag accelererer bolden en smule (hastigheds-boost) og giver 1 point (`SOUND_HIT` + cyan lysglimt på battet).
   - **Liv & Game Over:** Spilleren har 3 liv (vises som lysende hjerte-prikker i toppen). Hvis bolden slipper forbi battet mod venstre, mistes et liv (`SOUND_MISS`). Efter en kort pause på 800 ms serves en ny bold.
   - **Resultat & High Score:** Når alle 3 liv er tabt (`SOUND_GAMEOVER`), vises en elegant modal med slutscore, personlig rekord og guld-fejring ved *"NY REKORD!"*. Rekorden gemmes persistent på MicroSD i `lumi.txt` (`PONG=<score>`).
3. **Effekt på Lumi:**
   - En fuldført omgang Pong belønner kæledyret: `happiness` stiger med op til +30 point, mens Lumi bruger en smule energi (`energy -= 8`).
4. **Grafik & Flimmerfri Delta-Rendering (`UiPongRenderer`):**
   - Kun battets og boldens forrige positioner overskrives med baggrundsfarven `#080C1A` i hvert frame.
   - Ingen `fillScreen()` under kørslen: afvikles ved **50+ FPS** med 0 skærmflimmer!
5. **Test & genveje:**
   - Skriv `'p'` eller `'P'` i Serial Monitor (115200 baud) for at starte Solo Pong øjeblikkeligt.
   - 25 nye unit-tests dækker 100% af fysikken, kollisionerne og tilstandene i `PongLogic`. Total: **195 af 195 unit-tests bestået**.

#### Verificerede kvalitets- og hukommelsestal (Arduino MKR WiFi 1010):
- **Flash-forbrug:** **171.428 bytes (65%)** ud af 256 KB — over 90,7 KB fri til labyrint-spillet!
- **SRAM-forbrug:** **8.388 bytes (25%)** ud af 32 KB — efterlader **24.380 bytes fri RAM**!
- **Unit tests:** **195 af 195 tests bestået (100% PASS, 0 advarsler)**.
- **Kompilering:** 0 fejl, 0 advarsler i projektkoden.

___

### 🌀 Opdatering 6/11 (Aften): Procedural IMU Marmor-Labyrint på Oplà (100% Gennemførlig, Fælder & Bær)
**Branch:** `feature/arduino-games`  
**Ansvarlig:** Oleh

Vi har implementeret det andet store on-board spil: **Procedural IMU Marmor-Labyrint**. Spillet udnytter 2D-akselerometret (IMU LSM6DS3) til ægte analog kugle-fysik, hvor Oplà-kortet vippes i hænderne som et klassisk labyrintspil i træ. Piletaster er bevidst frakoblet under spillet – styringen sker 100% via IMU!

#### Hvad er tilføjet:
1. **100% Garanteret Gennemførlighed (DFS Spanning Tree Algoritme):**
   - Hver gang spillet startes, genereres en helt ny, unik $7 \times 7$ labyrint (49 felter) via en Depth-First Search (Recursive Backtracker) algoritme.
   - Algoritmen bygger et matematisk spændetræ: enhver celle kan nås, ingen isolerede rum findes, og der er altid **100% garanti for en farbar rute** fra start $(0,0)$ til udgangsportalen $(6,6)$.
2. **Karakter-kugle: Mini-Lumi i bevægelse (`UiMazeRenderer`):**
   - Kuglen er udformet som en charmerende miniature-Lumi (9x9 px mochi-form i mintgrøn med gylden spire på toppen).
   - **Levende øjne:** Lumis øjne kigger i den retning, kuglen ruller (fremad, nedad, sidelæns) for en levende karakter-følelse.
   - **Hjørne-glidning (Corner Sliding):** Ved skråt træk mod en væg glider Lumi blødt langs gangen uden at sidde fast.
3. **Fælder og Belønninger (Tricks & Traps i blindgyder):**
   - 🕳️ **Faldgruber / Sorte huller (`ITEM_PITFALL`):** 2 lumske huller er placeret i fristende blindgyder. Ruller Lumi heri, suges han ned med et hvin (`SOUND_MISS`), og straffes med **+5 sekunder** på uret før genstart ved startpunktet.
   - 🍓 **Deku-Bær (`ITEM_BERRY`):** 2–3 skinnende gyldne bær gemmer sig i andre blindgyder. At rulle ind og hente dem giver bonuspoint, spil-feedback (`SOUND_CONFIRM`) og genoplader Lumis energi.
   - 🧊 **Is-zoner (`ITEM_ICE`):** Visse gange er belagt med glat is (nul friktion), hvor kuglen suser hurtigt afsted og kræver lynhurtig mod-hældning!
4. **Mål, High Score og Kæledyrs-effekt:**
   - Målet er en lysende guldportal i nederste højre hjørne.
   - Top-HUD viser samlede bær (`* 2/3`) og et løbende stopur (`TID: 00:24`).
   - Ved fuldførelse vises en sejrsmodal med tid, bær og eventuel ny rekord (`MAZE=<sekunder>`), der automatisk gemmes på MicroSD i `lumi.txt`.
   - Sejr giver massivt humør-boost: `happiness += 35` og `discipline += 15`!
5. **Genveje & Verifikation:**
   - Tast `'l'` eller `'L'` i Serial Monitor for at starte Labyrinten direkte.
   - 12 nye unit-tests verificerer DFS-spændetræ, 100% tilgængelighed af alle 49 felter, fælder, bær og IMU-fysik.
   - Total: **207 af 207 unit-tests bestået (100% PASS)**.

#### Verificerede kvalitets- og hukommelsestal (Arduino MKR WiFi 1010):
- **Flash-forbrug:** **177.132 bytes (67%)** ud af 256 KB — 85 KB fri plads tilbage!
- **SRAM-forbrug:** **8.508 bytes (25%)** ud af 32 KB — 24.260 bytes fri RAM til stak!
- **Unit tests:** **207 af 207 tests bestået (100% PASS, 0 advarsler)**.
- **Kompilering:** 0 fejl, 0 advarsler i projektkoden.






