// WHAT: MicroSD persistence manager implementation for Lumi's Tamagotchi vitals
// WHY:  Clean non-blocking SD read/write with graceful fallback if no card is inserted
// USED BY: TamaPal.ino, tests/test_main.cpp

#include "storage_pet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(ARDUINO)
  #include <Arduino.h>
  #include <SD.h>
  #ifndef SD_CS
    #define SD_CS 0
  #endif
#endif

StoragePet::StoragePet()
  : lastSavedHash(0),
    cardInitialized(false) {
}

uint32_t StoragePet::computeHash(const char* str) {
  uint32_t hash = 2166136261u;
  if (str == nullptr) return hash;
  while (*str) {
    hash ^= (uint8_t)*str++;
    hash *= 16777619u;
  }
  return hash;
}

bool StoragePet::begin() {
#if defined(ARDUINO)
  cardInitialized = SD.begin(SD_CS);
  if (cardInitialized) {
    Serial.println(F("[STORAGE] MicroSD-kort fundet og klar til brug!"));
  } else {
    Serial.println(F("[STORAGE] Intet MicroSD-kort fundet (koerer i RAM uden gemning)."));
  }
  return cardInitialized;
#else
  cardInitialized = false;
  return false;
#endif
}

bool StoragePet::isCardReady() const {
  return cardInitialized;
}

uint32_t StoragePet::getLastSavedHash() const {
  return lastSavedHash;
}

void StoragePet::serializeState(char* buffer, size_t bufSize, const PetLogic& pet) {
  if (buffer == nullptr || bufSize == 0) return;
  snprintf(buffer, bufSize,
           "HUNGER=%d\nWARMTH=%d\nENERGY=%d\nHAPPINESS=%d\nDISCIPLINE=%d\nASLEEP=%d\nSTAGE=%d\nOUTFIT=%d\nPONG=%u\nMAZE=%u\nAGE=%lu\nGEN=%u\nALIVE=%d\nDEATH=%d\n",
           pet.getHunger(),
           pet.getWarmth(),
           pet.getEnergy(),
           pet.getHappiness(),
           pet.getDiscipline(),
           pet.isSleeping() ? 1 : 0,
           (int)pet.getStageEnum(),
           (int)pet.getOutfitEnum(),
           (unsigned int)pet.getHighScorePong(),
           (unsigned int)pet.getHighScoreMaze(),
           (unsigned long)pet.getAgeMinutes(),
           (unsigned int)pet.getGeneration(),
           pet.isAlive() ? 1 : 0,
           (int)pet.getDeathCause());
}

bool StoragePet::deserializeState(const char* text,
                                 float& hunger,
                                 float& warmth,
                                 float& energy,
                                 float& happiness,
                                 float& discipline,
                                 bool& asleep) {
  PetStage unusedStage = STAGE_BABY;
  PetOutfit unusedOutfit = OUTFIT_DEFAULT;
  return deserializeState(text, hunger, warmth, energy, happiness, discipline, asleep, unusedStage, unusedOutfit);
}

bool StoragePet::deserializeState(const char* text,
                                 float& hunger,
                                 float& warmth,
                                 float& energy,
                                 float& happiness,
                                 float& discipline,
                                 bool& asleep,
                                 PetStage& stage) {
  PetOutfit unusedOutfit = OUTFIT_DEFAULT;
  return deserializeState(text, hunger, warmth, energy, happiness, discipline, asleep, stage, unusedOutfit);
}

bool StoragePet::deserializeState(const char* text,
                                 float& hunger,
                                 float& warmth,
                                 float& energy,
                                 float& happiness,
                                 float& discipline,
                                 bool& asleep,
                                 PetStage& stage,
                                 PetOutfit& outfit) {
  uint16_t dummyPong = 0;
  uint16_t dummyMaze = 0;
  uint32_t dummyAge = 0;
  return deserializeState(text, hunger, warmth, energy, happiness, discipline, asleep, stage, outfit, dummyPong, dummyMaze, dummyAge);
}

bool StoragePet::deserializeState(const char* text,
                                 float& hunger,
                                 float& warmth,
                                 float& energy,
                                 float& happiness,
                                 float& discipline,
                                 bool& asleep,
                                 PetStage& stage,
                                 PetOutfit& outfit,
                                 uint16_t& pong,
                                 uint16_t& maze,
                                 uint32_t& age,
                                 uint16_t& generation,
                                 bool& alive,
                                 DeathCause& deathCause) {
  if (text == nullptr || text[0] == '\0') return false;

  int h = -1, w = -1, e = -1, hap = -1, d = -1, s = -1;
  int st = (int)STAGE_BABY;       // Default to baby if not present in legacy file
  int outf = (int)OUTFIT_DEFAULT; // Default to natural fur if not present in legacy file
  int pVal = 0;
  int mVal = 0;
  long aVal = 0;
  int gVal = 1;
  int alVal = 1;
  int dcVal = 0;
  const char* p = text;

  // Robust line-by-line parser supporting arbitrary line order
  while (*p != '\0') {
    if (strncmp(p, "HUNGER=", 7) == 0) {
      h = atoi(p + 7);
    } else if (strncmp(p, "WARMTH=", 7) == 0) {
      w = atoi(p + 7);
    } else if (strncmp(p, "ENERGY=", 7) == 0) {
      e = atoi(p + 7);
    } else if (strncmp(p, "HAPPINESS=", 10) == 0) {
      hap = atoi(p + 10);
    } else if (strncmp(p, "DISCIPLINE=", 11) == 0) {
      d = atoi(p + 11);
    } else if (strncmp(p, "ASLEEP=", 7) == 0) {
      s = atoi(p + 7);
    } else if (strncmp(p, "STAGE=", 6) == 0) {
      st = atoi(p + 6);
    } else if (strncmp(p, "OUTFIT=", 7) == 0) {
      outf = atoi(p + 7);
    } else if (strncmp(p, "PONG=", 5) == 0) {
      pVal = atoi(p + 5);
    } else if (strncmp(p, "MAZE=", 5) == 0) {
      mVal = atoi(p + 5);
    } else if (strncmp(p, "AGE=", 4) == 0) {
      aVal = atol(p + 4);
    } else if (strncmp(p, "GEN=", 4) == 0) {
      gVal = atoi(p + 4);
    } else if (strncmp(p, "ALIVE=", 6) == 0) {
      alVal = atoi(p + 6);
    } else if (strncmp(p, "DEATH=", 6) == 0) {
      dcVal = atoi(p + 6);
    }

    const char* nextLine = strchr(p, '\n');
    if (nextLine == nullptr) break;
    p = nextLine + 1;
  }

  // Validate that all fields are within valid bounds
  if (h < 0 || h > 100 ||
      w < 0 || w > 100 ||
      e < 0 || e > 100 ||
      hap < 0 || hap > 100 ||
      d < 0 || d > 100 ||
      s < 0 || s > 1 ||
      st < 0 || st > 3 ||
      outf < 0 || outf > 1 ||
      pVal < 0 || mVal < 0 || aVal < 0 ||
      gVal < 1 || alVal < 0 || alVal > 1 || dcVal < 0 || dcVal > 6) {
    return false;
  }

  hunger = (float)h;
  warmth = (float)w;
  energy = (float)e;
  happiness = (float)hap;
  discipline = (float)d;
  asleep = (s == 1);
  stage = (PetStage)st;
  outfit = (PetOutfit)outf;
  pong = (uint16_t)pVal;
  maze = (uint16_t)mVal;
  age = (uint32_t)aVal;
  generation = (uint16_t)gVal;
  alive = (alVal == 1);
  deathCause = (DeathCause)dcVal;
  return true;
}

bool StoragePet::deserializeState(const char* text,
                                 float& hunger,
                                 float& warmth,
                                 float& energy,
                                 float& happiness,
                                 float& discipline,
                                 bool& asleep,
                                 PetStage& stage,
                                 PetOutfit& outfit,
                                 uint16_t& pong,
                                 uint16_t& maze,
                                 uint32_t& age) {
  uint16_t gen = 1;
  bool al = true;
  DeathCause dc = DEATH_NONE;
  return deserializeState(text, hunger, warmth, energy, happiness, discipline, asleep, stage, outfit, pong, maze, age, gen, al, dc);
}

bool StoragePet::savePet(const PetLogic& pet) {
#if defined(ARDUINO)
  if (!cardInitialized) return false;

  char buffer[256];
  serializeState(buffer, sizeof(buffer), pet);

  uint32_t currentHash = computeHash(buffer);
  if (currentHash == lastSavedHash) {
    return true; // Zero SPI bus traffic when pet state is identical
  }

  // Remove stale file before writing to prevent appending
  if (SD.exists(PET_STORAGE_FILENAME)) {
    SD.remove(PET_STORAGE_FILENAME);
  }

  File file = SD.open(PET_STORAGE_FILENAME, FILE_WRITE);
  if (!file) {
    Serial.println(F("[STORAGE] FEJL: Kunne ikke aabne lumi.txt til skrivning!"));
    return false;
  }

  file.print(buffer);
  file.close();
  lastSavedHash = currentHash;
  Serial.println(F("[STORAGE] Lumi gemt paa MicroSD-kort."));
  return true;
#else
  (void)pet;
  return false;
#endif
}

bool StoragePet::loadPet(PetLogic& pet) {
#if defined(ARDUINO)
  if (!cardInitialized) return false;

  if (!SD.exists(PET_STORAGE_FILENAME)) {
    Serial.println(F("[STORAGE] Ingen tidligere gemt lumi.txt fundet paa SD-kort."));
    return false;
  }

  File file = SD.open(PET_STORAGE_FILENAME, FILE_READ);
  if (!file) {
    Serial.println(F("[STORAGE] FEJL: Kunne ikke laese lumi.txt fra SD-kort!"));
    return false;
  }

  char buffer[256];
  int bytesRead = 0;
  while (file.available() && bytesRead < (int)(sizeof(buffer) - 1)) {
    buffer[bytesRead++] = (char)file.read();
  }
  buffer[bytesRead] = '\0';
  file.close();

  float h = 0, w = 0, e = 0, hap = 0, d = 0;
  bool asleep = false;
  PetStage stage = STAGE_BABY;
  PetOutfit outfit = OUTFIT_DEFAULT;
  uint16_t pong = 0;
  uint16_t maze = 0;
  uint32_t age = 0;
  uint16_t gen = 1;
  bool alive = true;
  DeathCause deathCause = DEATH_NONE;
  if (deserializeState(buffer, h, w, e, hap, d, asleep, stage, outfit, pong, maze, age, gen, alive, deathCause)) {
    pet.setVitals(h, w, e, hap, d);
    pet.setSleeping(asleep);
    pet.setStage(stage);
    pet.setOutfit(outfit);
    pet.setHighScorePong(pong);
    pet.setHighScoreMaze(maze);
    pet.setAgeMinutes(age);
    pet.setGeneration(gen);
    if (!alive) {
      pet.triggerDeath(deathCause);
    }
    lastSavedHash = computeHash(buffer);
    Serial.print(F("[STORAGE] SUCCES: Indlaeste Lumi fra MicroSD! GEN: "));
    Serial.print((unsigned int)gen);
    Serial.print(F(", Status: "));
    Serial.print(alive ? F("Levende") : F("Doed"));
    Serial.print(F(", Sult: "));
    Serial.print((int)h);
    Serial.print(F("%, Energi: "));
    Serial.print((int)e);
    Serial.print(F("%, Stadie: "));
    Serial.print(pet.getStage());
    Serial.print(F(", Alder: "));
    Serial.print((unsigned long)age);
    Serial.println(F(" min"));
    return true;
  } else {
    Serial.println(F("[STORAGE] ADVARSEL: lumi.txt data var ugyldig, starter forfra."));
    return false;
  }
#else
  (void)pet;
  return false;
#endif
}

static const uint8_t MAX_RAM_GRAVE_RECORDS = 5;
static GraveRecord ramGraveRecords[MAX_RAM_GRAVE_RECORDS];
static uint8_t ramGraveCount = 0;

bool StoragePet::logGraveRecord(const PetLogic& pet) {
  // Always update in-memory ring buffer (acts as fallback if no SD or during tests)
  GraveRecord rec;
  rec.generation = pet.getGeneration();
  rec.ageMinutes = pet.getAgeMinutes();
  rec.deathCause = pet.getDeathCause();
  rec.highScorePong = pet.getHighScorePong();
  rec.highScoreMaze = pet.getHighScoreMaze();
  strncpy(rec.epitaph, pet.getDeathEpitaph(), sizeof(rec.epitaph) - 1);
  rec.epitaph[sizeof(rec.epitaph) - 1] = '\0';

  if (ramGraveCount < MAX_RAM_GRAVE_RECORDS) {
    ramGraveRecords[ramGraveCount++] = rec;
  } else {
    for (uint8_t i = 0; i < MAX_RAM_GRAVE_RECORDS - 1; i++) {
      ramGraveRecords[i] = ramGraveRecords[i + 1];
    }
    ramGraveRecords[MAX_RAM_GRAVE_RECORDS - 1] = rec;
  }

#if defined(ARDUINO)
  if (!cardInitialized) return false;

  File file = SD.open("grave.txt", FILE_WRITE);
  if (!file) {
    Serial.println(F("[STORAGE] FEJL: Kunne ikke aabne grave.txt til mindeskrift!"));
    return false;
  }

  char record[220];
  snprintf(record, sizeof(record),
           "GEN %u | Alder: %lu min | Doed: %s | Pong: %u | Labyrint: %u sek | \"%s\"\n",
           (unsigned int)pet.getGeneration(),
           (unsigned long)pet.getAgeMinutes(),
           pet.getDeathCauseName(),
           (unsigned int)pet.getHighScorePong(),
           (unsigned int)pet.getHighScoreMaze(),
           pet.getDeathEpitaph());
  file.print(record);
  file.close();

  Serial.print(F("[STORAGE] Mindeskrift tilfoejet til grave.txt for GEN "));
  Serial.println((unsigned int)pet.getGeneration());
  return true;
#else
  return true;
#endif
}

bool StoragePet::parseGraveLine(const char* line, GraveRecord& outRecord) {
  if (line == nullptr || line[0] == '\0') return false;

  const char* pGen = strstr(line, "GEN ");
  if (!pGen) return false;
  outRecord.generation = (uint16_t)atoi(pGen + 4);

  const char* pAge = strstr(line, "| Alder: ");
  if (!pAge) return false;
  outRecord.ageMinutes = (uint32_t)atol(pAge + 9);

  const char* pDeath = strstr(line, "| Doed: ");
  if (!pDeath) return false;
  const char* causeStart = pDeath + 8;
  const char* causeEnd = strchr(causeStart, '|');
  if (!causeEnd) return false;

  if (strstr(causeStart, "sult") && causeStart < causeEnd) {
    outRecord.deathCause = DEATH_HUNGER;
  } else if (strstr(causeStart, "ihjel") && causeStart < causeEnd) {
    outRecord.deathCause = DEATH_COLD;
  } else if (strstr(causeStart, "hedeslag") && causeStart < causeEnd) {
    outRecord.deathCause = DEATH_HEAT;
  } else if (strstr(causeStart, "ensomhed") && causeStart < causeEnd) {
    outRecord.deathCause = DEATH_BOREDOM;
  } else if (strstr(causeStart, "udmattelse") && causeStart < causeEnd) {
    outRecord.deathCause = DEATH_EXHAUSTION;
  } else if (strstr(causeStart, "alderdom") && causeStart < causeEnd) {
    outRecord.deathCause = DEATH_OLD_AGE;
  } else {
    outRecord.deathCause = DEATH_NONE;
  }

  const char* pPong = strstr(line, "| Pong: ");
  outRecord.highScorePong = pPong ? (uint16_t)atoi(pPong + 8) : 0;

  const char* pMaze = strstr(line, "| Labyrint: ");
  outRecord.highScoreMaze = pMaze ? (uint16_t)atoi(pMaze + 12) : 0;

  const char* pQuoteStart = strchr(line, '"');
  outRecord.epitaph[0] = '\0';
  if (pQuoteStart) {
    pQuoteStart++; // skip opening quote
    const char* pQuoteEnd = strchr(pQuoteStart, '"');
    size_t qLen = pQuoteEnd ? (size_t)(pQuoteEnd - pQuoteStart) : strlen(pQuoteStart);
    if (qLen >= sizeof(outRecord.epitaph)) {
      qLen = sizeof(outRecord.epitaph) - 1;
    }
    memcpy(outRecord.epitaph, pQuoteStart, qLen);
    outRecord.epitaph[qLen] = '\0';
  }

  return (outRecord.generation > 0);
}

uint16_t StoragePet::getGraveRecordCount() {
#if defined(ARDUINO)
  if (!cardInitialized || !SD.exists("grave.txt")) {
    return (uint16_t)ramGraveCount;
  }
  File file = SD.open("grave.txt", FILE_READ);
  if (!file) {
    return (uint16_t)ramGraveCount;
  }
  uint16_t count = 0;
  while (file.available()) {
    char c = (char)file.read();
    if (c == '\n') {
      count++;
    }
  }
  file.close();
  return (count > 0) ? count : (uint16_t)ramGraveCount;
#else
  return (uint16_t)ramGraveCount;
#endif
}

bool StoragePet::loadGraveRecord(uint16_t index, GraveRecord& outRecord) {
#if defined(ARDUINO)
  if (!cardInitialized || !SD.exists("grave.txt")) {
    if (index < ramGraveCount) {
      outRecord = ramGraveRecords[index];
      return true;
    }
    return false;
  }

  File file = SD.open("grave.txt", FILE_READ);
  if (!file) {
    if (index < ramGraveCount) {
      outRecord = ramGraveRecords[index];
      return true;
    }
    return false;
  }

  uint16_t currentLine = 0;
  char lineBuf[220];
  int linePos = 0;
  bool found = false;

  while (file.available()) {
    char c = (char)file.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineBuf[linePos] = '\0';
      if (currentLine == index) {
        found = parseGraveLine(lineBuf, outRecord);
        break;
      }
      currentLine++;
      linePos = 0;
    } else if (linePos < (int)(sizeof(lineBuf) - 1)) {
      lineBuf[linePos++] = c;
    }
  }

  if (!found && linePos > 0 && currentLine == index) {
    lineBuf[linePos] = '\0';
    found = parseGraveLine(lineBuf, outRecord);
  }

  file.close();

  if (!found && index < ramGraveCount) {
    outRecord = ramGraveRecords[index];
    return true;
  }
  return found;
#else
  if (index < ramGraveCount) {
    outRecord = ramGraveRecords[index];
    return true;
  }
  return false;
#endif
}

