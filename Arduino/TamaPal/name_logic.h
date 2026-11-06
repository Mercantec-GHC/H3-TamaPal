// WHAT: Pure arcade name entry logic for naming Lumi (up to 4 letters)
// WHY:  Separated from hardware and display for deterministic unit testing on Mac/PC
// USED BY: TamaPal.ino, ui_name.cpp, tests/test_main.cpp

#ifndef NAME_LOGIC_H
#define NAME_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

class NameEntryLogic {
public:
  static const uint8_t MAX_NAME_LENGTH = 4;

  NameEntryLogic();

  // Reset to a default name (defaults to "LUMI") and place cursor at slot 0
  void reset(const char* defaultName = "LUMI");

  // Cycle character at current cursor slot
  void nextLetter();
  void prevLetter();

  // Slot navigation (0..3)
  void nextSlot();
  void prevSlot();

  // Set explicit slot index (clamped to 0..3)
  void setCursorSlot(uint8_t slot);

  // Directly set char at slot
  void setChar(uint8_t slot, char c);

  // Finalize entry
  void confirm();

  // Getters
  uint8_t getCursorSlot() const;
  char getChar(uint8_t slot) const;
  bool isCompleted() const;

  // Returns null-terminated trimmed name (e.g. "LUMI", "MAX", "BOB")
  const char* getName() const;

private:
  char slots[MAX_NAME_LENGTH + 1];
  mutable char trimmedName[MAX_NAME_LENGTH + 1];
  uint8_t cursorSlot;
  bool completed;

  static char cycleNext(char c);
  static char cyclePrev(char c);
};

#endif // NAME_LOGIC_H
