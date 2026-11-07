// WHAT: MicroSD persistence manager for Lumi's Tamagotchi vitals
// WHY:  Ensures Lumi's state is preserved when USB power is unplugged
// USED BY: TamaPal.ino, tests/test_main.cpp

#ifndef STORAGE_PET_H
#define STORAGE_PET_H

#include <stddef.h>
#include <stdbool.h>
#include "pet_logic.h"

// MicroSD state filename (8.3 FAT filename format for SD library compatibility)
#define PET_STORAGE_FILENAME "lumi.txt"
#define PET_BACKUP_FILENAME  "lumi.bak"

class StoragePet {
public:
  StoragePet();

  // Hardware SD initialization (called in setup())
  bool begin();

  // Save current vitals to MicroSD card (called periodically & on actions)
  bool savePet(const PetLogic& pet);

  // Restore vitals from MicroSD card on boot (returns true if file existed & was valid)
  bool loadPet(PetLogic& pet);

  // Status query
  bool isCardReady() const;
  uint32_t getLastSavedHash() const;

  // Pure logic helpers for unit testing (zero hardware dependencies)
  static void serializeState(char* buffer, size_t bufSize, const PetLogic& pet);
  static bool deserializeState(const char* text,
                              float& hunger,
                              float& warmth,
                              float& energy,
                              float& happiness,
                              float& discipline,
                              bool& asleep);
  static bool deserializeState(const char* text,
                               float& hunger,
                               float& warmth,
                               float& energy,
                               float& happiness,
                               float& discipline,
                               bool& asleep,
                               PetStage& stage);
  static bool deserializeState(const char* text,
                               float& hunger,
                               float& warmth,
                               float& energy,
                               float& happiness,
                               float& discipline,
                               bool& asleep,
                               PetStage& stage,
                               PetOutfit& outfit);
  static bool deserializeState(const char* text,
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
                               uint32_t& age);
  static bool deserializeState(const char* text,
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
                               DeathCause& deathCause);
  static bool deserializeState(const char* text,
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
                               bool& poop);

  // Memorial ancestral log on MicroSD (appends to grave.txt)
  bool logGraveRecord(const PetLogic& pet);

  // Query records from grave.txt (or RAM fallback)
  uint16_t getGraveRecordCount();
  bool loadGraveRecord(uint16_t index, GraveRecord& outRecord);

  // Pure logic parser for a single line from grave.txt (unit testable without SD)
  static bool parseGraveLine(const char* line, GraveRecord& outRecord);

  // Extract pet name from serialized state (defaults to "LUMI" if missing)
  static void extractName(const char* text, char* nameOut, size_t maxLen);

  // FNV-1a non-cryptographic hash for dirty checking before SPI writes
  static uint32_t computeHash(const char* str);

private:
  uint32_t lastSavedHash;
  bool cardInitialized;
};

#endif // STORAGE_PET_H

