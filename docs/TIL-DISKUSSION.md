# Til diskussion – fra Andrii (til fredag)

1. **Dataflow:** Arduino → backend via MQTT (praktisk og bedre end HTTP til mange små beskeder) → frontend i browseren via SignalR.
2. **JSON-format:** hvilke data har vi brug for at arbejde videre? (bold, ketsjere, point, TamaPals statistik)
3. **Frontend:** Blazor eller JS/canvas?
4. **Login/JWT, Docker og deploy:** til sidst.
5. **Hvem styrer bolden:** backend eller browseren?
6. **Git-branches:** intet direkte i `main` – kun releases (hver fredag).
   - `dev` er vores fælles arbejdsbranch.
   - Hver opgave får sin egen korte branch, fx `frontend/pingpong-canvas`, `backend/signalr-hub`, `arduino/pet-needs`.
   - Færdigt arbejde → Pull Request ind i `dev`, en anden fra gruppen kigger det igennem.
   - Fredag før release: `dev` → `main` + tag (fx `v0.3.0`).

## Min vision for dataflowet (forslag)

```
Arduino ──MQTT──► Backend + database ──SignalR──► Browser
```

- **Arduino → backend:** Oplàen sender små JSON-beskeder til serveren via MQTT (eller HTTP, hvis vi hellere vil det). Fx TamaPals behov hvert 30. sekund og hældningen (IMU), når vi spiller.
- **Backend → browser:** serveren sender data videre til browseren via SignalR. Browseren skal ikke selv spørge (ingen request/response) – den får bare nye data automatisk.
- **Kæledyret bor på Arduinoen:** sult, søvn, vækst osv. regnes ud på Oplàen. Backenden gemmer bare en kopi i databasen, så TamaPal ikke forsvinder, når strømmen går.
- **Kampen bor på backenden:** de to Oplàer er bare controllere (ligesom i Bomberman). Backenden flytter bolden, tæller point og gemmer resultatet i databasen. Browseren tegner kun det, den får.
- **Statistik:** browseren henter historik og highscores fra backenden med almindelige requests (REST).
- **Arbejde samtidig:** hvis vi aftaler JSON-formatet først, kan alle tre starte med det samme – med falske testdata, indtil de andre dele er klar.





**En skitse af hele spillets loop – fra Oliver:** (Kan rettes, tilføjes og slettes)
Fase 1 – Lumi bliver født

1. Arduinoen starter, og en ny Lumi bliver født med et unikt ID.
2. Lumi har behov som sult, glæde, energi, varme og disciplin.
3. Spilleren passer Lumi med Arduinoens knapper og sensorer.
4. Lumi udvikler sig fra baby til teenager, voksen og gammel.
5. Hvis Lumi ikke bliver passet, kan den dø.
   
Fase 2 – Forbind til multiplayer

7. Spilleren vælger Connect på Arduinoen.
8. Arduinoen forbinder til backend via Wi-Fi og MQTT (Mosquitto).
9. ASP.NET Core registrerer Lumi som online.
10. Backend sender online-status til hjemmesiden via SignalR.
11. Spilleren åbner hjemmesiden og forbinder sin browser med sin Arduino.
    
Fase 3 – Fælles lobby

13. Hjemmesiden viser alle online Tamagotchier med navn, karakter og status.
14. Spillere kan se, hvem der er ledige til at spille.
15. På samme hjemmeside findes en kirkegård med tidligere døde Tamagotchier.
16. Der findes også en highscore-liste med tidligere spilresultater.
    
Fase 4 – Inviter til minispil

18. Spilleren vælger en anden Tamagotchi og inviterer til Pong eller labyrint-race.
19. Backend sender invitationen til modstanderen via SignalR.
20. Modstanderen accepterer eller afviser invitationen.
21. Ved accept opretter backend en fælles spilsession.
22. Begge Arduinoer skifter til controller-tilstand.
    
Fase 5 – Spil minispillet

24. Minispillet vises i begge spilleres browsere.
25. Arduinoerne sender knaptryk og bevægelser til backend via MQTT.
26. Backend beregner spillets bevægelser, kollisioner og point.
27. Backend sender løbende spiltilstanden til browserne via SignalR.
28. Frontend viser spillet med JavaScript Canvas.
    
Fase 6 – Resultat og belønninger

30. Når spillet afsluttes, beregner backend vinderen.
31. Resultatet og scoren gemmes i Neon PostgreSQL.
32. Begge spillere får vist resultatet i browseren.
33. Backend sender eventuelle belønninger til Arduinoerne via MQTT.
34. Arduinoerne vender tilbage til normal Tamagotchi-tilstand, og spillerne kan fortsætte i lobbyen.
    
Fase 7 – Lumi dør

36. Lumi fortsætter sit liv på Arduinoen og kan dø af sult, kulde eller alderdom.
37. Arduinoen registrerer dødsfaldet og sender oplysningerne via MQTT.
38. Backend modtager og gemmer Lumis navn, levetid, udviklingstrin og dødsårsag i Neon.
39. Backend giver hjemmesiden besked via SignalR.
40. Kirkegården opdateres automatisk med en ny gravsten og en død version af Lumi.
41. Alle spillere kan besøge kirkegården og se tidligere døde Tamagotchier.
    
Fase 8 – Nyt liv

43. Når Arduinoen genstartes, fødes en ny Lumi med nye startstatistikker og et nyt ID.
44. Den gamle Lumi kan ikke genoplives, men gravstenen bliver liggende i databasen.
45. Hele spillets loop starter forfra.





