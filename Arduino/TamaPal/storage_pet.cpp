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
           "NAME=%s\nHUNGER=%d\nWARMTH=%d\nENERGY=%d\nHAPPINESS=%d\nDISCIPLINE=%d\nASLEEP=%d\nSTAGE=%d\nOUTFIT=%d\nPONG=%u\nMAZE=%u\nAGE=%lu\nGEN=%u\nALIVE=%d\nDEATH=%d\nBERRIES=%u\nSICK=%d\nPOOP=%d\n",
           pet.getName(),
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
           (int)pet.getDeathCause(),
           (unsigned int)pet.getBerries(),
           pet.isSick() ? 1 : 0,
           pet.getHasPoop() ? 1 : 0);
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
                                 DeathCause& deathCause,
                                 uint16_t& berries,
                                 bool& sick,
                                 bool& poop) {
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
  int bVal = 0;
  int sickVal = 0;
  int poopVal = 0;
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
    } else if (strncmp(p, "BERRIES=", 8) == 0) {
      bVal = atoi(p + 8);
    } else if (strncmp(p, "SICK=", 5) == 0) {
      sickVal = atoi(p + 5);
    } else if (strncmp(p, "POOP=", 5) == 0) {
      poopVal = atoi(p + 5);
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
      gVal < 1 || alVal < 0 || alVal > 1 || dcVal < 0 || dcVal > 6 ||
      bVal < 0 || sickVal < 0 || sickVal > 1 || poopVal < 0 || poopVal > 1) {
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
  berries = (uint16_t)bVal;
  sick = (sickVal == 1);
  poop = (poopVal == 1);
  return true;
}

void StoragePet::extractName(const char* text, char* nameOut, size_t maxLen) {
  if (nameOut == nullptr || maxLen == 0) return;
  nameOut[0] = '\0';

  if (text == nullptr) {
    strncpy(nameOut, "LUMI", maxLen - 1);
    nameOut[maxLen - 1] = '\0';
    return;
  }

  const char* p = strstr(text, "NAME=");
  if (p) {
    p += 5;
    size_t i = 0;
    while (*p != '\0' && *p != '\r' && *p != '\n' && i < maxLen - 1) {
      nameOut[i++] = *p++;
    }
    nameOut[i] = '\0';
  }

  if (nameOut[0] == '\0') {
    strncpy(nameOut, "LUMI", maxLen - 1);
    nameOut[maxLen - 1] = '\0';
  }
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
  uint16_t dummyBerries = 0;
  bool dummySick = false;
  bool dummyPoop = false;
  return deserializeState(text, hunger, warmth, energy, happiness, discipline, asleep, stage, outfit, pong, maze, age, generation, alive, deathCause, dummyBerries, dummySick, dummyPoop);
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

#if defined(ARDUINO)
static void backupFile(const char* srcFilename, const char* bakFilename) {
  if (!SD.exists(srcFilename)) return;
  if (SD.exists(bakFilename)) {
    SD.remove(bakFilename);
  }
  File src = SD.open(srcFilename, FILE_READ);
  if (!src) return;
  File bak = SD.open(bakFilename, FILE_WRITE);
  if (!bak) {
    src.close();
    return;
  }
  uint8_t copyBuf[64];
  while (src.available()) {
    int bytes = src.read(copyBuf, sizeof(copyBuf));
    if (bytes > 0) {
      bak.write(copyBuf, bytes);
    }
  }
  bak.flush();
  bak.close();
  src.close();
}

static bool readFileToBuffer(const char* filename, char* buffer, size_t bufSize) {
  if (!SD.exists(filename)) return false;
  File file = SD.open(filename, FILE_READ);
  if (!file) return false;
  int bytesRead = 0;
  while (file.available() && bytesRead < (int)(bufSize - 1)) {
    buffer[bytesRead++] = (char)file.read();
  }
  buffer[bytesRead] = '\0';
  file.close();
  return (bytesRead > 0);
}

static bool applyStateToPet(const char* buffer, PetLogic& pet, uint32_t& hashOut) {
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
  uint16_t berries = 0;
  bool sick = false;
  bool poop = false;
  if (StoragePet::deserializeState(buffer, h, w, e, hap, d, asleep, stage, outfit, pong, maze, age, gen, alive, deathCause, berries, sick, poop)) {
    pet.setVitals(h, w, e, hap, d);
    pet.setSleeping(asleep);
    pet.setStage(stage);
    pet.setOutfit(outfit);
    pet.setHighScorePong(pong);
    pet.setHighScoreMaze(maze);
    pet.setAgeMinutes(age);
    pet.setGeneration(gen);
    pet.setBerries(berries);
    pet.setSick(sick);
    pet.setHasPoop(poop);
    char parsedName[8] = "LUMI";
    StoragePet::extractName(buffer, parsedName, sizeof(parsedName));
    pet.setName(parsedName);
    if (!alive) {
      pet.triggerDeath(deathCause);
    }
    hashOut = StoragePet::computeHash(buffer);
    return true;
  }
  return false;
}
#endif

bool StoragePet::savePet(const PetLogic& pet) {
#if defined(ARDUINO)
  if (!cardInitialized) return false;

  char buffer[256];
  serializeState(buffer, sizeof(buffer), pet);

  uint32_t currentHash = computeHash(buffer);
  if (currentHash == lastSavedHash) {
    return true; // Zero SPI bus traffic when pet state is identical
  }

  // Backup current valid file before modifying to prevent data loss on sudden power-off
  if (SD.exists(PET_STORAGE_FILENAME)) {
    backupFile(PET_STORAGE_FILENAME, PET_BACKUP_FILENAME);
    SD.remove(PET_STORAGE_FILENAME);
  }

  File file = SD.open(PET_STORAGE_FILENAME, FILE_WRITE);
  if (!file) {
    Serial.println(F("[STORAGE] FEJL: Kunne ikke aabne lumi.txt til skrivning!"));
    return false;
  }

  file.print(buffer);
  file.flush();
  file.close();
  lastSavedHash = currentHash;
  Serial.println(F("[STORAGE] Lumi gemt paa MicroSD (sikret med lumi.bak)."));
  return true;
#else
  (void)pet;
  return false;
#endif
}

bool StoragePet::loadPet(PetLogic& pet) {
#if defined(ARDUINO)
  if (!cardInitialized) return false;

  char buffer[256];
  bool loadedFromPrimary = false;

  // 1. First attempt: Read primary state file lumi.txt
  if (readFileToBuffer(PET_STORAGE_FILENAME, buffer, sizeof(buffer))) {
    if (applyStateToPet(buffer, pet, lastSavedHash)) {
      loadedFromPrimary = true;
      Serial.println(F("[STORAGE] SUCCES: Indlaeste Lumi fra lumi.txt"));
    } else {
      Serial.println(F("[STORAGE] ADVARSEL: lumi.txt korrupt! Proever backup lumi.bak..."));
    }
  }

  // 2. Fallback attempt: If primary was missing or corrupt, restore from lumi.bak
  if (!loadedFromPrimary) {
    if (readFileToBuffer(PET_BACKUP_FILENAME, buffer, sizeof(buffer))) {
      if (applyStateToPet(buffer, pet, lastSavedHash)) {
        Serial.println(F("[STORAGE] SUCCES: Lumi gendannet fra sikkerhedskopi lumi.bak!"));
        // Re-create healthy lumi.txt from the restored backup immediately
        if (SD.exists(PET_STORAGE_FILENAME)) {
          SD.remove(PET_STORAGE_FILENAME);
        }
        File restoreFile = SD.open(PET_STORAGE_FILENAME, FILE_WRITE);
        if (restoreFile) {
          restoreFile.print(buffer);
          restoreFile.flush();
          restoreFile.close();
          Serial.println(F("[STORAGE] lumi.txt er blevet genoprettet fra backup."));
        }
        return true;
      } else {
        Serial.println(F("[STORAGE] FEJL: Backup lumi.bak var ogsaa korrupt."));
      }
    } else {
      Serial.println(F("[STORAGE] Ingen gyldig lumi.txt eller lumi.bak fundet. Starter forfra."));
    }
    return false;
  }

  return true;
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
  strncpy(rec.name, pet.getName(), sizeof(rec.name) - 1);
  rec.name[sizeof(rec.name) - 1] = '\0';
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
           "%s | GEN %u | Alder: %lu min | Doed: %s | Pong: %u | Labyrint: %u sek | \"%s\"\n",
           pet.getName(),
           (unsigned int)pet.getGeneration(),
           (unsigned long)pet.getAgeMinutes(),
           pet.getDeathCauseName(),
           (unsigned int)pet.getHighScorePong(),
           (unsigned int)pet.getHighScoreMaze(),
           pet.getDeathEpitaph());
  file.print(record);
  file.close();

  Serial.print(F("[STORAGE] Mindeskrift tilfoejet til grave.txt for "));
  Serial.print(pet.getName());
  Serial.print(F(" (GEN "));
  Serial.print((unsigned int)pet.getGeneration());
  Serial.println(F(")"));
  return true;
#else
  return true;
#endif
}

bool StoragePet::parseGraveLine(const char* line, GraveRecord& outRecord) {
  if (line == nullptr || line[0] == '\0') return false;

  outRecord.name[0] = '\0';
  const char* pSep = strstr(line, " | GEN ");
  if (pSep && (pSep - line) < (int)sizeof(outRecord.name)) {
    size_t nameLen = (size_t)(pSep - line);
    strncpy(outRecord.name, line, nameLen);
    outRecord.name[nameLen] = '\0';
  } else {
    strncpy(outRecord.name, "LUMI", sizeof(outRecord.name));
  }

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

