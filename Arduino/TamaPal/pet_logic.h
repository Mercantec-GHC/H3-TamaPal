// WHAT: Pure business logic for Lumi's 5 Tamagotchi vitals (Sult, Varme, Energi, Glæde, Disciplin)
// WHY:  Separated from hardware and graphics for deterministic testing on Mac / PC
// USED BY: TamaPal.ino, ui_stats.cpp, tests/test_main.cpp

#ifndef PET_LOGIC_H
#define PET_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "settings.h"

class PetLogic {
public:
  PetLogic();

  // Reset or initialize vitals to healthy defaults
  void reset();

  // Player interaction actions
  bool feed();     // Increases Hunger (sult/mæthed) and slightly Happiness
  bool play();     // Increases Happiness, consumes Energy and Hunger
  bool scold();    // Increases Discipline, slightly reduces Happiness
  bool applyReward(const char* rewardType, int amount); // Backend reward (e.g. "happy_boost")
  void toggleSleep();
  void setSleeping(bool sleep);

  // Environmental sensor updates (ambient room temperature & light from MKR Carrier)
  void setAmbientTemperature(float celsius);
  void setAmbientLight(int lux, unsigned long nowMs = 0);

  // Periodic decay and simulation step (called periodically with millis())
  void update(unsigned long currentTimeMs);

  // Getters for the 5 vital stats (0 to 100%)
  int getHunger() const;      // Sult / Mæthed (100 = mæt, 0 = udsultet)
  int getWarmth() const;      // Varme (100 = behagelig, 0 = for kold/varm)
  int getEnergy() const;      // Energi / Søvn (100 = udhvilet, 0 = udmattet)
  int getHappiness() const;   // Glæde (100 = super glad, 0 = trist)
  int getDiscipline() const;  // Disciplin (100 = velopdragen, 0 = ulydig)

  // Overall status queries
  int getOverallHealth() const;       // Weighted average (0..100)
  bool isSleeping() const;
  bool isAlive() const;               // True if overall health > 0
  const char* getStage() const;       // Lifecycle stage name: "baby", "teen", "adult", "old"
  PetStage getStageEnum() const;      // Current stage enum
  void setStage(PetStage stage);      // Set lifecycle stage
  const char* getOutfit() const;      // Wardrobe outfit name: "default", "winter"
  PetOutfit getOutfitEnum() const;    // Current wardrobe outfit enum
  void setOutfit(PetOutfit outfit);   // Set wardrobe outfit
  void toggleOutfit();                // Toggle between default and winter outfit
  bool isOverheating() const;         // True if wearing winter clothes while warm (> 75% or > 23.5C)
  bool needsAttention() const;        // True if any vital is below PET_CRITICAL_THRESHOLD
  const char* getMoodStatusDanish() const;

  // JSON serialization for MQTT syncing (docs/MESSAGES.md Del 1b Proposal 3A)
  void formatStateJson(char* buffer, unsigned int bufferSize, const char* petName = "Lumi", const char* petStage = nullptr) const;

  // Direct vital setter for test verification
  void setVitals(float hunger, float warmth, float energy, float happiness, float discipline);

  // Minigame records and character age (persisted to MicroSD)
  uint16_t getHighScorePong() const;
  void setHighScorePong(uint16_t score);
  uint16_t getHighScoreMaze() const;
  void setHighScoreMaze(uint16_t score);
  uint32_t getAgeMinutes() const;
  void setAgeMinutes(uint32_t mins);

private:
  float hungerVal;
  float warmthVal;
  float energyVal;
  float happinessVal;
  float disciplineVal;

  uint32_t ageMinutes;
  uint16_t highScorePong;
  uint16_t highScoreMaze;
  uint8_t ageTickCount;

  PetStage currentStage;
  PetOutfit currentOutfit;
  bool isAsleep;
  bool decayInitialized;
  unsigned long lastDecayTimeMs;
  unsigned long darkStartTimeMs;
  unsigned long lightStartTimeMs;
  float ambientTemp;
  int ambientLux;

  static float clamp0100(float val);
};

#endif // PET_LOGIC_H
