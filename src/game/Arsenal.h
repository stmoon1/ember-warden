#pragma once
#include <array>
namespace citadel::game {
struct WeaponSpec {
  const char *name;
  int price;
  float damage, interval, range;
  int magazine, pellets, ammoPrice;
  float reload, spread;
};
inline constexpr std::array<WeaponSpec, 7> Weapons{
    {{"Knife", 0, 20, .50f, 2.5f, 0, 1, 0, 0, 0},
     {"Rifle", 120, 18, .14f, 35, 30, 1, 24, 1.8f, .012f},
     {"Machine gun", 240, 14, .085f, 30, 80, 1, 40, 3.2f, .030f},
     {"Sniper", 260, 125, 1.25f, 65, 5, 1, 30, 2.6f, .002f},
     {"Shotgun", 170, 12, .85f, 14, 6, 8, 24, 2.4f, .10f},
     {"Baseball bat", 100, 42, .80f, 3.1f, 0, 1, 0, 0, 0},
     {"Katana", 230, 34, .32f, 3.5f, 0, 1, 0, 0, 0}}};
struct TowerSpec {
  const char *name;
  const char *role;
  int price;
  float damage, interval, range, hp;
  int income;
};
inline constexpr std::array<TowerSpec, 6> TowerSpecs{
    {{"Cannon", "Attack / single target", 100, 24, .75f, 8, 180, 0},
     {"Cryo turret", "Attack / slow", 130, 12, .60f, 7, 160, 0},
     {"Relay", "Buff / +20% damage and attack speed", 140, 0, 1, 6, 160, 0},
     {"Extractor", "Economy / income each cleared wave", 120, 0, 1, 0, 90, 35},
     {"Bulwark", "Defence / high health and taunt; requires augment to reflect",
      85, 0, 1, 3.5f, 650, 0},
     {"Tesla", "Attack / chain lightning", 190, 20, .90f, 7, 170, 0}}};
enum class Perk {
  Ballistics,
  Melee,
  Engineering,
  Vitality,
  Tempo,
  Treasury,
  Thorns,
  Overclock,
  Cleave,
  Ricochet,
  Vampire,
  CryoRounds,
  Incendiary,
  Venom,
  Bleed,
  Shock,
  Piercing,
  TowerFire,
  TowerFrost,
  TowerPoison,
  TowerShock,
  Conductor,
  Shatter,
  Volatile,
  Dividend,
  Repair,
  Execution,
  RelayMedic,
  SteadyAim,
  LongBarrel,
  LargeMagazine,
  SpeedLoader,
  CriticalEye,
  DeadlyFocus,
  RifleDiscipline,
  RifleBurst,
  RifleDrill,
  MachineFeed,
  MachineSpin,
  MachineStorm,
  SniperFocus,
  SniperRail,
  SniperBoss,
  ShotgunChoke,
  ShotgunPellets,
  ShotgunBlast,
  KnifePrecision,
  KnifeSpeed,
  BatImpact,
  BatShockwave,
  KatanaFlurry,
  KatanaDraw,
  MeleeReach,
  MeleeCombo,
  MeleeCrit,
  MeleeKnockback,
  FullHealth,
  LowHealth,
  CloseRange,
  LongRange,
  FirstBlood,
  Finisher,
  EliteHunter,
  CrowdHunter,
  GlassCannon,
  HeavyRounds,
  TwinShot,
  FreeAmmo,
  KillReload,
  ReloadNova,
  LastRound,
  FirstRound,
  MovingFire,
  Stationary,
  MoveSpeed,
  Armor,
  Regeneration,
  KillHeal,
  WaveShield,
  Emergency,
  Dodge,
  Retaliation,
  Reserves,
  Discount,
  TowerDiscount,
  WaveBounty,
  Interest,
  BossBounty,
  TowerArmor,
  TowerRate,
  TowerReach,
  TowerCrit,
  TowerBounty,
  CannonSplash,
  CannonHeavy,
  CryoDuration,
  TeslaJumps,
  RelayRange,
  RelayArmor,
  RelayIncome,
  ExtractorArmor,
  ExtractorInterest,
  BulwarkThorns,
  BulwarkMend,
  BulwarkTaunt,
  TowerRegen,
  UpgradeDiscount,
  TwinTurret,
  BurnPower,
  BurnDuration,
  FireSpread,
  FireExplosion,
  PoisonPower,
  PoisonCapacity,
  PoisonSpread,
  BleedPower,
  BleedCrit,
  IceFragile,
  IceBurst,
  ShockPower,
  ShockRange,
  ToxicFlame,
  BloodFire,
  StormVenom,
  Permafrost,
  ElementMaster,
  StatusHunter,
  Overload,
  Count
};
enum class Rarity { Bronze, Silver, Gold };
inline constexpr std::array<int, 3> RarityWeights{{60, 30, 10}};
inline constexpr Rarity perkRarity(Perk perk) {
  switch (perk) {
  case Perk::Cleave:
  case Perk::Ricochet:
  case Perk::Vampire:
  case Perk::Piercing:
  case Perk::Conductor:
  case Perk::Shatter:
  case Perk::Volatile:
  case Perk::Execution:
  case Perk::RelayMedic:
  case Perk::MachineStorm:
  case Perk::SniperRail:
  case Perk::SniperBoss:
  case Perk::BatShockwave:
  case Perk::KatanaDraw:
  case Perk::MeleeCombo:
  case Perk::GlassCannon:
  case Perk::TwinShot:
  case Perk::KillReload:
  case Perk::ReloadNova:
  case Perk::Emergency:
  case Perk::Retaliation:
  case Perk::CannonSplash:
  case Perk::TeslaJumps:
  case Perk::BulwarkThorns:
  case Perk::TwinTurret:
  case Perk::FireSpread:
  case Perk::FireExplosion:
  case Perk::PoisonSpread:
  case Perk::IceBurst:
  case Perk::ToxicFlame:
  case Perk::BloodFire:
  case Perk::StormVenom:
  case Perk::Overload:
    return Rarity::Gold;
  case Perk::Thorns:
  case Perk::Overclock:
  case Perk::CryoRounds:
  case Perk::Incendiary:
  case Perk::Venom:
  case Perk::Bleed:
  case Perk::Shock:
  case Perk::TowerFire:
  case Perk::TowerFrost:
  case Perk::TowerPoison:
  case Perk::TowerShock:
  case Perk::Dividend:
  case Perk::Repair:
  case Perk::RifleBurst:
  case Perk::RifleDrill:
  case Perk::MachineSpin:
  case Perk::ShotgunPellets:
  case Perk::ShotgunBlast:
  case Perk::MeleeCrit:
  case Perk::FullHealth:
  case Perk::LowHealth:
  case Perk::Finisher:
  case Perk::EliteHunter:
  case Perk::HeavyRounds:
  case Perk::FreeAmmo:
  case Perk::LastRound:
  case Perk::FirstRound:
  case Perk::Armor:
  case Perk::WaveShield:
  case Perk::Dodge:
  case Perk::Interest:
  case Perk::BossBounty:
  case Perk::TowerCrit:
  case Perk::TowerBounty:
  case Perk::CannonHeavy:
  case Perk::CryoDuration:
  case Perk::RelayArmor:
  case Perk::RelayIncome:
  case Perk::ExtractorArmor:
  case Perk::ExtractorInterest:
  case Perk::BulwarkMend:
  case Perk::TowerRegen:
  case Perk::PoisonCapacity:
  case Perk::BleedCrit:
  case Perk::IceFragile:
  case Perk::ShockRange:
  case Perk::ElementMaster:
  case Perk::StatusHunter:
    return Rarity::Silver;
  default:
    return Rarity::Bronze;
  }
}
enum class Bonus {
  None,
  Spread,
  GunRange,
  Magazine,
  ReloadSpeed,
  CritChance,
  CritDamage,
  RifleDamage,
  RifleBurst,
  RiflePierce,
  MachineMagazine,
  MachineSpin,
  MachineStorm,
  SniperCrit,
  SniperPierce,
  SniperBoss,
  ShotgunChoke,
  ShotgunPellets,
  ShotgunKnockback,
  KnifeCrit,
  KnifeSpeed,
  BatImpact,
  BatShockwave,
  KatanaSpeed,
  KatanaDraw,
  MeleeReach,
  MeleeCombo,
  MeleeCritDamage,
  MeleeKnockback,
  FullHealth,
  LowHealth,
  CloseRange,
  LongRange,
  FirstBlood,
  Finisher,
  BossDamage,
  CrowdDamage,
  GlassCannon,
  HeavyRounds,
  TwinShot,
  FreeAmmo,
  KillReload,
  ReloadNova,
  LastRound,
  FirstRound,
  MovingFire,
  Stationary,
  MoveSpeed,
  Armor,
  Regeneration,
  KillHeal,
  WaveShield,
  Emergency,
  Dodge,
  Retaliation,
  AmmoPack,
  Discount,
  TowerDiscount,
  WaveBounty,
  Interest,
  BossBounty,
  TowerHealth,
  TowerRate,
  TowerRange,
  TowerCrit,
  TowerBounty,
  CannonSplash,
  CannonHeavy,
  CryoDuration,
  TeslaJumps,
  RelayRange,
  RelayArmor,
  RelayIncome,
  ExtractorArmor,
  ExtractorNetwork,
  BulwarkThorns,
  BulwarkMend,
  BulwarkTaunt,
  TowerRegen,
  UpgradeDiscount,
  TwinTurret,
  BurnPower,
  BurnDuration,
  FireSpread,
  FireExplosion,
  PoisonPower,
  PoisonCapacity,
  PoisonSpread,
  BleedPower,
  BleedCrit,
  IceFragile,
  IceBurst,
  ShockPower,
  ShockRange,
  ToxicFlame,
  BloodFire,
  StormVenom,
  Permafrost,
  ElementPower,
  StatusHunter,
  Overload
};
struct PerkSpec {
  const char *name;
  const char *description;
  Bonus bonus = Bonus::None;
  float amount = 0;
  int maxStacks = 1;
};
inline constexpr std::array<PerkSpec, static_cast<int>(Perk::Count)> Perks{
    {{"Ballistics", "Ranged weapon damage +20% (additive)."},
     {"Blade mastery", "Melee damage +25%, reach +0.25m."},
     {"Engineering", "All tower damage +20%."},
     {"Vitality", "Max health +25, heal 40."},
     {"Quick hands", "Attack speed +12%, reload speed +15%."},
     {"Bounty hunter", "Kill income +20%."},
     {"Reactive armor", "All towers reflect 40% of incoming damage."},
     {"Overclock", "Relay damage bonus +10%, fire rate bonus +10%."},
     {"Whirlwind", "All melee weapons cleave every enemy in their arc.",
      Bonus::None, 0, 1},
     {"Ricochet", "Gun hits bounce to another enemy within 4m for 35% damage.",
      Bonus::None, 0, 1},
     {"Blood drinker", "Melee hits heal 2 HP."},
     {"Cryo rounds",
      "Weapon hits chill. Repeated chill freezes, bosses resist."},
     {"Incendiary", "Weapons ignite for 6 DPS for 3s."},
     {"Venom", "Weapons poison, stacking to 8, each stack deals 2 DPS."},
     {"Serrated edge", "Melee applies bleeding, 8 DPS for 4s."},
     {"Static charge", "Weapon hits arc for 8 damage within 4m."},
     {"Armor piercing", "Guns pierce one more enemy, ignore armor."},
     {"Furnace turrets", "Tower hits ignite for 6 DPS."},
     {"Glacial network", "All attacking towers chill on hit."},
     {"Toxic industry", "Tower hits apply stacking poison."},
     {"Storm grid", "All attacking towers chain lightning."},
     {"Conductive ice", "Lightning deals +60% damage to chilled targets.",
      Bonus::None, 0, 1},
     {"Shatter", "Frozen targets take +50% direct damage.", Bonus::None, 0, 1},
     {"Thermal shock", "Fire + chill consumes both, 30 AoE damage.",
      Bonus::None, 0, 1},
     {"Compound interest", "Extractors produce +15 credits per wave."},
     {"Repair crews", "All towers restore 20% max HP after each wave."},
     {"Executioner", "Sniper hits below 30% HP execute non-boss enemies.",
      Bonus::None, 0, 1},
     {"Field hospital", "Relay aura heals player 3 HP/s."},
     {"Steady aim", "Gun spread -25%.", Bonus::Spread, -0.25f, 1},
     {"Long barrel", "Gun range +30%.", Bonus::GunRange, 0.3f, 1},
     {"Expanded magazines", "Gun magazine capacity +30%.", Bonus::Magazine,
      0.3f, 1},
     {"Speed loader", "Reload speed +25%.", Bonus::ReloadSpeed, 0.25f, 1},
     {"Critical eye", "Critical chance +10 percentage points.",
      Bonus::CritChance, 0.1f, 1},
     {"Deadly focus", "Critical damage +50 percentage points.",
      Bonus::CritDamage, 0.5f, 1},
     {"Rifle discipline", "Rifle damage +30%.", Bonus::RifleDamage, 0.3f, 1},
     {"Triple cadence", "Every third rifle shot gains +80% damage.",
      Bonus::RifleBurst, 0.8f, 1},
     {"Drill rounds", "Rifle shots penetrate one extra enemy.",
      Bonus::RiflePierce, 1.0f, 1},
     {"Belt feeder", "Machine gun magazine +50%.", Bonus::MachineMagazine, 0.5f,
      1},
     {"Spinning fury", "Machine gun fire rate ramps up to +40% while firing.",
      Bonus::MachineSpin, 0.4f, 1},
     {"Lead storm", "Every eighth machine gun shot launches an explosive hit.",
      Bonus::MachineStorm, 1.0f, 1},
     {"Patient hunter", "Sniper critical chance +25 percentage points.",
      Bonus::SniperCrit, 0.25f, 1},
     {"Rail lance", "Sniper penetrates three extra targets.",
      Bonus::SniperPierce, 3.0f, 1},
     {"Giant slayer", "Sniper damage against bosses +60%.", Bonus::SniperBoss,
      0.6f, 1},
     {"Focused choke", "Shotgun spread -40%, range +4m.", Bonus::ShotgunChoke,
      1.0f, 1},
     {"Flechette cloud", "Shotgun fires three extra pellets.",
      Bonus::ShotgunPellets, 3.0f, 1},
     {"Breaching shot", "Shotgun impacts have +1.5m knockback.",
      Bonus::ShotgunKnockback, 1.5f, 1},
     {"Stiletto", "Knife critical chance +25 percentage points.",
      Bonus::KnifeCrit, 0.25f, 1},
     {"Knife dance", "Knife attack speed +35%.", Bonus::KnifeSpeed, 0.35f, 1},
     {"Grand slam", "Bat damage +50%, knockback +1m.", Bonus::BatImpact, 0.5f,
      1},
     {"Groundbreaker", "Bat swings deal 18 damage in a 4m frontal wave.",
      Bonus::BatShockwave, 18.0f, 1},
     {"Silver flurry", "Katana attack speed +30%.", Bonus::KatanaSpeed, 0.3f,
      1},
     {"Draw cut", "First katana hit after switching deals +150% damage.",
      Bonus::KatanaDraw, 1.5f, 1},
     {"Lunging edge", "Melee reach +0.8m.", Bonus::MeleeReach, 0.8f, 1},
     {"Rhythm blade", "Consecutive melee hits build up to +60% damage.",
      Bonus::MeleeCombo, 0.6f, 1},
     {"Bladed fortune", "Melee critical damage +75 percentage points.",
      Bonus::MeleeCritDamage, 0.75f, 1},
     {"Concussive edges", "Melee knockback +1m.", Bonus::MeleeKnockback, 1.0f,
      1},
     {"Untouched", "Deal +30% damage at full health.", Bonus::FullHealth, 0.3f,
      1},
     {"Last stand", "Deal +60% damage below 35% health.", Bonus::LowHealth,
      0.6f, 1},
     {"Point blank", "Deal +35% gun damage within 5m.", Bonus::CloseRange,
      0.35f, 1},
     {"Eagle range", "Deal +40% gun damage beyond 12m.", Bonus::LongRange, 0.4f,
      1},
     {"First blood", "Deal +35% damage to full-health enemies.",
      Bonus::FirstBlood, 0.35f, 1},
     {"Merciless", "Deal +40% damage to enemies below 30% health.",
      Bonus::Finisher, 0.4f, 1},
     {"Trophy hunter", "All damage against bosses +25%.", Bonus::BossDamage,
      0.25f, 1},
     {"Crowd hunter", "Deal +20% damage when 8 or more enemies are alive.",
      Bonus::CrowdDamage, 0.2f, 1},
     {"Glass reactor", "All damage +45%, incoming player damage +20%.",
      Bonus::GlassCannon, 0.45f, 1},
     {"Heavy rounds", "Gun damage +35%, attack rate -12%.", Bonus::HeavyRounds,
      0.35f, 1},
     {"Echo chamber", "Every fifth gun shot repeats its pellets for free.",
      Bonus::TwinShot, 1.0f, 1},
     {"Lucky chamber", "20% chance a gun attack consumes no ammunition.",
      Bonus::FreeAmmo, 0.2f, 1},
     {"Scavenged rounds",
      "Weapon kills restore two rounds to the active magazine.",
      Bonus::KillReload, 2.0f, 1},
     {"Reload discharge",
      "Completing a reload shocks enemies within 5m for 25 damage.",
      Bonus::ReloadNova, 25.0f, 1},
     {"Final cartridge", "Last bullet in a magazine deals +120% damage.",
      Bonus::LastRound, 1.2f, 1},
     {"Opening salvo", "First bullet of a full magazine deals +60% damage.",
      Bonus::FirstRound, 0.6f, 1},
     {"Skirmisher", "Gun attack rate +15% while moving.", Bonus::MovingFire,
      0.15f, 1},
     {"Entrenched", "Gun damage +25% while stationary.", Bonus::Stationary,
      0.25f, 1},
     {"Fleet feet", "Movement speed +12%.", Bonus::MoveSpeed, 0.12f, 1},
     {"Plated coat", "Incoming player damage -15%, capped at 70%.",
      Bonus::Armor, 0.15f, 1},
     {"Second wind", "Regenerate 1 HP/s after 4 seconds without damage.",
      Bonus::Regeneration, 1.0f, 1},
     {"Soul collector", "Every kill heals 1 HP.", Bonus::KillHeal, 1.0f, 1},
     {"Wave barrier", "Gain 20 shield at the beginning of each wave.",
      Bonus::WaveShield, 20.0f, 1},
     {"Emergency protocol",
      "Once per wave, lethal damage instead restores 35 HP.", Bonus::Emergency,
      35.0f, 1},
     {"Phase weave", "10% chance to evade incoming player damage.",
      Bonus::Dodge, 0.1f, 1},
     {"Pain pulse", "Taking damage deals 12 damage to enemies within 3m.",
      Bonus::Retaliation, 12.0f, 1},
     {"Reserve cache", "Ammo packs contain 50% more ammunition.",
      Bonus::AmmoPack, 0.5f, 1},
     {"Supply contract", "Weapon and ammunition prices -15%, capped at 50%.",
      Bonus::Discount, 0.15f, 1},
     {"Builder contract", "Tower blueprints cost -15%, capped at 50%.",
      Bonus::TowerDiscount, 0.15f, 1},
     {"Hazard pay", "Wave-clear reward +25%.", Bonus::WaveBounty, 0.25f, 1},
     {"Savings account", "Earn 5% of banked credits at wave clear, maximum 60.",
      Bonus::Interest, 0.05f, 1},
     {"Boss bounty", "Boss kill reward +80 credits.", Bonus::BossBounty, 80.0f,
      1},
     {"Reinforced frames", "New and existing towers gain +30% maximum health.",
      Bonus::TowerHealth, 0.3f, 1},
     {"Clockwork engine", "Attacking towers fire 20% faster.", Bonus::TowerRate,
      0.2f, 1},
     {"Surveyor optics", "Tower attack range +20%.", Bonus::TowerRange, 0.2f,
      1},
     {"Precision servos", "Towers gain 15% critical chance.", Bonus::TowerCrit,
      0.15f, 1},
     {"Automated salvage", "Tower kills award +5 credits.", Bonus::TowerBounty,
      5.0f, 1},
     {"Cluster shells", "Cannon hits splash for 40% damage within 2.5m.",
      Bonus::CannonSplash, 0.4f, 1},
     {"Siege rounds", "Cannon damage +50%, rate -15%.", Bonus::CannonHeavy,
      0.5f, 1},
     {"Deep winter", "Tower chill and freeze durations +60%.",
      Bonus::CryoDuration, 0.6f, 1},
     {"Forked current", "Tesla gains two extra chain targets.",
      Bonus::TeslaJumps, 2.0f, 1},
     {"Signal booster", "Relay aura radius +40%.", Bonus::RelayRange, 0.4f, 1},
     {"Protective signal", "Inside relay aura, player incoming damage -20%.",
      Bonus::RelayArmor, 0.2f, 1},
     {"Network dividend",
      "Extractors inside relay aura earn +20 credits per wave.",
      Bonus::RelayIncome, 20.0f, 1},
     {"Armored industry", "Extractors gain +100% base maximum health.",
      Bonus::ExtractorArmor, 1.0f, 1},
     {"Production line",
      "Each extractor gains +5 income per other extractor, max +25.",
      Bonus::ExtractorNetwork, 5.0f, 1},
     {"Spiked rampart",
      "Bulwarks gain reflection: return 50% of incoming attack damage.",
      Bonus::BulwarkThorns, 0.5f, 1},
     {"Living wall", "Bulwarks regenerate 3 HP/s.", Bonus::BulwarkMend, 3.0f,
      1},
     {"Challenge beacon", "Bulwark aggro radius +3m.", Bonus::BulwarkTaunt,
      3.0f, 1},
     {"Maintenance drones", "All towers regenerate 1.5 HP/s.",
      Bonus::TowerRegen, 1.5f, 1},
     {"Efficient assembly", "Tower upgrade prices -20%, capped at 50%.",
      Bonus::UpgradeDiscount, 0.2f, 1},
     {"Dual chambers", "Every third attacking tower hit deals +80% damage.",
      Bonus::TwinTurret, 0.8f, 1},
     {"White-hot embers", "Burn damage +50%.", Bonus::BurnPower, 0.5f, 1},
     {"Slow-burning fuel", "Burn lasts 2 seconds longer.", Bonus::BurnDuration,
      2.0f, 1},
     {"Wildfire", "Burning enemies spread ignition to nearby enemies on death.",
      Bonus::FireSpread, 1.0f, 1},
     {"Ash bomb", "Burning enemies explode on death for 20 damage within 3m.",
      Bonus::FireExplosion, 20.0f, 1},
     {"Concentrated toxin", "Poison damage +50%.", Bonus::PoisonPower, 0.5f, 1},
     {"Toxic reservoir", "Maximum poison stacks +4.", Bonus::PoisonCapacity,
      4.0f, 1},
     {"Contagion",
      "Poisoned deaths transfer their stacks to enemies within 3m.",
      Bonus::PoisonSpread, 1.0f, 1},
     {"Open wounds", "Bleed damage +50%.", Bonus::BleedPower, 0.5f, 1},
     {"Blood scent", "Critical chance +20 points against bleeding enemies.",
      Bonus::BleedCrit, 0.2f, 1},
     {"Brittle armor", "Chilled enemies take +20% direct damage.",
      Bonus::IceFragile, 0.2f, 1},
     {"Glacial shards",
      "Frozen enemies explode on death for 24 damage within 3m.",
      Bonus::IceBurst, 24.0f, 1},
     {"High voltage", "Chain lightning damage +50%.", Bonus::ShockPower, 0.5f,
      1},
     {"Arc extension", "Lightning and ricochet jump radius +2m.",
      Bonus::ShockRange, 2.0f, 1},
     {"Catalytic combustion", "Burn deals +50% damage to poisoned targets.",
      Bonus::ToxicFlame, 0.5f, 1},
     {"Cauterizing blast",
      "Hits on burning and bleeding enemies deal 12 bonus damage.",
      Bonus::BloodFire, 12.0f, 1},
     {"Ionized toxin", "Lightning deals +50% damage to poisoned targets.",
      Bonus::StormVenom, 0.5f, 1},
     {"Permafrost", "Chill lasts +2 seconds.", Bonus::Permafrost, 2.0f, 1},
     {"Prismatic core", "All elemental damage +20%.", Bonus::ElementPower, 0.2f,
      1},
     {"Affliction hunter", "Direct damage +10% per distinct active ailment.",
      Bonus::StatusHunter, 0.1f, 1},
     {"Tri-element overload",
      "Hits against burning, poisoned and chilled targets detonate for 45 area "
      "damage, 2s cooldown per enemy.",
      Bonus::Overload, 45.0f, 1}}};

} // namespace citadel::game
