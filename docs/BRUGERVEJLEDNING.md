# 🐣 TamaPal – Brugervejledning & Spilmanual

Velkommen til **TamaPal**! Denne vejledning forklarer kort og præcist alt, hvad du behøver at vide for at passe dit kæledyr **Lumi**, spille de indbyggede arkadespil og forbinde til multiplayer i klassen.

---

## 🕹️ 1. Oplà-controllerens knapper

Arduino Oplà har en rund farveskærm omgivet af 5 kapacitive touch-knapper:

```
                  [TOUCH2] (02)
                   OK / Vælg
                      ▲
     [TOUCH1] (01)    │    [TOUCH3] (03)
      Op / Næste ◀───[Oplà]───▶ Tilbage / Menu
                      │
     [TOUCH0] (00)    ▼    [TOUCH4] (04)
     Ned / Forrige   Lumi  (Arkade-gem)
```

| Knap | Funktion på Hjem-skærmen | Funktion i spil |
|---|---|---|
| **TOUCH0 (00)** | Bladre til venstre i menuen (`<`) | Pong: Flyt ketsjer **NED** |
| **TOUCH1 (01)** | Bladre til højre i menuen (`>`) | Pong: Flyt ketsjer **OP** |
| **TOUCH2 (02)** | **OK / Bekræft** handling | Pong: Skyd bolden i gang / Labyrint: **Pause** |
| **TOUCH3 (03)** | **Tilbage** til Hjem (Hold 1.2s: Lyd til/fra) | Labyrint: Afslut (når sat på pause) |
| **TOUCH4 (04)** | Deaktiveret på Hjem (bruges ved navngivning) | Bekræft arkade-navn ved æggeklækning |
| **Håndbevægelse** | **Klap Lumi:** Stryg hånden 5–10 cm over skærmen | Lumi spinder af glæde (+12% glæde) ❤️ |

---

## 🧭 2. Hovedmenuen på Hjem-skærmen

Brug **TOUCH0** (`<`) og **TOUCH1** (`>`) til at bladre mellem de 7 ikoner i toppen, og tryk **TOUCH2** (`OK`):

1. 🥣 **1. MAD (Deku-bær):**  
   Fodrer Lumi med saftige bær (+25% mæthed, +8% energi, +5% glæde).
2. 🎮 **2. SPIL (Minispil):**  
   Åbner menuen med **Solo Pong** og **Marmor-Labyrint**. Kræver mindst 15% energi.
3. 🏠 **3. HJEM & SKAB (Hus-menu):**  
   Åbner husets faciliteter:
   - **Garderobe (Skab):** Skift mellem standardtøj og varm vinterhue & halstørklæde.
   - **Sengetid (Søvn):** Lægger Lumi til at sove. Han sover uforstyrret og vågner automatisk fuld af energi ved 100%!
   - **Mindelund (De Dødes Skov & Kirkegård):** Træd ind i den hellige skovlysning med fyrretræer, nattemåne, ildfluer og gravstene ("De Dødes Skov"). Vælg en forfader med **TOUCH0 / TOUCH1** og tryk **TOUCH2 (OK)** for at åbne deres fulde **Livscyklus og mindekrønike** (generation, dage levet, dødsårsag, arkaderekorder og gravskrift)! Tryk **TOUCH3** for at vende tilbage til skoven.
4. 💊 **4. MEDICIN:**  
   Helbreder Lumi, hvis han er blevet forkølet eller syg.
5. ⚡ **5. SKÆLD UD (Disciplin):**  
   Opdrager Lumi (+15% disciplin), hvis han laver narrestreger eller nægter at spise.
6. 📊 **6. STATUS (Statistik):**  
   Åbner et fuldskærms-dashboard med de 5 livs-parametre. Tryk `TOUCH3` for at vende tilbage.
7. 🌐 **7. FORBIND (Multiplayer):**  
   Forvandler Oplà til en trådløs controller for storskærms-Pong i browseren!

---

## ❤️ 3. De 5 Behov (Vitals)

Lumi er et levende væsen med 5 parametre (0–100%):

* **SULT:** Falder langsomt over tid. Fodr ham med bær via mad-ikonet.
* **VARM (Varme):** Måler den virkelige temperatur i klasseværelset! Hvis der er koldt (< 19°C), fryser Lumi — varm sensoren med fingeren eller giv ham vintertøj på i garderoben!
* **ENER (Energi):** Falder når han leger. Genvindes hurtigt ved at lægge ham i seng via Hjem-menuen eller ved at dække lyssensoren med hånden (mørke).
* **GLÆD (Glæde):** Stiger når du aer ham med hånden, vinder spil eller giver ham godbidder.
* **DISC (Disciplin):** Viser hvor velopdragen han er. Høj disciplin gør, at han som voksen udvikler sig til den vise **Harmoniske Skogsvogter** i stedet for en vild skovtrold!

---

## 🏓 4. Minispil: Solo Pong

Klassisk arkade-bordtennis mod en robot-modstander:
* **Styring:**  
  - Tryk **TOUCH1** for at flytte din ketsjer **OP**.  
  - Tryk **TOUCH0** for at flytte din ketsjer **NED**.  
  - Tryk **TOUCH2** for at skyde bolden i gang.
* **Effekter:**  
  - Bolden brænder som en flammende komet med glødende hale.  
  - Væggene slår gnister ved hårde rikochetter.  
  - Lille Lumi-cheerleader hepper i baggrunden med guld-pomponer!  
  - Bolden accelererer jævnt for hver vellykket duel.
* **Pause / Ud:**  
  - Under aktivt spil er `TOUCH3` beskyttet mod uheld. Hold `TOUCH3` i 1.2 sek for at afslutte tidligt, eller tryk `TOUCH3` efter Game Over for at vende tilbage til menuen.

---

## 🌀 5. Minispil: Marmor-Labyrint

Fysisk vippespil baseret på Oplà-kortets indbyggede IMU-bevægelsessensor:
* **Styring:**  
  - Tag fat i Oplà med begge hænder og **vip kortet** i den retning, du vil rulle!
* **Mini-Lumi som kugle:**  
  - Mini-Lumi ruller som en lille kugle og viser vilde ansigtsudtryk `(O_O)` med åben mund, når farten bliver høj!
* **Elementer på banen:**  
  - 🍓 **Deku-bær:** Saml de lysende bær. Hvert bær gemmes i din pung og kan bruges til mad og medicin!  
  - 🕳️ **Kosmiske Sorte Huller:** Pulserende lilla vortex-huller med tyngdekraft, der suger dig ind, hvis du kommer for tæt på!  
  - 🌀 **Magisk Port:** Målet i nederste højre hjørne. Rul ind i den roterende stjerne-portal for at vinde!
* **Pause og sikkerhed:**  
  - Tryk **TOUCH2** for at sætte spillet på **PAUSE**.  
  - På pause trykker du **TOUCH3** for at afslutte til menuen, eller **TOUCH2** for at fortsætte.

---

## 🌐 6. Multiplayer: Spil mod klassen

1. Vælg ikon **7 (Forbind)** i menuen.
2. Vælg **Spiller 1 (Venstre)** med `TOUCH0` eller **Spiller 2 (Højre)** med `TOUCH1`.
3. Oplà forbinder automatisk til MQTT-serveren via Wi-Fi.
4. Åbn web-browseren på computeren — din Oplà styrer nu ketsjeren direkte på storskærmen!

---

## ✨ 7. Hemmelige funktioner & genveje

* **Biograf Showcase-tilstand:**  
  Hold **TOUCH1 + TOUCH3 nede samtidigt i 0.8 sekunder** på Hjem-skærmen. Oplà starter en fuldautomatisk animationsfilm, der viser hele Lumis livscyklus fra æg til alderdom med musik og lysshow! (Tryk `TOUCH3` for at afbryde).
* **Slå lyd fra / til (Mute):**  
  Hold **TOUCH3 nede i 1.2 sekunder** på Hjem-skærmen for at slå lyden fra eller til.
* **Livscyklus og genfødsel:**  
  Lumi lever et fuldt, langt liv (op til 100 dage). Hvis han en dag sover stille ind af alderdom, trykker du på **TOUCH2**, staver din næste generations navn på arkade-tavlen, og et nyt magisk æg klækkes med generationstæller på MicroSD!

