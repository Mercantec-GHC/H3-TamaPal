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
  SHOWCASE_EGG_HATCH,       // Act 2: 2.5s .. 7.0s (Heartbeat suspense, crack, Nova explosion)
  SHOWCASE_BABY_FEED_POOP,  // Act 3: 7.0s .. 12.0s (Eat berry, digest, poop drops)
  SHOWCASE_BATH_CLEAN,      // Act 4: 12.0s .. 16.5s (Shaking bath, bubble pop shake)
  SHOWCASE_GROW_TEEN_PET,   // Act 5: 16.5s .. 23.5s (Teen evolution, Nova burst, cuddle pet)
  SHOWCASE_SOLO_PONG,       // Act 6: 23.5s .. 35.0s (Auto-Pong, hits wall, scores berries)
  SHOWCASE_NIGHT_SLEEP,     // Act 7: 35.0s .. 45.0s (Starry night, crib, LED breathing)
  SHOWCASE_OLD_MEDICINE,    // Act 8: 45.0s .. 54.0s (Elder Lumi 100d, beard, medicine)
  SHOWCASE_PEACEFUL_DEATH,  // Act 9: 54.0s .. 62.0s (Celestial aura, golden halo, ascension glow)
  SHOWCASE_MEMORIAL_STELE,  // Act 10: 62.0s .. 68.0s (Ancestral graveyard stele)
  SHOWCASE_TITLE_SPLASH,    // Act 11: 68.0s .. 72.0s (Grand Finale: TamaPal Mercantec title splash!)
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
  bool triggerShake;        // Impact camera shake!
  bool triggerNova;         // Burst of golden stars / explosion!
  bool triggerCelestialAura;// Angelic rotating halo & spirit motes!
  bool triggerChaser;       // Circular running LED chaser!
  bool triggerBreathing;    // Soft pulsating LED breathing!
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

