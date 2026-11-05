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

private:
  bool cardInitialized;
};

#endif // STORAGE_PET_H

