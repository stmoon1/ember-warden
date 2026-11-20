#pragma once
#include "Arsenal.h"

namespace citadel::game {
enum Element : unsigned {
  Fire = 1,
  Ice = 2,
  Lightning = 4,
  Poison = 8,
  Bleeding = 16,
  Penetration = 32
};
inline unsigned requiredElements(Perk perk) {
  switch (perk) {
  case Perk::BurnPower:
  case Perk::BurnDuration:
  case Perk::FireSpread:
  case Perk::FireExplosion:
    return Fire;
  case Perk::Shatter:
  case Perk::CryoDuration:
  case Perk::IceFragile:
  case Perk::IceBurst:
  case Perk::Permafrost:
    return Ice;
  case Perk::TeslaJumps:
  case Perk::ShockPower:
  case Perk::ShockRange:
  case Perk::ReloadNova:
    return Lightning;
  case Perk::PoisonPower:
  case Perk::PoisonCapacity:
  case Perk::PoisonSpread:
    return Poison;
  case Perk::BleedPower:
  case Perk::BleedCrit:
    return Bleeding;
  case Perk::RifleDrill:
  case Perk::SniperRail:
    return Penetration;
  case Perk::Conductor:
    return Lightning | Ice;
  case Perk::Volatile:
    return Fire | Ice;
  case Perk::ToxicFlame:
    return Fire | Poison;
  case Perk::BloodFire:
    return Fire | Bleeding;
  case Perk::StormVenom:
    return Lightning | Poison;
  case Perk::Overload:
    return Fire | Poison | Ice;
  default:
    return 0;
  }
}
inline unsigned enabledElements(Perk perk) {
  switch (perk) {
  case Perk::Incendiary:
  case Perk::TowerFire:
    return Fire;
  case Perk::CryoRounds:
  case Perk::TowerFrost:
    return Ice;
  case Perk::Shock:
  case Perk::TowerShock:
    return Lightning;
  case Perk::Venom:
  case Perk::TowerPoison:
    return Poison;
  case Perk::Bleed:
    return Bleeding;
  case Perk::Piercing:
    return Penetration;
  default:
    return 0;
  }
}
inline bool requiresAnyAilment(Perk perk) {
  return perk == Perk::ElementMaster || perk == Perk::StatusHunter;
}
inline unsigned augmentElements(Perk perk) {
  return requiredElements(perk) | enabledElements(perk) |
         (requiresAnyAilment(perk) ? Fire | Ice | Lightning | Poison | Bleeding
                                   : 0);
}
inline bool reactionAugment(Perk perk) {
  unsigned bits = requiredElements(perk);
  return bits && (bits & (bits - 1));
}
enum class CatalogCategory {
  All,
  Stats,
  Mechanics,
  Weapons,
  Melee,
  Towers,
  Economy,
  Survival,
  Fire,
  Ice,
  Lightning,
  Poison,
  Bleeding,
  Penetration,
  Reactions,
  Sets,
  Count
};
inline const char *categoryName(CatalogCategory category) {
  constexpr const char *names[] = {"All augments",
                                   "Stat upgrades",
                                   "Mechanic upgrades",
                                   "Ranged weapons",
                                   "Melee weapons",
                                   "Tower upgrades",
                                   "Economy",
                                   "Survival",
                                   "Fire",
                                   "Ice",
                                   "Lightning",
                                   "Poison",
                                   "Bleeding",
                                   "Penetration",
                                   "Element reactions",
                                   "Set bonuses"};
  return names[int(category)];
}
inline bool mechanicAugment(Perk perk) {
  if (enabledElements(perk) || reactionAugment(perk))
    return true;
  switch (perk) {
  case Perk::Thorns:
  case Perk::Cleave:
  case Perk::Ricochet:
  case Perk::Vampire:
  case Perk::Repair:
  case Perk::Execution:
  case Perk::RelayMedic:
  case Perk::RifleBurst:
  case Perk::BatShockwave:
  case Perk::KatanaDraw:
  case Perk::MeleeCombo:
  case Perk::TwinShot:
  case Perk::FreeAmmo:
  case Perk::KillReload:
  case Perk::ReloadNova:
  case Perk::LastRound:
  case Perk::FirstRound:
  case Perk::Emergency:
  case Perk::Dodge:
  case Perk::Retaliation:
  case Perk::BulwarkThorns:
  case Perk::BulwarkMend:
  case Perk::BulwarkTaunt:
  case Perk::TwinTurret:
  case Perk::FireSpread:
  case Perk::FireExplosion:
  case Perk::PoisonSpread:
  case Perk::IceBurst:
    return true;
  default:
    return false;
  }
}
inline bool categoryContains(CatalogCategory category, Perk perk) {
  switch (category) {
  case CatalogCategory::All:
    return true;
  case CatalogCategory::Stats:
    return !mechanicAugment(perk) && !augmentElements(perk);
  case CatalogCategory::Mechanics:
    return mechanicAugment(perk);
  case CatalogCategory::Fire:
    return augmentElements(perk) & Fire;
  case CatalogCategory::Ice:
    return augmentElements(perk) & Ice;
  case CatalogCategory::Lightning:
    return augmentElements(perk) & Lightning;
  case CatalogCategory::Poison:
    return augmentElements(perk) & Poison;
  case CatalogCategory::Bleeding:
    return augmentElements(perk) & Bleeding;
  case CatalogCategory::Penetration:
    return augmentElements(perk) & Penetration;
  case CatalogCategory::Reactions:
    return reactionAugment(perk);
  case CatalogCategory::Melee:
    return perk == Perk::Melee || perk == Perk::Cleave ||
           perk == Perk::Vampire || perk == Perk::Bleed ||
           (perk >= Perk::KnifePrecision && perk <= Perk::MeleeKnockback);
  case CatalogCategory::Weapons:
    return perk == Perk::Ballistics || perk == Perk::Tempo ||
           perk == Perk::Ricochet || perk == Perk::Execution ||
           (perk >= Perk::CryoRounds && perk <= Perk::Piercing) ||
           (perk >= Perk::SteadyAim && perk <= Perk::ShotgunBlast) ||
           (perk >= Perk::FullHealth && perk <= Perk::Stationary) ||
           perk == Perk::Reserves;
  case CatalogCategory::Towers:
    return perk == Perk::Engineering || perk == Perk::Thorns ||
           perk == Perk::Overclock ||
           (perk >= Perk::TowerFire && perk <= Perk::TowerShock) ||
           perk == Perk::Dividend || perk == Perk::Repair ||
           perk == Perk::RelayMedic || perk == Perk::TowerDiscount ||
           (perk >= Perk::TowerArmor && perk <= Perk::TwinTurret);
  case CatalogCategory::Economy:
    return perk == Perk::Treasury || perk == Perk::Dividend ||
           (perk >= Perk::Discount && perk <= Perk::BossBounty) ||
           perk == Perk::ExtractorInterest || perk == Perk::TowerBounty;
  case CatalogCategory::Survival:
    return perk == Perk::Vitality || perk == Perk::Vampire ||
           perk == Perk::RelayMedic ||
           (perk >= Perk::MoveSpeed && perk <= Perk::Retaliation);
  default:
    return false;
  }
}
} // namespace citadel::game
