// WHAT: Pure logic controller for autonomous hands-free showcase presentation
// WHY:  Orchestrates ~70s full lifecycle theater without human touch inputs.
// USED BY: TamaPal.ino, test_main.cpp

#include "showcase_logic.h"

ShowcaseDirector::ShowcaseDirector()
  : active(false),
    startTimestampMs(0),
    currentPhase(SHOWCASE_INACTIVE),
    subPhaseStep(0) {}

void ShowcaseDirector::start(unsigned long nowMs) {
  active = true;
  startTimestampMs = nowMs;
  currentPhase = SHOWCASE_INACTIVE;
  subPhaseStep = 0;
}

void ShowcaseDirector::stop() {
  active = false;
  currentPhase = SHOWCASE_INACTIVE;
  subPhaseStep = 0;
}

bool ShowcaseDirector::isActive() const {
  return active;
}

ShowcasePhase ShowcaseDirector::getPhase() const {
  return currentPhase;
}

unsigned long ShowcaseDirector::getElapsedMs(unsigned long nowMs) const {
  if (!active || nowMs < startTimestampMs) return 0;
  return nowMs - startTimestampMs;
}

ShowcaseAction ShowcaseDirector::update(unsigned long nowMs,
                                        PetLogic& pet,
                                        MenuLogic& menu,
                                        NameEntryLogic& nameEntry,
                                        PongLogic& pong,
                                        uint16_t graveCount) {
  ShowcaseAction action;
  action.active = active;
  action.phase = currentPhase;
  action.soundToPlay = SOUND_NONE;
  action.hasFlash = false;
  action.flashR = 0;
  action.flashG = 0;
  action.flashB = 0;
  action.flashDurationMs = 0;
  action.triggerBath = false;
  action.saveGrave = false;
  action.forceRedraw = false;

  if (!active) {
    return action;
  }

  unsigned long elapsed = getElapsedMs(nowMs);

  // -------------------------------------------------------------
  // ACT 1: Arcade Name Entry (0 ms .. 2,500 ms)
  // -------------------------------------------------------------
  if (elapsed < 2500) {
    if (currentPhase != SHOWCASE_NAME_ENTRY) {
      currentPhase = SHOWCASE_NAME_ENTRY;
      subPhaseStep = 0;
      nameEntry.reset("LUMI");
      menu.setScreen(SCREEN_NAME_ENTRY, nowMs);
      action.soundToPlay = SOUND_CONFIRM;
      action.forceRedraw = true;
    } else if (subPhaseStep == 0 && elapsed >= 1200) {
      subPhaseStep = 1;
      nameEntry.confirm();
      action.soundToPlay = SOUND_CLICK;
    }
  }
  // -------------------------------------------------------------
  // ACT 2: Egg Hatching & Rebirth (2,500 ms .. 7,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 7000) {
    if (currentPhase != SHOWCASE_EGG_HATCH) {
      currentPhase = SHOWCASE_EGG_HATCH;
      subPhaseStep = 0;
      pet.setName(nameEntry.getName());
      pet.rebirth();
      pet.startHatching(nowMs);
      menu.setScreen(SCREEN_HATCHING, nowMs);
      action.soundToPlay = SOUND_REBIRTH;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 235; action.flashB = 90;
      action.flashDurationMs = 400;
      action.forceRedraw = true;
    } else if (subPhaseStep == 0 && elapsed >= 5350) {
      subPhaseStep = 1;
      pet.update(nowMs);
      menu.setScreen(SCREEN_HOME, nowMs);
      action.soundToPlay = SOUND_CONFIRM;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 3: Baby Care - Eating berry & Poop drops (7,000 ms .. 12,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 12000) {
    if (currentPhase != SHOWCASE_BABY_FEED_POOP) {
      currentPhase = SHOWCASE_BABY_FEED_POOP;
      subPhaseStep = 0;
      menu.setScreen(SCREEN_HOME, nowMs);
      pet.setVitals(35.0f, 85.0f, 90.0f, 75.0f, 50.0f);
      pet.feed();
      action.soundToPlay = SOUND_CONFIRM;
      action.hasFlash = true;
      action.flashR = 80; action.flashG = 255; action.flashB = 80;
      action.flashDurationMs = 300;
      action.forceRedraw = true;
    } else if (subPhaseStep == 0 && elapsed >= 9500) {
      subPhaseStep = 1;
      pet.setHasPoop(true, nowMs);
      action.soundToPlay = SOUND_CLICK;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 4: Bath & Soap Bubbles (12,000 ms .. 16,500 ms)
  // -------------------------------------------------------------
  else if (elapsed < 16500) {
    if (currentPhase != SHOWCASE_BATH_CLEAN) {
      currentPhase = SHOWCASE_BATH_CLEAN;
      subPhaseStep = 0;
      pet.cleanPoop();
      action.triggerBath = true;
      action.soundToPlay = SOUND_BUBBLES;
      action.hasFlash = true;
      action.flashR = 0; action.flashG = 220; action.flashB = 255;
      action.flashDurationMs = 600;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 5: Evolution to Teen & Cuddle Petting (16,500 ms .. 23,500 ms)
  // -------------------------------------------------------------
  else if (elapsed < 23500) {
    if (currentPhase != SHOWCASE_GROW_TEEN_PET) {
      currentPhase = SHOWCASE_GROW_TEEN_PET;
      subPhaseStep = 0;
      pet.setStage(STAGE_TEEN);
      pet.setAgeMinutes(15);
      action.soundToPlay = SOUND_CONFIRM;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 220; action.flashB = 60;
      action.flashDurationMs = 400;
      action.forceRedraw = true;
    } else if (subPhaseStep == 0 && elapsed >= 19000) {
      subPhaseStep = 1;
      pet.pet(nowMs);
      action.soundToPlay = SOUND_PURR;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 105; action.flashB = 180;
      action.flashDurationMs = 600;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 6: Solo Pong Minigame (23,500 ms .. 35,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 35000) {
    if (currentPhase != SHOWCASE_SOLO_PONG) {
      currentPhase = SHOWCASE_SOLO_PONG;
      subPhaseStep = 0;
      menu.openPongGame();
      pong.startNewGame(pet.getHighScorePong());
      pong.serveBall();
      action.soundToPlay = SOUND_CONFIRM;
      action.forceRedraw = true;
    } else {
      // Pong Autopilot: smoothly track ball
      if (elapsed < 33500) {
        float targetY = pong.getBallY();
        float currentY = pong.getPaddleY();
        float diff = targetY - currentY;
        pong.movePaddle(diff * 0.35f);
      } else if (subPhaseStep == 0) {
        // Ball slips past at 33.5s for clean game-over finish
        subPhaseStep = 1;
        if (pong.getScore() > pet.getHighScorePong()) {
          pet.setHighScorePong(pong.getScore());
        }
        pet.addBerries(3);
      }
    }
  }
  // -------------------------------------------------------------
  // ACT 7: Starry Night & Sleep in Crib under Quilt (35,000 ms .. 45,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 45000) {
    if (currentPhase != SHOWCASE_NIGHT_SLEEP) {
      currentPhase = SHOWCASE_NIGHT_SLEEP;
      subPhaseStep = 0;
      menu.setScreen(SCREEN_HOME, nowMs);
      pet.setSleeping(true);
      action.soundToPlay = SOUND_CLICK;
      action.hasFlash = true;
      action.flashR = 40; action.flashG = 70; action.flashB = 150;
      action.flashDurationMs = 500;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 8: Elder Lumi 100 Days & Herbal Medicine (45,000 ms .. 54,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 54000) {
    if (currentPhase != SHOWCASE_OLD_MEDICINE) {
      currentPhase = SHOWCASE_OLD_MEDICINE;
      subPhaseStep = 0;
      menu.setScreen(SCREEN_HOME, nowMs);
      pet.setSleeping(false);
      pet.setStage(STAGE_OLD);
      pet.setAgeMinutes(99);
      if (pet.getBerries() < 3) {
        pet.addBerries(3);
      }
      action.soundToPlay = SOUND_CONFIRM;
      action.forceRedraw = true;
    } else if (elapsed >= 49500) {
      if (subPhaseStep < 2) {
        subPhaseStep = 2;
        if (pet.getBerries() < 3) {
          pet.addBerries(3);
        }
        pet.cureSickness(3);
        action.soundToPlay = SOUND_HEAL;
        action.hasFlash = true;
        action.flashR = 80; action.flashG = 255; action.flashB = 120;
        action.flashDurationMs = 500;
        action.forceRedraw = true;
      }
    } else if (elapsed >= 47500) {
      if (subPhaseStep < 1) {
        subPhaseStep = 1;
        pet.setSick(true);
        action.soundToPlay = SOUND_MISS;
        action.forceRedraw = true;
      }
    }
  }
  // -------------------------------------------------------------
  // ACT 9: Peaceful Passing with Angel Wings (54,000 ms .. 62,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 62000) {
    if (currentPhase != SHOWCASE_PEACEFUL_DEATH) {
      currentPhase = SHOWCASE_PEACEFUL_DEATH;
      subPhaseStep = 0;
      pet.setAgeMinutes(100);
      pet.triggerDeath(DEATH_OLD_AGE);
      action.soundToPlay = SOUND_REBIRTH;
      action.hasFlash = true;
      action.flashR = 180; action.flashG = 220; action.flashB = 255;
      action.flashDurationMs = 800;
      action.saveGrave = true;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 10: Ancestral Memorial Graveyard Stele (62,000 ms .. 70,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 70000) {
    if (currentPhase != SHOWCASE_MEMORIAL_STELE) {
      currentPhase = SHOWCASE_MEMORIAL_STELE;
      subPhaseStep = 0;
      menu.openMemorial(graveCount > 0 ? graveCount : 1, SCREEN_HOME);
      action.soundToPlay = SOUND_CONFIRM;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // FINALE: Showcase Completed! (>= 70,000 ms)
  // -------------------------------------------------------------
  else {
    menu.onButtonBack();
    stop();
    action.active = false;
    currentPhase = SHOWCASE_COMPLETED;
    action.soundToPlay = SOUND_CONFIRM;
    action.forceRedraw = true;
  }

  action.phase = currentPhase;
  return action;
}
