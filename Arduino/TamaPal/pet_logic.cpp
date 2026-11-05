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

  currentStage = STAGE_BABY; // Starts as cute egg baby
  isAsleep = false;
  decayInitialized = false;
  lastDecayTimeMs = 0;
  darkStartTimeMs = 0;
  lightStartTimeMs = 0;
  ambientTemp = 22.0f;
  ambientLux = 150;
}

void PetLogic::setVitals(float hunger, float warmth, float energy, float happiness, float discipline) {
  hungerVal = clamp0100(hunger);
  warmthVal = clamp0100(warmth);
  energyVal = clamp0100(energy);
  happinessVal = clamp0100(happiness);
  disciplineVal = clamp0100(discipline);
}

bool PetLogic::feed() {
  if (isAsleep) return false;     // Cannot feed while sleeping
  if (hungerVal >= 100.0f) return false; // Already completely full

  hungerVal = clamp0100(hungerVal + PET_FEED_RESTORE);
  happinessVal = clamp0100(happinessVal + 5.0f);
  return true;
}

bool PetLogic::play() {
  if (isAsleep) return false;    // Cannot play while sleeping
  if (energyVal < 15.0f) return false; // Too exhausted to play

  happinessVal = clamp0100(happinessVal + PET_PLAY_RESTORE);
  energyVal = clamp0100(energyVal - 15.0f);
  hungerVal = clamp0100(hungerVal - 5.0f);
  return true;
}

bool PetLogic::scold() {
  if (isAsleep) return false;

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

  // Ideal temperature is around 22.0 deg Celsius
  float diff = (celsius >= 22.0f) ? (celsius - 22.0f) : (22.0f - celsius);
  float targetWarmth = 100.0f - (diff * 7.0f);
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
  if (!decayInitialized) {
    lastDecayTimeMs = currentTimeMs;
    decayInitialized = true;
    return;
  }

  if (currentTimeMs - lastDecayTimeMs >= PET_DECAY_INTERVAL_MS) {
    unsigned long elapsedIntervals = (currentTimeMs - lastDecayTimeMs) / PET_DECAY_INTERVAL_MS;
    lastDecayTimeMs += elapsedIntervals * PET_DECAY_INTERVAL_MS;

    float intervals = (float)elapsedIntervals;
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
  if (getHunger() < PET_CRITICAL_THRESHOLD) {
    return "Lumi er sulten!";
  }
  if (getWarmth() < PET_CRITICAL_THRESHOLD) {
    return "Lumi fryser!";
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
  return getOverallHealth() > 0;
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

void PetLogic::formatStateJson(char* buffer, unsigned int bufferSize, const char* petName, const char* petStage) const {
  if (buffer == nullptr || bufferSize == 0) return;

  const char* name = (petName != nullptr && petName[0] != '\0') ? petName : "Lumi";
  const char* stage = (petStage != nullptr && petStage[0] != '\0') ? petStage : getStage();

  snprintf(buffer, bufferSize,
           "{\"name\":\"%s\",\"stage\":\"%s\",\"hunger\":%d,\"warmth\":%d,\"energy\":%d,\"happy\":%d,\"discipline\":%d,\"alive\":%s}",
           name,
           stage,
           getHunger(),
           getWarmth(),
           getEnergy(),
           getHappiness(),
           getDiscipline(),
           isAlive() ? "true" : "false");
}

