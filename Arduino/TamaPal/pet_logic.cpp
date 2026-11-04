// WHAT: Pure business logic implementation for Lumi's 5 Tamagotchi vitals
// WHY:  Zero Arduino/hardware dependencies - pure portable C++ with 100% test coverage
// USED BY: TamaPal.ino, ui_stats.cpp, tests/test_main.cpp

#include "pet_logic.h"

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

  isAsleep = false;
  decayInitialized = false;
  lastDecayTimeMs = 0;
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

void PetLogic::toggleSleep() {
  setSleeping(!isAsleep);
}

void PetLogic::setSleeping(bool sleep) {
  isAsleep = sleep;
}

void PetLogic::setAmbientTemperature(float celsius) {
  ambientTemp = celsius;

  // Ideal temperature is around 22.0 deg Celsius
  float diff = (celsius >= 22.0f) ? (celsius - 22.0f) : (22.0f - celsius);
  float targetWarmth = 100.0f - (diff * 7.0f);
  targetWarmth = clamp0100(targetWarmth);

  // Smooth adjustment towards the environmental target (EMA filter)
  warmthVal = (warmthVal * 0.80f) + (targetWarmth * 0.20f);
  warmthVal = clamp0100(warmthVal);
}

void PetLogic::setAmbientLight(int lux) {
  ambientLux = lux;

  // If very dark (< 30 lux) and Lumi is getting tired, Lumi falls asleep
  if (ambientLux < 30 && energyVal < 40.0f && !isAsleep) {
    isAsleep = true;
  }
  // If lights turn on (> 100 lux) and Lumi is well-rested, Lumi wakes up
  else if (ambientLux > 100 && isAsleep && energyVal > 85.0f) {
    isAsleep = false;
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

    for (unsigned long i = 0; i < elapsedIntervals; i++) {
      if (isAsleep) {
        // Recover energy while sleeping
        energyVal = clamp0100(energyVal + PET_SLEEP_ENERGY_RECOVERY);
        // Hunger decays slower while asleep (half rate)
        hungerVal = clamp0100(hungerVal - (PET_DECAY_HUNGER * 0.5f));
      } else {
        // Normal daytime awake decay
        energyVal = clamp0100(energyVal - PET_DECAY_ENERGY);
        hungerVal = clamp0100(hungerVal - PET_DECAY_HUNGER);
        happinessVal = clamp0100(happinessVal - PET_DECAY_HAPPINESS);
        disciplineVal = clamp0100(disciplineVal - PET_DECAY_DISCIPLINE);
      }
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
