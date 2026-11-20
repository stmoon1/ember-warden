#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace citadel::game {
enum class EnemyGoal { Mixed, CrystalOnly, TowersOnly, ExtractorsOnly };
struct EnemySpec {
  const char *name, *mechanic, *counter;
  EnemyGoal goal;
  int firstWave, weight;
  float healthScale, speedScale, damage;
  int coreDamage;
  std::array<float, 3> color;
};
inline constexpr std::array<EnemySpec, 12> EnemySpecs{
    {{"Ash raider",
      "Approaches nearby players, otherwise follows the siege route.",
      "Keep moving and use the knife against isolated raiders.",
      EnemyGoal::Mixed,
      1,
      16,
      1,
      1,
      7,
      1,
      {1, .65f, .30f}},
     {"Rift hound",
      "Fast flanker with low health and a long player detection radius.",
      "Use slowing towers and close-range weapons.",
      EnemyGoal::Mixed,
      2,
      12,
      .65f,
      1.7f,
      7,
      1,
      {1, .62f, .22f}},
     {"Iron guard",
      "Heavy armor reduces non-piercing damage by 45%.",
      "Use Armor piercing or concentrate tower fire.",
      EnemyGoal::Mixed,
      2,
      9,
      1.8f,
      .7f,
      16,
      1,
      {1, .48f, .21f}},
     {"Frostwing",
      "Flies over terrain and dives toward visible players.",
      "Watch the sky and use ranged weapons.",
      EnemyGoal::Mixed,
      3,
      8,
      .8f,
      1,
      7,
      1,
      {.32f, .85f, 1}},
     {"Rune caster",
      "Attacks players or towers at range when line of sight is clear.",
      "Use cover and eliminate it before it starts casting.",
      EnemyGoal::Mixed,
      4,
      8,
      .85f,
      1,
      7,
      1,
      {.74f, .48f, 1}},
     {"Plague keeper",
      "Restores 4 HP/s to enemies within 4m.",
      "Focus the healer before fighting its group.",
      EnemyGoal::Mixed,
      5,
      6,
      1.3f,
      1,
      7,
      1,
      {.3f, .95f, .63f}},
     {"Brood stalker",
      "Hunts towers from farther away and splits into two weak hounds on "
      "death.",
      "Use area damage and protect exposed towers.",
      EnemyGoal::Mixed,
      6,
      7,
      1,
      1.2f,
      7,
      1,
      {1, .55f, .28f}},
     {"Ember carrier",
      "Carries explosives and causes an explosion when killed.",
      "Fight it away from yourself and fragile towers.",
      EnemyGoal::Mixed,
      7,
      6,
      1.4f,
      1,
      7,
      1,
      {1, .35f, .18f}},
     {"Ash warlord",
      "Appears every fifth wave. Rotates between siege pulse, hound summons "
      "and self-healing.",
      "Keep space for the pulse, clear summons and concentrate boss damage.",
      EnemyGoal::Mixed,
      5,
      0,
      12,
      .65f,
      28,
      5,
      {1, .30f, .37f}},
     {"Crystal runner",
      "Ignores players, towers and taunts. Rushes the crystal along its route; "
      "a breach costs 2 core HP.",
      "Intercept early with slows and concentrated fire.",
      EnemyGoal::CrystalOnly,
      3,
      10,
      .9f,
      1.45f,
      0,
      2,
      {.3f, .85f, 1}},
     {"Siege breaker",
      "Only attacks towers. Searches across the map, ignores players and never "
      "damages the crystal.",
      "Use a Bulwark as bait and attack while it commits to a tower.",
      EnemyGoal::TowersOnly,
      4,
      9,
      1.5f,
      .85f,
      14,
      0,
      {1, .58f, .25f}},
     {"Coin thief",
      "Only attacks extractors. Steals up to 6 credits per hit, carrying at "
      "most 60. Killing it recovers stolen credits; escape loses them.",
      "Protect extractors and kill the thief before it escapes. Other towers "
      "cannot taunt it.",
      EnemyGoal::ExtractorsOnly,
      6,
      8,
      .7f,
      1.55f,
      6,
      0,
      {1, .83f, .3f}}}};
inline const EnemySpec &enemySpec(int kind) {
  return EnemySpecs[std::clamp(kind, 0, int(EnemySpecs.size()) - 1)];
}
inline const char *goalName(EnemyGoal goal) {
  switch (goal) {
  case EnemyGoal::CrystalOnly:
    return "Crystal only";
  case EnemyGoal::TowersOnly:
    return "Towers only";
  case EnemyGoal::ExtractorsOnly:
    return "Extractors only";
  default:
    return "Players, towers and crystal";
  }
}
inline float waveEnemyHealth(int wave) {
  return 40 * std::pow(1.13f, float(std::min(120, wave - 1)));
}
inline float waveEnemySpeed(int wave) {
  return std::min(3.2f, 1 + .055f * (wave - 1));
}
} // namespace citadel::game
