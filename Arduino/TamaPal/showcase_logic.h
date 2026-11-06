// WHAT: Pure logic controller for autonomous hands-free showcase presentation
// WHY:  Orchestrates ~70s full lifecycle theater (Name -> Egg -> Baby -> Poop -> Bath -> Teen -> Pong -> Sleep -> Old 100d -> Angel Death -> Memorial)
//       without requiring human touch inputs. Ideal for video recording and exhibitions.
// USED BY: TamaPal.ino, test_main.cpp

#ifndef SHOWCASE_LOGIC_H
#define SHOWCASE_LOGIC_H

#include <stdint.h>
#include "settings.h"
#include "pet_logic.h"
#include "menu_logic.h"
#include "name_logic.h"
#include "pong_logic.h"

enum ShowcasePhase {
  SHOWCASE_INACTIVE = 0,
  SHOWCASE_NAME_ENTRY,      // Act 1: 0.0s .. 2.5s (Arcade name entry "LUMI")
  SHOWCASE_EGG_HATCH,       // Act 2: 2.5s .. 7.0s (Wobble, crack, Baby hatch)
  SHOWCASE_BABY_FEED_POOP,  // Act 3: 7.0s .. 12.0s (Eat berry, digest, poop drops)
  SHOWCASE_BATH_CLEAN,      // Act 4: 12.0s .. 16.5s (Shaking bath, soap bubbles)
  SHOWCASE_GROW_TEEN_PET,   // Act 5: 16.5s .. 23.5s (Teen evolution, cuddle pet <3)
  SHOWCASE_SOLO_PONG,       // Act 6: 23.5s .. 35.0s (Auto-Pong, hits wall, scores berries)
  SHOWCASE_NIGHT_SLEEP,     // Act 7: 35.0s .. 45.0s (Starry night, wooden crib with quilt)
  SHOWCASE_OLD_MEDICINE,    // Act 8: 45.0s .. 54.0s (Elder Lumi 100d, beard, medicine)
  SHOWCASE_PEACEFUL_DEATH,  // Act 9: 54.0s .. 62.0s (Peaceful passing, angel wings & halo)
  SHOWCASE_MEMORIAL_STELE,  // Act 10: 62.0s .. 70.0s (Ancestral graveyard stele)
  SHOWCASE_COMPLETED        // Finished
};

struct ShowcaseAction {
  bool active;
  ShowcasePhase phase;
  SoundType soundToPlay;
  bool hasFlash;
  uint8_t flashR;
  uint8_t flashG;
  uint8_t flashB;
  uint16_t flashDurationMs;
  bool triggerBath;
  bool saveGrave;
  bool forceRedraw;
};

class ShowcaseDirector {
public:
  ShowcaseDirector();

  void start(unsigned long nowMs);
  void stop();
  bool isActive() const;
  ShowcasePhase getPhase() const;
  unsigned long getElapsedMs(unsigned long nowMs) const;

  ShowcaseAction update(unsigned long nowMs,
                        PetLogic& pet,
                        MenuLogic& menu,
                        NameEntryLogic& nameEntry,
                        PongLogic& pong,
                        uint16_t graveCount = 1);

private:
  bool active;
  unsigned long startTimestampMs;
  ShowcasePhase currentPhase;
  uint8_t subPhaseStep;
};

#endif // SHOWCASE_LOGIC_H

