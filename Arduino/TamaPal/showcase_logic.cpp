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
  action.triggerShake = false;
  action.triggerNova = false;
  action.triggerCelestialAura = false;
  action.triggerChaser = false;
  action.triggerBreathing = false;
  action.triggerMorph = false;
  action.triggerIrisWipe = false;
  action.actNumber = 0;
  action.actTitle = "";
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
    action.actNumber = 1;
    action.actTitle = "ARKADE NAVNGIVNING";
    if (currentPhase != SHOWCASE_NAME_ENTRY) {
      currentPhase = SHOWCASE_NAME_ENTRY;
      subPhaseStep = 0;
      nameEntry.reset("LUMI");
      menu.setScreen(SCREEN_NAME_ENTRY, nowMs);
      action.soundToPlay = SOUND_CONFIRM;
      action.triggerIrisWipe = true;
      action.forceRedraw = true;
    } else if (subPhaseStep == 0 && elapsed >= 1200) {
      subPhaseStep = 1;
      nameEntry.confirm();
      action.soundToPlay = SOUND_CLICK;
    }
  }
  // -------------------------------------------------------------
  // ACT 2: Egg Hatching & Heartbeat Suspense (2,500 ms .. 7,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 7000) {
    action.actNumber = 2;
    action.actTitle = (subPhaseStep >= 3) ? "BABY LUMI FOEDT!" : "DET MAGISKE AEG";
    if (currentPhase != SHOWCASE_EGG_HATCH) {
      currentPhase = SHOWCASE_EGG_HATCH;
      subPhaseStep = 0;
      pet.setName(nameEntry.getName());
      pet.startHatching(nowMs);
      menu.setScreen(SCREEN_HATCHING, nowMs);
      action.soundToPlay = SOUND_REBIRTH;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 235; action.flashB = 90;
      action.flashDurationMs = 400;
      action.triggerIrisWipe = true;
      action.forceRedraw = true;
    } else if (subPhaseStep == 0 && elapsed >= 3800) {
      // Heartbeat 1 (deep organic thump + ruby LED pulse)
      subPhaseStep = 1;
      action.soundToPlay = SOUND_HEARTBEAT;
      action.hasFlash = true;
      action.flashR = 220; action.flashG = 15; action.flashB = 25;
      action.flashDurationMs = 150;
    } else if (subPhaseStep == 1 && elapsed >= 4600) {
      // Heartbeat 2 (urgent thump before cracking)
      subPhaseStep = 2;
      action.soundToPlay = SOUND_HEARTBEAT;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 20; action.flashB = 30;
      action.flashDurationMs = 150;
    } else if (subPhaseStep <= 2 && elapsed >= 5350) {
      // CRACK & NOVA! Egg cracks with impact screen shake, golden starburst & morph!
      subPhaseStep = 3;
      pet.update(nowMs);
      menu.setScreen(SCREEN_HOME, nowMs);
      action.actTitle = "BABY LUMI FOEDT!";
      action.soundToPlay = SOUND_REBIRTH;
      action.triggerShake = true;
      action.triggerNova = true;
      action.triggerMorph = true;
      action.triggerChaser = true;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 220; action.flashB = 60;
      action.flashDurationMs = 600;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 3: Baby Care - Eating berry & Poop drops (7,000 ms .. 12,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 12000) {
    action.actNumber = 3;
    action.actTitle = (subPhaseStep == 0) ? "BABY SPISER BAER" : "HOVSA! RENGOR MIG";
    if (currentPhase != SHOWCASE_BABY_FEED_POOP) {
      currentPhase = SHOWCASE_BABY_FEED_POOP;
      subPhaseStep = 0;
      pet.setVitals(35.0f, 85.0f, 90.0f, 75.0f, 50.0f);
      pet.feed();
      menu.setScreen(SCREEN_ACTION, nowMs);
      action.soundToPlay = SOUND_CONFIRM;
      action.hasFlash = true;
      action.flashR = 80; action.flashG = 255; action.flashB = 80;
      action.flashDurationMs = 300;
      action.forceRedraw = true;
    } else if (subPhaseStep == 0 && elapsed < 9500) {
      // Keep SCREEN_ACTION alive so MenuLogic::update doesn't auto-close it at 8,500 ms
      menu.setScreen(SCREEN_ACTION, nowMs);
    } else if (subPhaseStep == 0 && elapsed >= 9500) {
      subPhaseStep = 1;
      action.actTitle = "HOVSA! RENGOR MIG";
      menu.setScreen(SCREEN_HOME, nowMs);
      pet.setHasPoop(true, nowMs);
      action.soundToPlay = SOUND_CLICK;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 4: Bath & Soap Bubbles (12,000 ms .. 16,500 ms)
  // -------------------------------------------------------------
  else if (elapsed < 16500) {
    action.actNumber = 4;
    action.actTitle = "SAEBESKUM-BAD!";
    if (currentPhase != SHOWCASE_BATH_CLEAN) {
      currentPhase = SHOWCASE_BATH_CLEAN;
      subPhaseStep = 0;
      pet.cleanPoop();
      action.triggerBath = true;
      action.triggerShake = true; // Hydro-bubble pop impact shake!
      action.soundToPlay = SOUND_BUBBLES;
      action.hasFlash = true;
      action.flashR = 0; action.flashG = 220; action.flashB = 255;
      action.flashDurationMs = 600;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 5: Evolution to Teen, Cuddle Petting & Adult Stage (16,500 ms .. 23,500 ms)
  // -------------------------------------------------------------
  else if (elapsed < 23500) {
    action.actNumber = 5;
    action.actTitle = (subPhaseStep >= 2) ? "VOKSEN DIPLOMAT!" : (subPhaseStep == 1 ? "AELER & SPINDER <3" : "TEENAGER EVOLUTION");
    if (currentPhase != SHOWCASE_GROW_TEEN_PET) {
      currentPhase = SHOWCASE_GROW_TEEN_PET;
      subPhaseStep = 0;
      pet.setStage(STAGE_TEEN);
      pet.setAgeMinutes(15);
      action.soundToPlay = SOUND_REBIRTH;
      action.triggerNova = true;     // Evolution Nova starburst!
      action.triggerMorph = true;    // Smooth stage evolution morph!
      action.triggerChaser = true;   // Circular LED spinning portal!
      action.hasFlash = true;
      action.flashR = 0; action.flashG = 255; action.flashB = 200;
      action.flashDurationMs = 500;
      action.forceRedraw = true;
    } else if (subPhaseStep == 0 && elapsed >= 19000) {
      subPhaseStep = 1;
      action.actTitle = "AELER & SPINDER <3";
      pet.pet(nowMs);
      action.soundToPlay = SOUND_PURR;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 105; action.flashB = 180;
      action.flashDurationMs = 600;
      action.forceRedraw = true;
    } else if (subPhaseStep == 1 && elapsed >= 21200) {
      // Smooth evolution to Adult diplomat stage (tie, briefcase & lotus crown)
      subPhaseStep = 2;
      action.actTitle = "VOKSEN DIPLOMAT!";
      pet.setStage(STAGE_ADULT);
      pet.setAgeMinutes(40);
      action.soundToPlay = SOUND_REBIRTH;
      action.triggerMorph = true;
      action.triggerNova = true;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 215; action.flashB = 60;
      action.flashDurationMs = 450;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 6: Solo Pong Minigame (23,500 ms .. 35,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 35000) {
    action.actNumber = 6;
    action.actTitle = "SOLO MUR-PONG";
    if (currentPhase != SHOWCASE_SOLO_PONG) {
      currentPhase = SHOWCASE_SOLO_PONG;
      subPhaseStep = 0;
      menu.openPongGame();
      pong.startNewGame(pet.getHighScorePong());
      pong.serveBall();
      action.soundToPlay = SOUND_CONFIRM;
      action.triggerIrisWipe = true;
      action.forceRedraw = true;
    } else {
      // Pong Autopilot: smoothly track ball center
      if (elapsed < 33500) {
        float paddleCenterY = pong.getPaddleY() + 16.0f;
        float ballCenterY = pong.getBallY() + 3.0f;
        float diff = ballCenterY - paddleCenterY;
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
    action.actNumber = 7;
    action.actTitle = "STJERNENAT & SOEVN";
    if (currentPhase != SHOWCASE_NIGHT_SLEEP) {
      currentPhase = SHOWCASE_NIGHT_SLEEP;
      subPhaseStep = 0;
      menu.setScreen(SCREEN_HOME, nowMs);
      pet.setSleeping(true);
      action.soundToPlay = SOUND_CLICK;
      action.triggerIrisWipe = true;
      action.triggerBreathing = true; // Soft LED nocturnal breathing!
      action.hasFlash = true;
      action.flashR = 40; action.flashG = 70; action.flashB = 160;
      action.flashDurationMs = 600;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 8: Elder Lumi 100 Days & Herbal Medicine (45,000 ms .. 54,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 54000) {
    action.actNumber = 8;
    action.actTitle = (subPhaseStep >= 2) ? "HELBREDT MED BAER!" : (subPhaseStep == 1 ? "AK! LUMI HAR FEBER" : "GAMLE LUMI (99 DAGE)");
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
      action.triggerMorph = true;
      action.triggerNova = true;
      action.forceRedraw = true;
    } else if (elapsed >= 52000) {
      if (subPhaseStep < 3) {
        subPhaseStep = 3;
        menu.setScreen(SCREEN_HOME, nowMs);
        action.forceRedraw = true;
      }
    } else if (elapsed >= 49500) {
      if (subPhaseStep < 2) {
        subPhaseStep = 2;
        action.actTitle = "HELBREDT MED BAER!";
        if (pet.getBerries() < 3) {
          pet.addBerries(3);
        }
        pet.cureSickness(3);
        menu.setScreen(SCREEN_ACTION, nowMs);
        action.soundToPlay = SOUND_HEAL;
        action.hasFlash = true;
        action.flashR = 80; action.flashG = 255; action.flashB = 120;
        action.flashDurationMs = 500;
        action.forceRedraw = true;
      } else {
        // Keep SCREEN_ACTION alive until 52,000 ms so MenuLogic::update doesn't close it early
        menu.setScreen(SCREEN_ACTION, nowMs);
      }
    } else if (elapsed >= 47500) {
      if (subPhaseStep < 1) {
        subPhaseStep = 1;
        action.actTitle = "AK! LUMI HAR FEBER";
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
    action.actNumber = 9;
    action.actTitle = "HIMMELFAERD (100 DAGE)";
    if (currentPhase != SHOWCASE_PEACEFUL_DEATH) {
      currentPhase = SHOWCASE_PEACEFUL_DEATH;
      subPhaseStep = 0;
      pet.setAgeMinutes(100);
      pet.triggerDeath(DEATH_OLD_AGE);
      action.triggerCelestialAura = true; // Celestial halo & ascension spirit particles!
      action.triggerMorph = true;         // Smooth ascension morph!
      action.triggerNova = true;
      action.soundToPlay = SOUND_REBIRTH;
      action.hasFlash = true;
      action.flashR = 160; action.flashG = 220; action.flashB = 255;
      action.flashDurationMs = 800;
      action.saveGrave = true;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 10: Ancestral Memorial Graveyard Stele (62,000 ms .. 68,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 68000) {
    action.actNumber = 10;
    action.actTitle = "MINDELUND & GRAVSTEN";
    if (currentPhase != SHOWCASE_MEMORIAL_STELE) {
      currentPhase = SHOWCASE_MEMORIAL_STELE;
      subPhaseStep = 0;
      menu.openMemorial(graveCount > 0 ? graveCount : 1, SCREEN_HOME);
      action.soundToPlay = SOUND_CONFIRM;
      action.triggerIrisWipe = true;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // ACT 11: Grand Finale Title Splash Screen (68,000 ms .. 72,000 ms)
  // -------------------------------------------------------------
  else if (elapsed < 72000) {
    action.actNumber = 11;
    action.actTitle = "TAMAPAL GRAND FINALE";
    if (currentPhase != SHOWCASE_TITLE_SPLASH) {
      currentPhase = SHOWCASE_TITLE_SPLASH;
      subPhaseStep = 0;
      action.soundToPlay = SOUND_FANFARE;
      action.triggerIrisWipe = true;
      action.triggerChaser = true;
      action.hasFlash = true;
      action.flashR = 255; action.flashG = 215; action.flashB = 0;
      action.flashDurationMs = 800;
      action.forceRedraw = true;
    }
  }
  // -------------------------------------------------------------
  // FINALE: Showcase Completed! (>= 72,000 ms)
  // -------------------------------------------------------------
  else {
    menu.setScreen(SCREEN_HOME, nowMs);
    stop();
    action.active = false;
    currentPhase = SHOWCASE_COMPLETED;
    action.soundToPlay = SOUND_CONFIRM;
    action.triggerIrisWipe = true;
    action.forceRedraw = true;
  }

  action.phase = currentPhase;
  return action;
}
