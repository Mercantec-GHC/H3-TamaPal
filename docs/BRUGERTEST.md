# Brugertest (#18)

Krav (DoD #9): én elev **uden for gruppen** bruger TamaPal i mindst 10 minutter, vi noterer hvad der sker,
retter **én** ting og tester igen. Før/efter skrives i `PLAN.md`.

## Før testen (5 min)

- [ ] Hvem tester: ____________ (ikke fra gruppen) – dato/tid: ____________
- [ ] Opsætning A (hele flowet): Oplà med skitsen fra `dev` + backend kører + browser åben på `Frontend/index.html`
- [ ] Opsætning B (plan B): browser med `fakeGame.js` (piletaster) – kun hvis A ikke virker
- [ ] Én fra gruppen styrer testen, én skriver noter. **Vi hjælper ikke testeren**, medmindre hen sidder helt fast

## Sådan kører testen (10–15 min)

Sig kun: *"Det her er et spil. Prøv at spille det – tænk gerne højt."* Ingen forklaring af knapper eller vip.

| Tid | Hvad testeren gør | Hvad vi kigger efter |
|---|---|---|
| 0–2 min | Finder selv ud af at starte | Finder hen knappen (Oplà T2 eller "Start" i browseren)? Hvor længe tager det? |
| 2–7 min | Spiller 2–3 spil | Forstår hen, at vip styrer ketsjeren? Føles den for følsom / for langsom? Ser hen score og liv? |
| 7–10 min | Game over → "Spil igen" | Forstår hen, at spillet er slut, og hvordan man starter igen? |
| 10–15 min | Spørgsmål (nedenfor) | – |

## Spørgsmål bagefter

1. Hvad var det sværeste / mest forvirrende?
2. Hvordan styrede du ketsjeren? Føltes det rigtigt?
3. Hvis du måtte ændre én ting, hvad skulle det være?
4. Ville du spille igen? Hvorfor / hvorfor ikke?

## Noter (udfyldes under testen)

| # | Hvad skete der / hvad sagde testeren | Hvor (Oplà / backend / browser) |
|---|---|---|
| 1 | | |
| 2 | | |
| 3 | | |

## Forbedring

Vælg **én** ting, der er lille, sikker og nem at vise før/efter. Eksempler afhængigt af hvad vi ser:

- Vip for følsomt/trægt → juster `MAX_TILT_DEG` (Oplà, Oleh) – MESSAGES.md siger allerede, at 30° kan justeres efter testen
- Testeren finder ikke start → tydeligere tekst på startskærmen, f.eks. "Tryk T2 eller Start" (browser, Andrii)
- Bolden for hurtig/langsom → `StartSpeed` / `SpeedUp` (backend, Oliver)

Ændringen laves af den, der ejer delen. Kode-stop var 9/11, så hold ændringen lille.

- **Før:** ____________
- **Ændring:** ____________ (commit: ______)
- **Efter (test igen, samme eller ny tester):** ____________
