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
  : cardInitialized(false) {
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

void StoragePet::serializeState(char* buffer, size_t bufSize, const PetLogic& pet) {
  if (buffer == nullptr || bufSize == 0) return;
  snprintf(buffer, bufSize,
           "HUNGER=%d\nWARMTH=%d\nENERGY=%d\nHAPPINESS=%d\nDISCIPLINE=%d\nASLEEP=%d\nSTAGE=%d\n",
           pet.getHunger(),
           pet.getWarmth(),
           pet.getEnergy(),
           pet.getHappiness(),
           pet.getDiscipline(),
           pet.isSleeping() ? 1 : 0,
           (int)pet.getStageEnum());
}

bool StoragePet::deserializeState(const char* text,
                                 float& hunger,
                                 float& warmth,
                                 float& energy,
                                 float& happiness,
                                 float& discipline,
                                 bool& asleep) {
  PetStage unusedStage = STAGE_BABY;
  return deserializeState(text, hunger, warmth, energy, happiness, discipline, asleep, unusedStage);
}

bool StoragePet::deserializeState(const char* text,
                                 float& hunger,
                                 float& warmth,
                                 float& energy,
                                 float& happiness,
                                 float& discipline,
                                 bool& asleep,
                                 PetStage& stage) {
  if (text == nullptr || text[0] == '\0') return false;

  int h = -1, w = -1, e = -1, hap = -1, d = -1, s = -1;
  int st = (int)STAGE_BABY; // Default to baby if not present in legacy file
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
    }

    const char* nextLine = strchr(p, '\n');
    if (nextLine == nullptr) break;
    p = nextLine + 1;
  }

  // Validate that all 6 required fields are within valid bounds (0..100)
  if (h < 0 || h > 100 ||
      w < 0 || w > 100 ||
      e < 0 || e > 100 ||
      hap < 0 || hap > 100 ||
      d < 0 || d > 100 ||
      s < 0 || s > 1 ||
      st < 0 || st > 3) {
    return false;
  }

  hunger = (float)h;
  warmth = (float)w;
  energy = (float)e;
  happiness = (float)hap;
  discipline = (float)d;
  asleep = (s == 1);
  stage = (PetStage)st;
  return true;
}

bool StoragePet::savePet(const PetLogic& pet) {
#if defined(ARDUINO)
  if (!cardInitialized) return false;

  char buffer[128];
  serializeState(buffer, sizeof(buffer), pet);

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

  char buffer[128];
  int bytesRead = 0;
  while (file.available() && bytesRead < (int)(sizeof(buffer) - 1)) {
    buffer[bytesRead++] = (char)file.read();
  }
  buffer[bytesRead] = '\0';
  file.close();

  float h = 0, w = 0, e = 0, hap = 0, d = 0;
  bool asleep = false;
  PetStage stage = STAGE_BABY;
  if (deserializeState(buffer, h, w, e, hap, d, asleep, stage)) {
    pet.setVitals(h, w, e, hap, d);
    pet.setSleeping(asleep);
    pet.setStage(stage);
    Serial.print(F("[STORAGE] SUCCES: Indlaeste Lumi fra MicroSD! Sult: "));
    Serial.print((int)h);
    Serial.print(F("%, Energi: "));
    Serial.print((int)e);
    Serial.print(F("%, Stadie: "));
    Serial.println(pet.getStage());
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

