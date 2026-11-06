#include "name_logic.h"

NameEntryLogic::NameEntryLogic() {
  reset("LUMI");
}

void NameEntryLogic::reset(const char* defaultName) {
  cursorSlot = 0;
  completed = false;

  for (uint8_t i = 0; i < MAX_NAME_LENGTH; ++i) {
    slots[i] = ' ';
  }
  slots[MAX_NAME_LENGTH] = '\0';
  trimmedName[0] = '\0';

  if (defaultName != nullptr) {
    size_t len = strlen(defaultName);
    for (uint8_t i = 0; i < MAX_NAME_LENGTH && i < len; ++i) {
      char c = defaultName[i];
      if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
      slots[i] = c;
    }
  }
}

char NameEntryLogic::cycleNext(char c) {
  if (c >= 'A' && c < 'Z') return c + 1;
  if (c == 'Z') return '0';
  if (c >= '0' && c < '9') return c + 1;
  if (c == '9') return ' ';
  if (c == ' ') return 'A';
  return 'A';
}

char NameEntryLogic::cyclePrev(char c) {
  if (c > 'A' && c <= 'Z') return c - 1;
  if (c == 'A') return ' ';
  if (c == ' ') return '9';
  if (c > '0' && c <= '9') return c - 1;
  if (c == '0') return 'Z';
  return 'A';
}

void NameEntryLogic::nextLetter() {
  if (completed) return;
  slots[cursorSlot] = cycleNext(slots[cursorSlot]);
}

void NameEntryLogic::prevLetter() {
  if (completed) return;
  slots[cursorSlot] = cyclePrev(slots[cursorSlot]);
}

void NameEntryLogic::nextSlot() {
  if (completed) return;
  if (cursorSlot < MAX_NAME_LENGTH - 1) {
    cursorSlot++;
  } else {
    confirm();
  }
}

void NameEntryLogic::prevSlot() {
  if (completed) return;
  if (cursorSlot > 0) {
    cursorSlot--;
  }
}

void NameEntryLogic::setCursorSlot(uint8_t slot) {
  if (slot >= MAX_NAME_LENGTH) slot = MAX_NAME_LENGTH - 1;
  cursorSlot = slot;
}

void NameEntryLogic::setChar(uint8_t slot, char c) {
  if (slot < MAX_NAME_LENGTH) {
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    slots[slot] = c;
  }
}

void NameEntryLogic::confirm() {
  completed = true;
}

uint8_t NameEntryLogic::getCursorSlot() const {
  return cursorSlot;
}

char NameEntryLogic::getChar(uint8_t slot) const {
  if (slot < MAX_NAME_LENGTH) {
    return slots[slot];
  }
  return ' ';
}

bool NameEntryLogic::isCompleted() const {
  return completed;
}

const char* NameEntryLogic::getName() const {
  // Trim trailing whitespace
  int lastNonSpace = -1;
  for (int i = (int)MAX_NAME_LENGTH - 1; i >= 0; --i) {
    if (slots[i] != ' ' && slots[i] != '\0') {
      lastNonSpace = i;
      break;
    }
  }

  if (lastNonSpace < 0) {
    // If empty or all spaces, default to "LUMI"
    strncpy(trimmedName, "LUMI", sizeof(trimmedName));
  } else {
    int i = 0;
    for (; i <= lastNonSpace; ++i) {
      trimmedName[i] = slots[i];
    }
    trimmedName[i] = '\0';
  }

  return trimmedName;
}
