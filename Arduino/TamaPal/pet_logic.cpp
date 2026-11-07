// WHAT: Pure business logic implementation for Lumi's 5 Tamagotchi vitals
// WHY:  Zero Arduino/hardware dependencies - pure portable C++ with 100% test coverage
// USED BY: TamaPal.ino, ui_stats.cpp, tests/test_main.cpp

#include "pet_logic.h"
#include <stdio.h>
#include <string.h>

PetLogic::PetLogic() {
  reset();
}

float PetLogic::clamp0100(float val) {
  if (val < 0.0f) return 0.0f;
  if (val > 100.0f) return 100.0f;
  return val;
}

void PetLogic::reset() {
  hungerVal = 80.0f;      // Well-fed
  warmthVal = 85.0f;      // Comfortable
  energyVal = 90.0f;      // Well-rested
  happinessVal = 75.0f;   // Cheerful
  disciplineVal = 50.0f;  // Balanced

  currentStage = STAGE_BABY;     // Starts as cute egg baby
  currentOutfit = OUTFIT_DEFAULT; // Natural fur mode
  isAsleep = false;
  alive = true;
  deathCause = DEATH_NONE;
  generation = 1;
  strncpy(petName, "Lumi", sizeof(petName));
  petName[sizeof(petName) - 1] = '\0';
  berryCount = 0;
  sick = false;
  sickTicks = 0;
  hasPoop = false;
  poopTimeMs = 0;
  hatchingActive = false;
  hatchingStartTimeMs = 0;
  pettingActiveUntilMs = 0;
  lastPetTimeMs = 0;
  criticalTicks = 0;
  decayInitialized = false;
  lastDecayTimeMs = 0;
  darkStartTimeMs = 0;
  lightStartTimeMs = 0;
  ambientTemp = 22.0f;
  ambientLux = 150;

  ageMinutes = 0;
  highScorePong = 0;
  highScoreMaze = 0;
  ageTickCount = 0;
}

void PetLogic::setVitals(float hunger, float warmth, float energy, float happiness, float discipline) {
  hungerVal = clamp0100(hunger);
  warmthVal = clamp0100(warmth);
  energyVal = clamp0100(energy);
  happinessVal = clamp0100(happiness);
  disciplineVal = clamp0100(discipline);
  if (getOverallHealth() == 0) {
    alive = false;
    if (deathCause == DEATH_NONE) {
      deathCause = DEATH_HUNGER;
    }
  }
}

bool PetLogic::feed() {
  if (!alive || isAsleep) return false;     // Cannot feed while dead or sleeping
  if (hungerVal >= 100.0f) return false; // Already completely full

  hungerVal = clamp0100(hungerVal + PET_FEED_RESTORE);
  happinessVal = clamp0100(happinessVal + 5.0f);
  hasPoop = true;
  poopTimeMs = 0;
  return true;
}

bool PetLogic::play() {
  if (!alive || isAsleep) return false;    // Cannot play while dead or sleeping
  if (energyVal < 15.0f) return false; // Too exhausted to play

  happinessVal = clamp0100(happinessVal + PET_PLAY_RESTORE);
  energyVal = clamp0100(energyVal - 15.0f);
  hungerVal = clamp0100(hungerVal - 5.0f);
  return true;
}

bool PetLogic::scold() {
  if (!alive || isAsleep) return false; // Cannot scold while dead or sleeping

  disciplineVal = clamp0100(disciplineVal + PET_SCOLD_RESTORE);
  happinessVal = clamp0100(happinessVal - 10.0f);
  return true;
}

bool PetLogic::applyReward(const char* rewardType, int amount) {
  if (rewardType == nullptr || amount <= 0) return false;

  if (strcmp(rewardType, "happy_boost") == 0) {
    happinessVal = clamp0100(happinessVal + (float)amount);
    return true;
  }
  if (strcmp(rewardType, "energy_boost") == 0) {
    energyVal = clamp0100(energyVal + (float)amount);
    return true;
  }
  return false;
}

void PetLogic::applyGameReward(uint16_t score) {
  float happyGain = (score >= 10) ? 30.0f : (float)(score * 3);
  happinessVal = clamp0100(happinessVal + happyGain);
  if (energyVal >= 8.0f) {
    energyVal -= 8.0f;
  } else {
    energyVal = 0.0f;
  }
}

void PetLogic::toggleSleep() {
  setSleeping(!isAsleep);
}

void PetLogic::setSleeping(bool sleep) {
  isAsleep = sleep;
  darkStartTimeMs = 0;
  lightStartTimeMs = 0;
}

void PetLogic::setAmbientTemperature(float celsius) {
  // Reject NaN or invalid sensor spikes from uninitialized hardware
  if (celsius < -20.0f || celsius > 65.0f || celsius != celsius) {
    return;
  }

  ambientTemp = celsius;

  float targetWarmth;
  if (currentOutfit == OUTFIT_WINTER) {
    // Winter clothes insulate Lumi against cold rooms!
    if (celsius < 20.0f) {
      // Keeps him warm and cozy (85%) even in the cold
      targetWarmth = 85.0f;
    } else {
      // In a warm room (>20C), winter clothes cause heat buildup
      float excess = celsius - 20.0f;
      targetWarmth = 85.0f + (excess * 4.0f);
    }
  } else {
    // Natural mochi fur: ideal temperature is around 22.0 deg Celsius
    float diff = (celsius >= 22.0f) ? (celsius - 22.0f) : (22.0f - celsius);
    targetWarmth = 100.0f - (diff * 7.0f);
  }
  targetWarmth = clamp0100(targetWarmth);

  // Smooth adjustment towards the environmental target (EMA filter)
  warmthVal = (warmthVal * 0.80f) + (targetWarmth * 0.20f);
  warmthVal = clamp0100(warmthVal);
}

void PetLogic::setAmbientLight(int lux, unsigned long nowMs) {
  if (lux < 0) lux = 0;
  ambientLux = lux;

  // If nowMs is not supplied (0), use immediate trigger for legacy compatibility
  if (nowMs == 0) {
    if (ambientLux < SENSOR_DARK_LUX_THRESHOLD && !isAsleep) {
      isAsleep = true;
    } else if (ambientLux >= SENSOR_LIGHT_LUX_THRESHOLD && isAsleep) {
      isAsleep = false;
    }
    return;
  }

  // Dark state timing (hand covering sensor for SENSOR_DARK_SLEEP_DELAY_MS = 5000 ms)
  if (ambientLux < SENSOR_DARK_LUX_THRESHOLD) {
    lightStartTimeMs = 0;
    if (darkStartTimeMs == 0) {
      darkStartTimeMs = nowMs;
    } else if (!isAsleep && (nowMs - darkStartTimeMs >= SENSOR_DARK_SLEEP_DELAY_MS)) {
      isAsleep = true;
    }
  }
  // Light state timing (uncovered room light for SENSOR_LIGHT_WAKE_DELAY_MS = 2000 ms)
  else if (ambientLux >= SENSOR_LIGHT_LUX_THRESHOLD) {
    darkStartTimeMs = 0;
    if (lightStartTimeMs == 0) {
      lightStartTimeMs = nowMs;
    } else if (isAsleep && (nowMs - lightStartTimeMs >= SENSOR_LIGHT_WAKE_DELAY_MS)) {
      isAsleep = false;
    }
  } else {
    // Intermediate lighting resets both delay counters
    darkStartTimeMs = 0;
    lightStartTimeMs = 0;
  }
}

void PetLogic::update(unsigned long currentTimeMs) {
  if (hatchingActive) {
    if (currentTimeMs - hatchingStartTimeMs >= 2800) {
      hatchingActive = false;
      rebirth();
    }
    return; // Pause vitals decay and death checks during hatching cinematic
  }

  if (!alive) return; // Simulation stops if Lumi is deceased

  // If pet loaded with poop from SD, initialize poop timer on first active frame
  if (hasPoop && poopTimeMs == 0) {
    poopTimeMs = currentTimeMs;
  }

  if (!decayInitialized) {
    lastDecayTimeMs = currentTimeMs;
    decayInitialized = true;
    return;
  }

  if (currentTimeMs - lastDecayTimeMs >= PET_DECAY_INTERVAL_MS) {
    unsigned long elapsedIntervals = (currentTimeMs - lastDecayTimeMs) / PET_DECAY_INTERVAL_MS;
    lastDecayTimeMs += elapsedIntervals * PET_DECAY_INTERVAL_MS;

    float intervals = (float)elapsedIntervals;
    unsigned long totalTicks = (unsigned long)ageTickCount + elapsedIntervals;
    ageMinutes += (totalTicks / 6);
    ageTickCount = (uint8_t)(totalTicks % 6);

    if (isAsleep) {
      // Recover energy while sleeping
      energyVal = clamp0100(energyVal + (PET_SLEEP_ENERGY_RECOVERY * intervals));
      // Hunger decays slower while asleep (half rate)
      hungerVal = clamp0100(hungerVal - (PET_DECAY_HUNGER * 0.5f * intervals));
    } else {
      // Normal daytime awake decay (O(1) calculation, zero lag even after long pauses)
      energyVal = clamp0100(energyVal - (PET_DECAY_ENERGY * intervals));
      hungerVal = clamp0100(hungerVal - (PET_DECAY_HUNGER * intervals));
      happinessVal = clamp0100(happinessVal - (PET_DECAY_HAPPINESS * intervals));
      disciplineVal = clamp0100(disciplineVal - (PET_DECAY_DISCIPLINE * intervals));

      // Overheating discomfort: if wearing heavy winter clothes in a warm room
      if (isOverheating()) {
        happinessVal = clamp0100(happinessVal - (1.5f * intervals));
      }
    }

    // Poop neglect: if poop stays on screen >= 60 seconds, Lumi catches an infection
    if (hasPoop) {
      if (poopTimeMs == 0) {
        poopTimeMs = currentTimeMs;
      } else if (currentTimeMs - poopTimeMs >= 60000) {
        sick = true;
      }
    }

    // Critical vitals infection: prolonged hunger, cold or exhaustion triggers sickness
    if (needsAttention()) {
      if (criticalTicks >= 4) {
        sick = true;
      }
    }

    // Untreated sickness progression:
    if (sick) {
      sickTicks += (uint8_t)elapsedIntervals;
      happinessVal = clamp0100(happinessVal - (1.0f * intervals));
      if (sickTicks >= 12) { // ~2 minutes untreated -> death from illness / exhaustion
        triggerDeath(DEATH_EXHAUSTION);
        return;
      }
    }

    // Check critical death conditions (after sustained neglected status)
    if (hungerVal <= 0.0f) {
      criticalTicks += (uint8_t)elapsedIntervals;
      if (criticalTicks >= 12) { // ~2 minutes of absolute 0% hunger
        triggerDeath(DEATH_HUNGER);
        return;
      }
    } else if (warmthVal <= 0.0f) {
      criticalTicks += (uint8_t)elapsedIntervals;
      if (criticalTicks >= 12) { // ~2 minutes of absolute 0% warmth in freezing cold
        triggerDeath(DEATH_COLD);
        return;
      }
    } else if (isOverheating() && warmthVal >= 90.0f && happinessVal <= 0.0f) {
      criticalTicks += (uint8_t)elapsedIntervals;
      if (criticalTicks >= 12) { // ~2 minutes of severe heat exhaustion
        triggerDeath(DEATH_HEAT);
        return;
      }
    } else if (happinessVal <= 0.0f && energyVal < 20.0f) {
      criticalTicks += (uint8_t)elapsedIntervals;
      if (criticalTicks >= 18) { // ~3 minutes of absolute boredom / depression
        triggerDeath(DEATH_BOREDOM);
        return;
      }
    } else if (energyVal <= 0.0f && hungerVal < 20.0f) {
      criticalTicks += (uint8_t)elapsedIntervals;
      if (criticalTicks >= 12) { // ~2 minutes of complete exhaustion
        triggerDeath(DEATH_EXHAUSTION);
        return;
      }
    } else if (currentStage == STAGE_OLD && ageMinutes >= 100) {
      triggerDeath(DEATH_OLD_AGE); // Peaceful old age death
      return;
    } else {
      criticalTicks = 0;
    }
  }
}

int PetLogic::getHunger() const {
  return (int)(hungerVal + 0.5f);
}

int PetLogic::getWarmth() const {
  return (int)(warmthVal + 0.5f);
}

int PetLogic::getEnergy() const {
  return (int)(energyVal + 0.5f);
}

int PetLogic::getHappiness() const {
  return (int)(happinessVal + 0.5f);
}

int PetLogic::getDiscipline() const {
  return (int)(disciplineVal + 0.5f);
}

int PetLogic::getOverallHealth() const {
  int total = getHunger() + getWarmth() + getEnergy() + getHappiness() + getDiscipline();
  return total / 5;
}

bool PetLogic::isSleeping() const {
  return isAsleep;
}

bool PetLogic::needsAttention() const {
  return (getHunger() < PET_CRITICAL_THRESHOLD) ||
         (getWarmth() < PET_CRITICAL_THRESHOLD) ||
         (getEnergy() < PET_CRITICAL_THRESHOLD) ||
         (getHappiness() < PET_CRITICAL_THRESHOLD);
}

const char* PetLogic::getMoodStatusDanish() const {
  if (isAsleep) {
    return "Sover zZz...";
  }
  if (sick) {
    return "Lumi er syg! Giv medicin!";
  }
  if (hasPoop) {
    return "Poelse paa skaermen! Ryst!";
  }
  if (isOverheating()) {
    return "Lumi sveder! Tag huen af!";
  }
  if (getWarmth() < 45) {
    return "Lumi fryser! Giv hue paa!";
  }
  if (getHunger() < PET_CRITICAL_THRESHOLD) {
    return "Lumi er sulten!";
  }
  if (getEnergy() < PET_CRITICAL_THRESHOLD) {
    return "Lumi er udmattet!";
  }
  if (getHappiness() < PET_CRITICAL_THRESHOLD) {
    return "Lumi er trist...";
  }
  if (getOverallHealth() >= 80) {
    return "Lumi er super glad!";
  }
  if (getOverallHealth() >= 50) {
    return "Lumi har det godt";
  }
  return "Kraever lidt omsorg";
}

bool PetLogic::isAlive() const {
  return alive && (getOverallHealth() > 0);
}

DeathCause PetLogic::getDeathCause() const {
  if (deathCause == DEATH_NONE && getOverallHealth() == 0) {
    return DEATH_HUNGER;
  }
  return deathCause;
}

uint16_t PetLogic::getGeneration() const {
  return generation;
}

void PetLogic::setGeneration(uint16_t gen) {
  generation = (gen == 0) ? 1 : gen;
}

const char* PetLogic::getName() const {
  return petName;
}

void PetLogic::setName(const char* name) {
  if (name != nullptr && name[0] != '\0') {
    strncpy(petName, name, sizeof(petName) - 1);
    petName[sizeof(petName) - 1] = '\0';
  } else {
    strncpy(petName, "Lumi", sizeof(petName));
    petName[sizeof(petName) - 1] = '\0';
  }
}

const char* PetLogic::getDeathCauseName() const {
  return getDeathCauseName(getDeathCause());
}

const char* PetLogic::getDeathCauseName(DeathCause cause) {
  switch (cause) {
    case DEATH_HUNGER:     return "Doede af sult";
    case DEATH_COLD:       return "Froes ihjel";
    case DEATH_HEAT:       return "Doede af hedeslag";
    case DEATH_BOREDOM:    return "Doede af ensomhed";
    case DEATH_EXHAUSTION: return "Doede af udmattelse";
    case DEATH_OLD_AGE:    return "Doede af alderdom";
    default:               return "Ukendt aarsag";
  }
}

const char* PetLogic::getDeathCauseCode() const {
  if (alive) return "none";
  return getDeathCauseCode(getDeathCause());
}

const char* PetLogic::getDeathCauseCode(DeathCause cause) {
  switch (cause) {
    case DEATH_HUNGER:     return "hunger";
    case DEATH_COLD:       return "cold";
    case DEATH_HEAT:       return "heat";
    case DEATH_BOREDOM:    return "boredom";
    case DEATH_EXHAUSTION: return "exhaustion";
    case DEATH_OLD_AGE:    return "old_age";
    default:               return "none";
  }
}

const char* PetLogic::getDeathEpitaph() const {
  switch (deathCause) {
    case DEATH_HUNGER:
      return "Maven rumlede til sidst...";
    case DEATH_COLD:
      return "Vinteren var for haard...";
    case DEATH_HEAT:
      return "Smeltede i pejsens varme!";
    case DEATH_BOREDOM:
      return "Hjertet visnede af savn.";
    case DEATH_EXHAUSTION:
      return "Fik aldrig sovet ud...";
    case DEATH_OLD_AGE:
      if (happinessVal >= 60.0f) {
        return "Levede et lykkeligt liv!";
      } else {
        return "Doede i en skoenheds arme!";
      }
    default:
      return "Lumi sover i skovens dyb.";
  }
}

void PetLogic::triggerDeath(DeathCause cause) {
  alive = false;
  deathCause = cause;
  isAsleep = false;
}

void PetLogic::rebirth() {
  uint16_t nextGen = generation + 1;
  uint16_t keepPong = highScorePong;
  uint16_t keepMaze = highScoreMaze;
  uint16_t keepBerries = berryCount;
  char keepName[16];
  strncpy(keepName, petName, sizeof(keepName));
  keepName[sizeof(keepName) - 1] = '\0';
  reset();
  generation = nextGen;
  highScorePong = keepPong;
  highScoreMaze = keepMaze;
  berryCount = keepBerries;
  strncpy(petName, keepName, sizeof(petName));
  petName[sizeof(petName) - 1] = '\0';
}

void PetLogic::startHatching(unsigned long nowMs) {
  hatchingActive = true;
  hatchingStartTimeMs = nowMs;
}

bool PetLogic::isHatching() const {
  return hatchingActive;
}

uint8_t PetLogic::getHatchingPhase(unsigned long nowMs) const {
  if (!hatchingActive) return 0;
  unsigned long elapsed = nowMs - hatchingStartTimeMs;
  if (elapsed < 1000) return 0; // 0..1000 ms: Wobble in nest
  if (elapsed < 2000) return 1; // 1000..2000 ms: Crack + sparks
  return 2;                     // 2000..2800 ms: Shell pop + Baby emerges
}

bool PetLogic::pet(unsigned long nowMs) {
  if (!alive || hatchingActive) return false;
  if (lastPetTimeMs > 0 && (nowMs - lastPetTimeMs < 1500)) return false; // Cooldown 1.5s
  lastPetTimeMs = nowMs;
  pettingActiveUntilMs = nowMs + 1800;

  happinessVal = clamp0100(happinessVal + 12.0f);
  if (isAsleep) {
    isAsleep = false; // Gentle cuddle wakes Lumi up happily
  }
  return true;
}

bool PetLogic::isBeingPetted(unsigned long nowMs) const {
  return (alive && !hatchingActive && (nowMs < pettingActiveUntilMs));
}

const char* PetLogic::getStage() const {
  switch (currentStage) {
    case STAGE_BABY:  return "baby";
    case STAGE_TEEN:  return "teen";
    case STAGE_ADULT: return "adult";
    case STAGE_OLD:   return "old";
    default:          return "teen";
  }
}

PetStage PetLogic::getStageEnum() const {
  return currentStage;
}

void PetLogic::setStage(PetStage stage) {
  currentStage = stage;
}

bool PetLogic::isOverheating() const {
  if (currentOutfit != OUTFIT_WINTER) return false;
  // Overheats when wearing winter clothes while warm/hot
  return (warmthVal >= 75.0f || ambientTemp >= 23.5f);
}

const char* PetLogic::getOutfit() const {
  return (currentOutfit == OUTFIT_WINTER) ? "winter" : "default";
}

PetOutfit PetLogic::getOutfitEnum() const {
  return currentOutfit;
}

void PetLogic::setOutfit(PetOutfit outfit) {
  currentOutfit = outfit;
  if (currentOutfit == OUTFIT_WINTER && warmthVal < 60.0f) {
    // Putting on warm winter clothes immediately provides a comforting warmth boost
    warmthVal = clamp0100(warmthVal + 25.0f);
  }
}

void PetLogic::toggleOutfit() {
  setOutfit((currentOutfit == OUTFIT_DEFAULT) ? OUTFIT_WINTER : OUTFIT_DEFAULT);
}

void PetLogic::formatStateJson(char* buffer, unsigned int bufferSize, const char* petName, const char* petStage) const {
  if (buffer == nullptr || bufferSize == 0) return;

  const char* name = (petName != nullptr && petName[0] != '\0' && strcmp(petName, "Lumi") != 0) ? petName : this->petName;
  const char* stage = (petStage != nullptr && petStage[0] != '\0') ? petStage : getStage();
  const char* causeCode = getDeathCauseCode();
  const char* epitaphStr = !isAlive() ? getDeathEpitaph() : "";

  snprintf(buffer, bufferSize,
           "{\"name\":\"%s\",\"stage\":\"%s\",\"outfit\":\"%s\",\"hunger\":%d,\"warmth\":%d,\"energy\":%d,\"happy\":%d,\"discipline\":%d,\"alive\":%s,\"generation\":%u,\"ageMinutes\":%lu,\"deathCause\":\"%s\",\"epitaph\":\"%s\"}",
           name,
           stage,
           getOutfit(),
           getHunger(),
           getWarmth(),
           getEnergy(),
           getHappiness(),
           getDiscipline(),
           isAlive() ? "true" : "false",
           (unsigned int)getGeneration(),
           (unsigned long)getAgeMinutes(),
           causeCode,
           epitaphStr);
}

uint16_t PetLogic::getHighScorePong() const {
  return highScorePong;
}

void PetLogic::setHighScorePong(uint16_t score) {
  if (score > highScorePong) {
    highScorePong = score;
  }
}

uint16_t PetLogic::getHighScoreMaze() const {
  return highScoreMaze;
}

void PetLogic::setHighScoreMaze(uint16_t score) {
  if (score == 0 || highScoreMaze == 0 || score < highScoreMaze) {
    highScoreMaze = score;
  }
}

uint32_t PetLogic::getAgeMinutes() const {
  return ageMinutes;
}

void PetLogic::setAgeMinutes(uint32_t mins) {
  ageMinutes = mins;
  ageTickCount = 0;
}

uint16_t PetLogic::getBerries() const {
  return berryCount;
}

void PetLogic::addBerries(uint16_t count) {
  if (berryCount + count > 65535) {
    berryCount = 65535;
  } else {
    berryCount += count;
  }
}

bool PetLogic::spendBerries(uint16_t count) {
  if (berryCount >= count) {
    berryCount -= count;
    return true;
  }
  return false;
}

void PetLogic::setBerries(uint16_t count) {
  berryCount = count;
}

bool PetLogic::isSick() const {
  return sick;
}

void PetLogic::setSick(bool isSick) {
  sick = isSick;
  if (!sick) {
    sickTicks = 0;
  }
}

bool PetLogic::cureSickness(uint16_t berryCost) {
  if (!alive || !sick) return false;
  if (berryCount < berryCost) return false;

  berryCount -= berryCost;
  sick = false;
  sickTicks = 0;
  happinessVal = clamp0100(happinessVal + 20.0f);
  return true;
}

uint8_t PetLogic::getSickTicks() const {
  return sickTicks;
}

bool PetLogic::getHasPoop() const {
  return hasPoop;
}

void PetLogic::setHasPoop(bool poop, unsigned long nowMs) {
  hasPoop = poop;
  if (hasPoop) {
    poopTimeMs = nowMs;
  } else {
    poopTimeMs = 0;
  }
}

void PetLogic::cleanPoop() {
  if (hasPoop) {
    hasPoop = false;
    poopTimeMs = 0;
    happinessVal = clamp0100(happinessVal + 15.0f);
  }
}

unsigned long PetLogic::getPoopTimeMs() const {
  return poopTimeMs;
}

