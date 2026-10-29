# Til diskussion – fra Andrii (til fredag)

1. **Dataflow:** Arduino → backend via MQTT (praktisk og bedre end HTTP til mange små beskeder) → frontend i browseren via SignalR.
2. **JSON-format:** hvilke data har vi brug for at arbejde videre? (bold, ketsjere, point, TamaPals statistik)
3. **Frontend:** Blazor eller JS/canvas?
4. **Login/JWT, Docker og deploy:** til sidst.
5. **Hvem styrer bolden:** backend eller browseren?

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
