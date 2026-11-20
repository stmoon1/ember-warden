#include "Localization.h"
#include "game/Game.h"
#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
using namespace citadel::game;
using Catch::Approx;
namespace {
Enemy grunt(uint32_t id = 500) {
  Enemy e;
  e.id = id;
  e.position = {8, 3, 10};
  e.hp = e.maxHp = 40;
  e.speed = 0;
  return e;
}
void tick(Game &g, float seconds) {
  for (float t = 0; t < seconds; t += .02f)
    g.update(.02f);
}
} // namespace
TEST_CASE("A run starts with only a knife and no purchasing power", "[game]") {
  Game g;
  g.reset(0);
  REQUIRE(g.resources == 0);
  REQUIRE(g.owned[0]);
  for (int i = 1; i < 7; ++i)
    REQUIRE_FALSE(g.owned[i]);
  REQUIRE_FALSE(g.buyWeapon(1));
  REQUIRE_FALSE(g.buyTower(0));
  REQUIRE(g.world.routes.size() == 3);
}
TEST_CASE("Two initial knife attacks kill a first wave grunt", "[game]") {
  Game g;
  g.reset(0);
  g.start();
  g.enemies.push_back(grunt());
  Vec3 origin{8, 4.65f, 12};
  Vec3 direction = normalized(Vec3{8, 4, 10} - origin);
  g.attack(origin, direction);
  REQUIRE(g.enemies[0].hp == Approx(20));
  g.attack(origin, direction);
  REQUIRE(g.enemies[0].hp == Approx(20));
  g.attackTimer = 0;
  g.attack(origin, direction);
  REQUIRE(g.enemies[0].hp == Approx(0));
  g.update(.02f);
  REQUIRE(g.kills == 1);
  REQUIRE(g.resources == 12);
}
TEST_CASE("A perfect first wave funds a rifle or a cannon", "[game]") {
  Game g;
  g.reset(0);
  g.start();
  g.playerPosition = {15, 4.65f, 15};
  for (int step = 0; step < 900 && g.state == State::Playing; ++step) {
    for (auto &e : g.enemies)
      e.hp = 0;
    g.update(.02f);
  }
  REQUIRE(g.state == State::Choosing);
  REQUIRE(g.resources == 130);
  REQUIRE(g.completedWaves == 1);
  REQUIRE(g.choices[0] != g.choices[1]);
  REQUIRE(g.choices[1] != g.choices[2]);
  REQUIRE(g.buyWeapon(1));
  REQUIRE(g.resources == 10);
  REQUIRE(g.ammo[1] == 30);
  REQUIRE(g.reserve[1] == 90);
  REQUIRE(g.choose(0));
  REQUIRE(g.wave == 2);
  REQUIRE_FALSE(g.choose(0));
}
TEST_CASE("Ammo purchases and reloads conserve inventory", "[game]") {
  Game g;
  g.reset(0);
  g.resources = 500;
  REQUIRE(g.buyWeapon(1));
  REQUIRE_FALSE(g.buyWeapon(1));
  g.ammo[1] = 4;
  g.reserve[1] = 10;
  g.reload();
  tick(g, 2);
  REQUIRE(g.ammo[1] == 14);
  REQUIRE(g.reserve[1] == 0);
  REQUIRE(g.buyAmmo(1));
  REQUIRE(g.reserve[1] == 90);
  REQUIRE_FALSE(g.buyAmmo(0));
  g.ammo[1] = 0;
  g.reload();
  g.equip(0);
  tick(g, 3);
  REQUIRE(g.ammo[1] == 0);
}
TEST_CASE("Blueprints are paid once and tower upgrades retain damage",
          "[game]") {
  Game g;
  g.reset(0);
  g.resources = 500;
  REQUIRE(g.buyTower(4));
  REQUIRE(g.resources == 415);
  REQUIRE(g.place(11, 8));
  REQUIRE(g.resources == 415);
  REQUIRE(g.blueprints[4] == 0);
  REQUIRE(g.towers[0].hp == 650);
  REQUIRE_FALSE(g.place(11, 8));
  g.towers[0].hp -= 100;
  REQUIRE(g.upgradeTower(0));
  REQUIRE(g.towers[0].maxHp == 975);
  REQUIRE(g.towers[0].hp == 875);
  REQUIRE(g.upgradeTower(0));
  REQUIRE_FALSE(g.upgradeTower(0));
}
TEST_CASE("All four tower roles can be attacked and defensive towers taunt",
          "[game]") {
  for (int type : {0, 2, 3, 4}) {
    Game g;
    g.reset(0);
    g.resources = 500;
    g.buyTower(type);
    REQUIRE(g.place(11, 8));
    g.start();
    auto e = grunt();
    e.position = {11.5f, 3, 7};
    g.enemies.push_back(e);
    float hp = g.towers[0].hp;
    g.update(.02f);
    REQUIRE(g.towers[0].hp < hp);
  }
}
TEST_CASE("Wave income and repair apply once per clear", "[game]") {
  Game g;
  g.reset(0);
  g.resources = 500;
  g.buyTower(3);
  g.place(11, 8);
  g.towers[0].hp = 50;
  g.perkStacks[int(Perk::Repair)] = 1;
  int before = g.resources;
  g.finishWave();
  REQUIRE(g.resources == before + 58 + 35);
  REQUIRE(g.towers[0].hp == 68);
  int after = g.resources;
  tick(g, 2);
  REQUIRE(g.resources == after);
}
TEST_CASE("Elements combine and armor piercing bypasses armor", "[game]") {
  Game g;
  g.reset(0);
  auto e = grunt();
  e.kind = 2;
  e.hp = e.maxHp = 500;
  g.enemies.push_back(e);
  g.damageEnemy(g.enemies[0], 20, false, false);
  REQUIRE(g.enemies[0].hp == 489);
  g.perkStacks[int(Perk::Piercing)] = 1;
  g.damageEnemy(g.enemies[0], 20, false, false);
  REQUIRE(g.enemies[0].hp == 469);
  g.perkStacks[int(Perk::Incendiary)] = 1;
  g.perkStacks[int(Perk::CryoRounds)] = 1;
  g.perkStacks[int(Perk::Volatile)] = 1;
  g.damageEnemy(g.enemies[0], 10, false, false);
  REQUIRE(g.enemies[0].hp == 429);
  REQUIRE(g.enemies[0].burn == 0);
}
TEST_CASE("Every fifth wave has exactly one boss and difficulty rises",
          "[game]") {
  for (int wave : {5, 10, 15}) {
    Game g;
    g.reset(1);
    g.wave = wave;
    g.health = 1000;
    g.playerHp = g.maxPlayerHp = 10000;
    g.playerPosition = {15, 4.65f, 15};
    g.start();
    tick(g, 40);
    int bosses = 0;
    for (const auto &e : g.enemies)
      if (e.kind == 8)
        ++bosses;
    REQUIRE(g.spawnedInWave == g.currentWaveSize());
    REQUIRE(bosses == 1);
  }
}
TEST_CASE("Loss and reset clear the complete run state", "[game]") {
  Game g;
  g.reset(0);
  g.resources = 500;
  g.buyWeapon(2);
  g.perkStacks[int(Perk::Shock)] = 3;
  g.start();
  g.playerHp = 0;
  g.update(.02f);
  REQUIRE(g.state == State::Lost);
  g.reset(1);
  REQUIRE(g.resources == 0);
  REQUIRE(g.playerHp == 100);
  REQUIRE_FALSE(g.owned[2]);
  REQUIRE(g.perk(Perk::Shock) == 0);
  REQUIRE(g.enemies.empty());
  REQUIRE(g.towers.empty());
}

TEST_CASE("Clearing a wave cannot pay out twice", "[game]") {
  Game g;
  g.reset(0);
  g.finishWave();
  int credits = g.resources;
  g.finishWave();
  REQUIRE(g.resources == credits);
}
TEST_CASE("Boss abilities summon reinforcements and apply siege damage",
          "[game]") {
  Game g;
  g.reset(0);
  g.start();
  g.wave = 10;
  auto boss = grunt();
  boss.kind = 8;
  boss.hp = boss.maxHp = 10000;
  boss.abilityClock = 0;
  g.enemies.push_back(boss);
  g.update(.02f);
  int children = 0;
  for (const auto &e : g.enemies)
    if (e.kind == 1)
      ++children;
  REQUIRE(children == 2);
  g.reset(0);
  g.start();
  g.wave = 5;
  g.playerPosition = {8, 4.65f, 12};
  boss.position = {8, 3, 10};
  boss.abilityClock = 0;
  g.enemies.push_back(boss);
  g.update(.02f);
  REQUIRE(g.playerHp < 100);
}

TEST_CASE("The upgrade catalogue contains 128 distinct usable augments",
          "[upgrades]") {
  REQUIRE(Perks.size() == 128);
  for (size_t i = 0; i < Perks.size(); ++i) {
    REQUIRE(std::string(Perks[i].name).size() > 3);
    REQUIRE(std::string(Perks[i].description).size() > 10);
    for (size_t j = i + 1; j < Perks.size(); ++j)
      REQUIRE(std::string(Perks[i].name) != Perks[j].name);
    Game g;
    g.reset(0);
    g.acquire(int(i));
    REQUIRE(g.perkStacks[i] == 1);
    if (Perks[i].bonus != Bonus::None)
      REQUIRE(g.bonus(Perks[i].bonus) == Approx(Perks[i].amount));
  }
}
TEST_CASE("Magazine and economy augments affect normal purchase entry points",
          "[upgrades]") {
  Game g;
  g.reset(0);
  g.resources = 1000;
  g.acquire(int(Perk::LargeMagazine));
  REQUIRE(g.magazine(1) == 39);
  g.acquire(int(Perk::Discount));
  REQUIRE(g.weaponPrice(1) == 102);
  REQUIRE(g.buyWeapon(1));
  REQUIRE(g.ammo[1] == 39);
  REQUIRE(g.reserve[1] == 117);
  REQUIRE(g.resources == 898);
  g.acquire(int(Perk::TowerDiscount));
  REQUIRE(g.towerPrice(0) == 80); // Golden Industry 2-piece adds a 5% discount.
}
TEST_CASE("Tower health augments update existing and newly deployed structures",
          "[upgrades]") {
  Game g;
  g.reset(0);
  g.resources = 1000;
  g.buyTower(4);
  g.place(11, 8);
  g.towers[0].hp -= 100;
  g.acquire(int(Perk::TowerArmor));
  REQUIRE(g.towers[0].maxHp == Approx(845));
  REQUIRE(g.towers[0].hp == Approx(745));
  g.buyTower(4);
  REQUIRE(g.place(10, 8));
  REQUIRE(g.towers[1].maxHp == Approx(845));
}
TEST_CASE("Damage feedback and knockback are tied to actual hits",
          "[feedback]") {
  Game g;
  g.reset(0);
  g.start();
  g.enemies.push_back(grunt());
  g.damageEnemy(g.enemies[0], 10, false, true);
  REQUIRE(g.enemies[0].flash > 0);
  REQUIRE(g.enemies[0].stagger > 0);
  REQUIRE(length(g.enemies[0].impulse) > 0);
  REQUIRE(g.damageNumbers.size() == 1);
  REQUIRE(g.damageNumbers[0].damage == 10);
  REQUIRE(g.hitStop > 0);
}
TEST_CASE("Critical augments and near death survival change combat",
          "[upgrades]") {
  Game g;
  g.reset(0);
  g.start();
  g.playerHp = 5;
  g.acquire(int(Perk::Emergency));
  g.hurtPlayer(20);
  REQUIRE(g.playerHp == 35);
  REQUIRE(g.emergencyUsed);
  g.hurtPlayer(100);
  REQUIRE(g.playerHp < 0);
  g.reset(0);
  g.start();
  g.enemies.push_back(grunt());
  g.acquire(int(Perk::FirstBlood));
  g.damageEnemy(g.enemies[0], 20, false, false);
  REQUIRE(g.enemies[0].hp == Approx(13));
}
TEST_CASE("Rerolls consume charges and skip capped upgrades", "[upgrades]") {
  Game g;
  g.reset(0);
  g.finishWave();
  for (int i = 28; i < 128; ++i)
    g.perkStacks[i] = Perks[i].maxStacks;
  REQUIRE(g.reroll());
  for (int id : g.choices)
    REQUIRE(id < 28);
  REQUIRE(g.reroll());
  REQUIRE_FALSE(g.reroll());
  REQUIRE(g.rerolls == 0);
}
TEST_CASE("Death explosions resolve cascading kills exactly once",
          "[upgrades]") {
  Game g;
  g.reset(0);
  g.start();
  g.acquire(int(Perk::FireExplosion));
  auto first = grunt(500);
  first.hp = 0;
  first.burn = 2;
  auto second = grunt(501);
  second.hp = 10;
  second.position.x += 1;
  second.burn = 2;
  g.enemies = {first, second};
  g.update(.02f);
  REQUIRE(g.kills == 2);
  int income = g.resources;
  g.update(.02f);
  REQUIRE(g.resources == income);
}

TEST_CASE("Fifty waves end in victory and never repeat a selected augment",
          "[campaign]") {
  Game g;
  g.reset(0);
  std::array<bool, 128> collected{};
  for (int wave = 1; wave <= Game::FinalWave; ++wave) {
    REQUIRE(g.wave == wave);
    g.finishWave();
    for (int id : g.choices) {
      REQUIRE(id >= 0);
      REQUIRE_FALSE(collected[id]);
    }
    int chosen = g.choices[0];
    collected[chosen] = true;
    REQUIRE(g.choose(0));
  }
  REQUIRE(g.state == State::Won);
  REQUIRE(g.wave == 50);
  REQUIRE(g.completedWaves == 50);
  REQUIRE(g.remainingAugments() == 78);
  int income = g.resources;
  g.update(.25f);
  g.finishWave();
  g.start();
  REQUIRE(g.state == State::Won);
  REQUIRE(g.resources == income);
}
TEST_CASE(
    "Unseen eligible augments remain valid as the collection is exhausted",
    "[upgrades]") {
  Game g;
  g.reset(0);
  for (int i = 3; i < 128; ++i)
    g.acquire(i);
  g.finishWave();
  REQUIRE(g.availableChoices() == 3);
  REQUIRE(g.choose(0));
  g.finishWave();
  REQUIRE(g.availableChoices() == 2);
  REQUIRE(g.choices[2] == -1);
  REQUIRE_FALSE(g.choose(2));
  REQUIRE(g.choose(0));
  g.finishWave();
  REQUIRE(g.availableChoices() == 1);
  REQUIRE(g.choices[1] == -1);
  REQUIRE(g.choices[2] == -1);
  REQUIRE(g.choose(0));
  g.finishWave();
  REQUIRE(g.remainingAugments() == 0);
  REQUIRE(g.availableChoices() == 0);
  REQUIRE_FALSE(g.reroll());
  REQUIRE(g.continueWithoutUpgrade());
}
TEST_CASE("Reacquiring an owned augment cannot duplicate stats or set pieces",
          "[sets]") {
  Game g;
  g.reset(0);
  g.acquire(int(Perk::BurnPower));
  g.acquire(int(Perk::BurnPower));
  REQUIRE(g.perk(Perk::BurnPower) == 1);
  REQUIRE(g.setPieces(0) == 1);
  REQUIRE(g.bonus(Bonus::BurnPower) == Approx(.5f));
}
TEST_CASE("Ember Covenant activates cumulative bonuses at 2 4 and 6 pieces",
          "[sets]") {
  Game g;
  g.reset(0);
  g.acquire(int(Perk::Incendiary));
  REQUIRE(g.bonus(Bonus::BurnPower) == 0);
  g.acquire(int(Perk::BurnPower));
  REQUIRE(g.setPieces(0) == 2);
  REQUIRE(g.bonus(Bonus::BurnPower) == Approx(.75f));
  g.acquire(int(Perk::TowerFire));
  g.acquire(int(Perk::BurnDuration));
  REQUIRE(g.bonus(Bonus::BurnDuration) == 3);
  REQUIRE(g.bonus(Bonus::FireExplosion) == 10);
  g.acquire(int(Perk::FireSpread));
  g.acquire(int(Perk::FireExplosion));
  REQUIRE(g.setPieces(0) == 6);
  REQUIRE(g.bonus(Bonus::BurnPower) == Approx(1.25f));
  REQUIRE(g.bonus(Bonus::FireSpread) == 2);
  REQUIRE(g.bonus(Bonus::FireExplosion) == 30);
}
TEST_CASE(
    "Tower set health is applied immediately while preserving existing damage",
    "[sets]") {
  Game g;
  g.reset(0);
  g.resources = 1000;
  g.buyTower(0);
  g.place(11, 8);
  g.towers[0].hp -= 30;
  g.acquire(int(Perk::Engineering));
  g.acquire(int(Perk::TowerArmor));
  REQUIRE(g.towers[0].maxHp == Approx(261));
  REQUIRE(g.towers[0].hp == Approx(231));
  g.buyTower(0);
  REQUIRE(g.place(10, 8));
  REQUIRE(g.towers[1].maxHp == Approx(261));
}
TEST_CASE("A single augment may advance multiple sets without double counting",
          "[sets]") {
  Game g;
  g.reset(0);
  g.acquire(int(Perk::Melee));
  REQUIRE(g.setPieces(4) == 1);
  REQUIRE(g.setPieces(6) == 1);
  g.acquire(int(Perk::KnifeSpeed));
  REQUIRE(g.setPieces(6) == 2);
  REQUIRE(g.bonus(Bonus::MeleeReach) == Approx(.3f));
  REQUIRE(g.setPieces(4) == 1);
  g.reset(0);
  REQUIRE(g.setPieces(4) == 0);
  REQUIRE(g.bonus(Bonus::MeleeReach) == 0);
}
TEST_CASE("Every set has eight different members and valid progressive tiers",
          "[sets]") {
  for (size_t i = 0; i < AugmentSets.size(); ++i) {
    const auto &set = AugmentSets[i];
    Game g;
    g.reset(0);
    for (size_t a = 0; a < set.members.size(); ++a) {
      for (size_t b = a + 1; b < set.members.size(); ++b)
        REQUIRE(set.members[a] != set.members[b]);
      g.acquire(int(set.members[a]));
      REQUIRE(g.setPieces(i) == int(a) + 1);
    }
    REQUIRE(set.tiers[0].pieces == 2);
    REQUIRE(set.tiers[1].pieces == 4);
    REQUIRE(set.tiers[2].pieces == 6);
    for (const auto &tier : set.tiers)
      for (const auto &reward : tier.modifiers)
        if (reward.stat != Bonus::None)
          REQUIRE(g.bonus(reward.stat) != 0);
  }
}

TEST_CASE(
    "Set-enhanced fire spread does not incorrectly extend poison transfer",
    "[sets]") {
  Game g;
  g.reset(0);
  g.start();
  for (auto member : std::array<Perk, 6>{
           {Perk::Incendiary, Perk::TowerFire, Perk::BurnPower,
            Perk::BurnDuration, Perk::FireSpread, Perk::FireExplosion}})
    g.acquire(int(member));
  g.acquire(int(Perk::PoisonSpread));
  auto first = grunt(500);
  first.hp = 0;
  first.burn = 2;
  first.burnDps = 6;
  first.poison = 2;
  first.poisonStacks = 1;
  auto second = grunt(501);
  second.position.x += 3.5f;
  g.enemies = {first, second};
  g.update(.02f);
  auto target = std::find_if(g.enemies.begin(), g.enemies.end(),
                             [](const Enemy &e) { return e.id == 501; });
  REQUIRE(target != g.enemies.end());
  REQUIRE(target->burn > 0);
  REQUIRE(target->poison == 0);
}
TEST_CASE("Winter set duration bonuses reach innate cryo turret attacks",
          "[sets]") {
  Game g;
  g.reset(0);
  g.resources = 1000;
  g.buyTower(1);
  g.place(11, 8);
  g.start();
  for (auto member : std::array<Perk, 6>{{Perk::CryoRounds, Perk::TowerFrost,
                                          Perk::Permafrost, Perk::CryoDuration,
                                          Perk::IceFragile, Perk::IceBurst}})
    g.acquire(int(member));
  auto e = grunt(500);
  e.position = {11.5f, 3, 7};
  g.enemies.push_back(e);
  g.update(.02f);
  auto target = std::find_if(g.enemies.begin(), g.enemies.end(),
                             [](const Enemy &e) { return e.id == 500; });
  REQUIRE(target != g.enemies.end());
  REQUIRE(target->slow == Approx(11.4f));
}

TEST_CASE("Every boss draft offers three distinct unseen golds even with "
          "greedy gold selection",
          "[rarity]") {
  for (unsigned seed = 1; seed <= 64; ++seed) {
    Game g;
    g.runSeed = seed;
    g.reset(0);
    for (int wave = 1; wave <= 50; ++wave) {
      g.finishWave();
      REQUIRE(g.availableChoices() == 3);
      if (wave % 5 == 0) {
        for (int id : g.choices)
          REQUIRE(perkRarity(Perk(id)) == Rarity::Gold);
        if (g.rerolls > 0) {
          REQUIRE(g.reroll());
          for (int id : g.choices)
            REQUIRE(perkRarity(Perk(id)) == Rarity::Gold);
        }
      }
      int selected = 0;
      for (int i = 0; i < 3; ++i)
        if (perkRarity(Perk(g.choices[i])) == Rarity::Gold)
          selected = i;
      REQUIRE(g.perkStacks[g.choices[selected]] == 0);
      REQUIRE(g.choices[0] != g.choices[1]);
      REQUIRE(g.choices[1] != g.choices[2]);
      REQUIRE(g.choices[0] != g.choices[2]);
      REQUIRE(g.choose(selected));
    }
    REQUIRE(g.state == State::Won);
    REQUIRE(g.wave == 50);
  }
}
TEST_CASE(
    "Normal draft rarity distribution follows the configured base weights",
    "[rarity]") {
  Game g;
  g.reset(0);
  std::array<int, 3> count{};
  for (int sample = 0; sample < 20000; ++sample) {
    g.rollChoices();
    for (int id : g.choices) {
      REQUIRE(id >= 0);
      ++count[int(perkRarity(Perk(id)))];
    }
  }
  for (int tier = 0; tier < 3; ++tier) {
    float probability = count[tier] / 60000.f;
    REQUIRE(probability == Approx(RarityWeights[tier] / 100.f).margin(.015f));
  }
}
TEST_CASE("Final boss reward is claimed before the victory screen",
          "[campaign]") {
  Game g;
  g.reset(0);
  g.wave = 50;
  g.finishWave();
  REQUIRE(g.state == State::Choosing);
  REQUIRE(g.completedWaves == 50);
  for (int id : g.choices)
    REQUIRE(perkRarity(Perk(id)) == Rarity::Gold);
  REQUIRE(g.choose(0));
  REQUIRE(g.state == State::Won);
  REQUIRE(g.wave == 50);
  REQUIRE_FALSE(g.choose(0));
  REQUIRE_FALSE(g.reroll());
  REQUIRE_FALSE(g.buyWeapon(1));
  REQUIRE_FALSE(g.buyTower(0));
  REQUIRE_FALSE(g.place(11, 8));
  g.endRun();
  REQUIRE(g.state == State::Won);
}
TEST_CASE("Simplified Chinese English and Japanese cover every augment and set",
          "[localization]") {
  namespace l = citadel::locale;
  l::load(std::filesystem::path(CITADEL_TEST_ASSET_DIR) /
          "i18n/translations.tsv");
  for (auto language : {l::Language::Chinese, l::Language::Japanese}) {
    l::setLanguage(language);
    for (const auto &perk : Perks) {
      REQUIRE(std::string(l::tr(perk.name)) != perk.name);
      REQUIRE(std::string(l::tr(perk.description)) != perk.description);
    }
    for (const auto &set : AugmentSets) {
      REQUIRE(std::string(l::tr(set.name)) != set.name);
      for (const auto &tier : set.tiers)
        REQUIRE(std::string(l::tr(tier.description)) != tier.description);
    }
  }
  l::setLanguage(l::Language::English);
  REQUIRE(std::string(l::tr("Warden")) == "Warden");
  l::setLanguage(l::Language::Chinese);
  REQUIRE(std::string(l::tr("Warden")) == "守望者");
  auto message = l::message("Augment installed. TAB supplies / SPACE next "
                            "wave. [Ember Covenant 2pc activated]");
  REQUIRE(message.find("Ember Covenant") == std::string::npos);
  REQUIRE(message.find("烈焰契约") != std::string::npos);
}

TEST_CASE(
    "Actor ray bounds cover tall boss heads and reject shots over hunters",
    "[visual-regression]") {
  Enemy boss;
  boss.kind = 8;
  boss.position = {8, 3, 5};
  float distance = 0;
  Vec3 origin{8, 4.65f, 12};
  REQUIRE(boss.rayHit(origin,
                      normalized(boss.position + Vec3{0, 3.4f, 0} - origin), 30,
                      distance));
  REQUIRE(distance > 0);
  REQUIRE(distance < 10);
  Enemy hunter;
  hunter.kind = 1;
  hunter.position = {8, 3, 5};
  REQUIRE_FALSE(hunter.rayHit({8, 4.8f, 12}, {0, 0, -1}, 30, distance));
  REQUIRE(hunter.rayHit({8, 4, 12}, {0, 0, -1}, 30, distance));
  REQUIRE_FALSE(hunter.rayHit({12, 4, 12}, {0, 0, -1}, 30, distance));
}
TEST_CASE("Aiming at the visible boss head lands a normal sniper hit",
          "[visual-regression]") {
  Game g;
  g.reset(0);
  g.resources = 1000;
  g.buyWeapon(3);
  g.start();
  auto boss = grunt(500);
  boss.kind = 8;
  boss.position = {8, 3, 5};
  boss.hp = boss.maxHp = 500;
  g.enemies.push_back(boss);
  Vec3 origin{8, 4.65f, 12};
  g.attack(origin, normalized(boss.position + Vec3{0, 3.4f, 0} - origin));
  REQUIRE(g.enemies[0].hp == Approx(375));
}
TEST_CASE("Walking actors turn into their route and stop striding when frozen",
          "[animation]") {
  Game g;
  g.reset(0);
  g.start();
  auto enemy = grunt(500);
  enemy.position = g.world.routes[0][0];
  enemy.speed = 1;
  g.enemies.push_back(enemy);
  g.update(.1f);
  auto actor = std::find_if(g.enemies.begin(), g.enemies.end(),
                            [](const Enemy &e) { return e.id == 500; });
  REQUIRE(actor != g.enemies.end());
  REQUIRE(actor->yaw > 0);
  REQUIRE(actor->stride > 0);
  Vec3 before = actor->position;
  actor->freeze = 2;
  g.update(.2f);
  actor = std::find_if(g.enemies.begin(), g.enemies.end(),
                       [](const Enemy &e) { return e.id == 500; });
  REQUIRE(actor->position.x == Approx(before.x));
  REQUIRE(actor->position.z == Approx(before.z));
  REQUIRE(actor->stride == 0);
}
TEST_CASE("Equipment belt cycles only usable weapons and tower blueprints",
          "[equipment]") {
  Game g;
  g.reset(0);
  REQUIRE_FALSE(g.equipTower(0));
  g.resources = 1000;
  REQUIRE(g.buyWeapon(1));
  REQUIRE(g.buyTower(2));
  g.equip(0);
  g.cycleEquipment(1);
  REQUIRE(g.equipped == 1);
  REQUIRE(g.heldTower == -1);
  g.cycleEquipment(1);
  REQUIRE(g.heldTower == 2);
  REQUIRE(g.selectedType == 2);
  g.cycleEquipment(1);
  REQUIRE(g.equipped == 0);
  REQUIRE(g.heldTower == -1);
  g.cycleEquipment(-1);
  REQUIRE(g.heldTower == 2);
  g.equip(6);
  REQUIRE(g.heldTower == 2);
  g.equip(0);
  REQUIRE(g.heldTower == -1);
}
TEST_CASE("Holding a blueprint blocks weapon fire and reload", "[equipment]") {
  Game g;
  g.reset(0);
  g.resources = 1000;
  REQUIRE(g.buyWeapon(1));
  REQUIRE(g.buyTower(0));
  REQUIRE(g.equipTower(0));
  g.start();
  int ammo = g.ammo[1];
  g.attack(g.playerPosition, {0, 0, -1});
  REQUIRE(g.shots == 0);
  REQUIRE(g.ammo[1] == ammo);
  g.ammo[1] = 0;
  g.reload();
  REQUIRE(g.reloadTimer == 0);
  g.equip(1);
  REQUIRE(g.heldTower == -1);
  g.reload();
  REQUIRE(g.reloadTimer > 0);
  REQUIRE(g.equipTower(0));
  REQUIRE(g.reloadTimer == 0);
}
TEST_CASE("Using the final held blueprint returns to the previous weapon",
          "[equipment]") {
  Game g;
  g.reset(0);
  g.resources = 1000;
  REQUIRE(g.buyWeapon(1));
  REQUIRE(g.buyTower(0));
  REQUIRE(g.equipTower(0));
  bool placed = false;
  for (int z = 4; z < 27 && !placed; ++z)
    for (int x = 4; x < 27 && !placed; ++x)
      if (g.placementIssue(x, z).empty())
        placed = g.place(x, z);
  REQUIRE(placed);
  REQUIRE(g.blueprints[0] == 0);
  REQUIRE(g.heldTower == -1);
  REQUIRE(g.equipped == 1);
  g.reset(0);
  REQUIRE(g.heldTower == -1);
  REQUIRE(g.swingTimer == 0);
}
TEST_CASE("Element follow-ups stay locked until matching triggers exist",
          "[prerequisites]") {
  Game g;
  g.reset(0);
  for (Perk p : {Perk::Incendiary, Perk::CryoRounds, Perk::Shock, Perk::Venom,
                 Perk::Bleed, Perk::Piercing, Perk::TowerFire, Perk::TowerFrost,
                 Perk::TowerShock, Perk::TowerPoison})
    REQUIRE(g.augmentEligible(int(p)));
  for (Perk p : {Perk::BurnPower, Perk::BurnDuration, Perk::IceBurst,
                 Perk::ShockPower, Perk::PoisonSpread, Perk::BleedCrit,
                 Perk::SniperRail, Perk::ElementMaster, Perk::StatusHunter,
                 Perk::Volatile, Perk::Overload})
    REQUIRE_FALSE(g.augmentEligible(int(p)));
  for (int draw = 0; draw < 500; ++draw) {
    g.rollChoices();
    for (int id : g.choices) {
      REQUIRE(g.augmentEligible(id));
      REQUIRE(requiredElements(Perk(id)) == 0);
      REQUIRE_FALSE(requiresAnyAilment(Perk(id)));
    }
  }
  g.acquire(int(Perk::Incendiary));
  REQUIRE(g.augmentEligible(int(Perk::BurnPower)));
  REQUIRE_FALSE(g.augmentEligible(int(Perk::Volatile)));
  g.acquire(int(Perk::CryoRounds));
  REQUIRE(g.augmentEligible(int(Perk::Volatile)));
  REQUIRE_FALSE(g.augmentEligible(int(Perk::Overload)));
  g.acquire(int(Perk::Venom));
  REQUIRE(g.augmentEligible(int(Perk::Overload)));
  g.acquire(int(Perk::BurnPower));
  REQUIRE_FALSE(g.augmentEligible(int(Perk::BurnPower)));
}
TEST_CASE(
    "Tower blueprints unlock innate elements and source-specific upgrades",
    "[prerequisites]") {
  Game g;
  g.reset(0);
  g.resources = 2000;
  REQUIRE(g.buyTower(1));
  REQUIRE(g.augmentEligible(int(Perk::IceFragile)));
  REQUIRE(g.augmentEligible(int(Perk::CryoDuration)));
  REQUIRE_FALSE(g.augmentEligible(int(Perk::TeslaJumps)));
  REQUIRE(g.buyTower(5));
  REQUIRE(g.augmentEligible(int(Perk::TeslaJumps)));
  REQUIRE(g.augmentEligible(int(Perk::Conductor)));
  g.blueprints[1] = g.blueprints[5] = 0;
  REQUIRE_FALSE(g.augmentEligible(int(Perk::IceFragile)));
  REQUIRE_FALSE(g.augmentEligible(int(Perk::TeslaJumps)));
  Tower tower;
  tower.type = 5;
  g.towers.push_back(tower);
  REQUIRE(g.augmentEligible(int(Perk::ShockPower)));
  g.towers[0].hp = 0;
  REQUIRE_FALSE(g.augmentEligible(int(Perk::ShockPower)));
  g.acquire(int(Perk::Shock));
  REQUIRE(g.augmentEligible(int(Perk::ShockPower)));
  REQUIRE_FALSE(g.augmentEligible(int(Perk::StatusHunter)));
  g.acquire(int(Perk::Ricochet));
  REQUIRE(g.augmentEligible(int(Perk::ShockRange)));
}
namespace {
void combatArena(Game &g) {
  g.reset(0);
  for (int z = 4; z < 21; ++z)
    for (int x = 4; x < 21; ++x) {
      g.world.set(x, 2, z, Grass);
      for (int y = 3; y < 12; ++y)
        g.world.set(x, y, z, Air);
    }
  g.start();
  g.spawnedInWave = g.currentWaveSize();
}
} // namespace
TEST_CASE("Nearby visible players are pursued and can be reached for attacks",
          "[aggro]") {
  Game g;
  combatArena(g);
  auto enemy = grunt();
  enemy.position = {8, 3, 7};
  enemy.speed = 2;
  g.enemies.push_back(enemy);
  float before =
      length(g.playerPosition - (enemy.position + Vec3{0, 1.65f, 0}));
  tick(g, .2f);
  REQUIRE(length(g.playerPosition -
                 (g.enemies[0].position + Vec3{0, 1.65f, 0})) < before);
  tick(g, 2);
  REQUIRE(g.playerHp < g.maxPlayerHp);
}
TEST_CASE("Player pursuit respects line of sight and defensive tower taunts",
          "[aggro]") {
  Game g;
  combatArena(g);
  g.world.routes[0] = {{8, 3, 6}, {8, 3, 5}};
  for (int x = 7; x <= 9; ++x)
    for (int y = 3; y < 8; ++y)
      g.world.set(x, y, 9, Stone);
  auto enemy = grunt();
  enemy.position = {8, 3, 7};
  enemy.speed = 2;
  g.enemies.push_back(enemy);
  tick(g, .2f);
  REQUIRE(g.enemies[0].position.z < 7);
  combatArena(g);
  g.playerPosition = {10, 4.65f, 13};
  Tower wall;
  wall.type = 4;
  wall.position = {11.5f, 3, 10.5f};
  wall.hp = wall.maxHp = 650;
  g.towers.push_back(wall);
  enemy = grunt();
  enemy.position = {10, 3, 10};
  g.enemies.push_back(enemy);
  g.update(.02f);
  REQUIRE(g.towers[0].hp < 650);
  REQUIRE(g.playerHp == g.maxPlayerHp);
}
TEST_CASE("Bulwark reflection is granted only by selected augments",
          "[balance]") {
  Game g;
  combatArena(g);
  g.playerPosition = {18, 4.65f, 18};
  Tower wall;
  wall.type = 4;
  wall.position = {11.5f, 3, 10.5f};
  wall.hp = wall.maxHp = 650;
  g.towers.push_back(wall);
  auto enemy = grunt();
  enemy.position = {10, 3, 10};
  enemy.hp = enemy.maxHp = 1000;
  g.enemies.push_back(enemy);
  g.update(.02f);
  REQUIRE(g.towers[0].hp == Approx(643));
  REQUIRE(g.enemies[0].hp == 1000);
  g.acquire(int(Perk::BulwarkThorns));
  g.enemies[0].attackClock = 0;
  g.update(.02f);
  REQUIRE(g.enemies[0].hp == Approx(996.5f));
  g.acquire(int(Perk::Thorns));
  g.enemies[0].attackClock = 0;
  g.update(.02f);
  REQUIRE(g.enemies[0].hp == Approx(990.2f));
  REQUIRE(TowerSpecs[4].damage == 0);
  REQUIRE(TowerSpecs[3].hp == 90);
  REQUIRE(g.towerMaxHp(3, 2) == 135);
  REQUIRE(g.towerMaxHp(3, 3) == 180);
}
TEST_CASE("Losing elemental towers cannot remove three-gold boss rewards",
          "[prerequisites][rarity]") {
  Game g;
  g.reset(0);
  for (int wave = 1; wave <= 50; ++wave) {
    g.blueprints[1] = wave % 2 ? 1 : 0;
    g.blueprints[5] = wave % 2 ? 1 : 0;
    g.finishWave();
    REQUIRE(g.availableChoices() == 3);
    int selection = 0;
    for (int slot = 0; slot < 3; ++slot) {
      int id = g.choices[slot];
      REQUIRE(g.augmentEligible(id));
      if (g.bossReward())
        REQUIRE(perkRarity(Perk(id)) == Rarity::Gold);
      if (perkRarity(Perk(id)) == Rarity::Gold)
        selection = slot;
    }
    REQUIRE(g.choose(selection));
  }
  REQUIRE(g.state == State::Won);
}
TEST_CASE("Dismantling refunds actual discounted investment once",
          "[salvage]") {
  Game g;
  g.reset(0);
  g.resources = 1000;
  g.acquire(int(Perk::TowerDiscount));
  int base = g.towerPrice(0);
  REQUIRE(g.buyTower(0));
  REQUIRE(g.place(11, 8));
  g.acquire(int(Perk::UpgradeDiscount));
  int upgrade = g.upgradePrice(0);
  REQUIRE(g.upgradeTower(0));
  g.towers[0].hp -= 80;
  REQUIRE(g.repairTower(0));
  int expected = (base + upgrade) * 60 / 100;
  REQUIRE(g.dismantleRefund(0) == expected);
  int before = g.resources;
  REQUIRE(g.dismantleTower(0));
  REQUIRE(g.resources == before + expected);
  REQUIRE(g.towers.empty());
  REQUIRE_FALSE(g.dismantleTower(0));
  REQUIRE(g.resources == before + expected);
  g.blueprints[0] = 1;
  REQUIRE(g.place(11, 8));
  REQUIRE(g.dismantleRefund(0) == 0);
  g.towers[0].hp = 0;
  REQUIRE_FALSE(g.dismantleTower(0));
}
TEST_CASE("Resource points are reachable, distinct and paid once per wave",
          "[exploration]") {
  for (int map = 0; map < 2; ++map) {
    Game g;
    g.reset(map);
    REQUIRE(g.resourcePoints.size() == 8);
    REQUIRE_FALSE(g.resourcePointsAvailable());
    REQUIRE(std::none_of(g.resourcePoints.begin(), g.resourcePoints.end(),
                         [](const auto &p) { return p.active; }));
    g.start();
    REQUIRE_FALSE(g.resourcePointsAvailable());
    g.update(.02f);
    REQUIRE(g.resourcePointsAvailable());
    int active = 0, gold = 0;
    size_t target = 0;
    for (size_t i = 0; i < g.resourcePoints.size(); ++i) {
      const auto &point = g.resourcePoints[i];
      REQUIRE(g.world.buildable(int(point.position.x), int(point.position.z)));
      for (size_t j = 0; j < i; ++j)
        REQUIRE(length(point.position - g.resourcePoints[j].position) >= 3);
      if (point.active) {
        ++active;
        if (point.kind == PickupKind::Gold) {
          ++gold;
          target = i;
        }
      }
    }
    REQUIRE(active == 3);
    REQUIRE(gold == 2);
    REQUIRE(g.resources == 0);
    REQUIRE_FALSE(g.collectResourcePoint(target));
    g.playerPosition = g.resourcePoints[target].position + Vec3{0, 1.65f, 0};
    REQUIRE(g.nearbyResourcePoint() == int(target));
    REQUIRE(g.collectResourcePoint(target));
    REQUIRE(g.resources == 20);
    REQUIRE_FALSE(g.collectResourcePoint(target));
    REQUIRE(g.resources == 20);
    g.finishWave();
    REQUIRE(g.choose(0));
    REQUIRE(g.resourceGoldValue() == 22);
    REQUIRE(std::count_if(g.resourcePoints.begin(), g.resourcePoints.end(),
                          [](const auto &p) { return p.active; }) == 0);
  }
}
TEST_CASE("Shrine buffs affect combat and expire without stacking strength",
          "[exploration]") {
  Game g;
  g.reset(0);
  g.start();
  auto sentinel = grunt();
  sentinel.hp = sentinel.maxHp = 100000;
  g.enemies.push_back(sentinel);
  g.spawnedInWave = g.currentWaveSize();
  for (auto kind : {PickupKind::Fury, PickupKind::Haste, PickupKind::Ward}) {
    auto found = std::find_if(g.resourcePoints.begin(), g.resourcePoints.end(),
                              [&](const auto &p) { return p.kind == kind; });
    REQUIRE(found != g.resourcePoints.end());
    found->active = true;
    g.playerPosition = found->position + Vec3{0, 1.65f, 0};
    REQUIRE(g.collectResourcePoint(size_t(found - g.resourcePoints.begin())));
  }
  REQUIRE(g.bonus(Bonus::MoveSpeed) == Approx(.2f));
  auto enemy = grunt();
  enemy.hp = enemy.maxHp = 100;
  g.damageEnemy(enemy, 20, false, false);
  REQUIRE(enemy.hp == Approx(75));
  g.hurtPlayer(10);
  REQUIRE(g.playerHp == Approx(92));
  tick(g, 21);
  REQUIRE(g.furyTime == 0);
  REQUIRE(g.hasteTime == 0);
  REQUIRE(g.wardTime == 0);
  REQUIRE(g.bonus(Bonus::MoveSpeed) == 0);
}
TEST_CASE(
    "Crystal runners ignore players and Bulwarks and damage only the core",
    "[specialists]") {
  Game g;
  combatArena(g);
  g.playerPosition = {8, 4.65f, 12};
  g.world.routes[0] = {{8, 3, 11}, {8, 3, 13}, {10, 3, 13}};
  Tower wall;
  wall.type = 4;
  wall.position = {8.5f, 3, 11};
  wall.hp = wall.maxHp = 650;
  g.towers.push_back(wall);
  auto enemy = grunt();
  enemy.kind = 9;
  enemy.position = {8, 3, 11};
  enemy.speed = 2;
  enemy.playerThreat = 10;
  g.enemies.push_back(enemy);
  tick(g, 3);
  REQUIRE(g.health == 18);
  REQUIRE(g.playerHp == 100);
  REQUIRE(g.towers[0].hp == 650);
  REQUIRE(g.kills == 0);
}
TEST_CASE(
    "Siege breakers path around obstacles to towers and ignore nearby players",
    "[specialists]") {
  Game g;
  combatArena(g);
  g.playerPosition = {8.5f, 4.65f, 7};
  for (int x = 6; x <= 10; ++x)
    for (int y = 3; y < 8; ++y)
      g.world.set(x, y, 10, Stone);
  Tower tower;
  tower.type = 0;
  tower.position = {8.5f, 3, 12.5f};
  tower.cooldown = 100;
  g.towers.push_back(tower);
  auto enemy = grunt();
  enemy.kind = 10;
  enemy.position = {8.5f, 3, 7.5f};
  enemy.speed = 2;
  enemy.hp = enemy.maxHp = 2000;
  g.enemies.push_back(enemy);
  tick(g, .1f);
  REQUIRE(g.towers[0].hp == 180);
  tick(g, 7);
  REQUIRE(g.towers[0].hp < 180);
  REQUIRE(g.playerHp == 100);
  REQUIRE(g.health == 20);
}
TEST_CASE("Thieves target extractors only and return stolen money on death",
          "[specialists]") {
  Game g;
  combatArena(g);
  g.playerPosition = {10, 4.65f, 9};
  g.resources = 100;
  Tower extractor;
  extractor.type = 3;
  extractor.position = {11.5f, 3, 8.5f};
  extractor.hp = extractor.maxHp = 90;
  Tower wall;
  wall.type = 4;
  wall.position = {10.2f, 3, 8.5f};
  wall.hp = wall.maxHp = 650;
  g.towers = {extractor, wall};
  auto thief = grunt();
  thief.kind = 11;
  thief.position = {10, 3, 8.5f};
  g.enemies.push_back(thief);
  auto survivor = grunt(501);
  survivor.position = {20, 3, 20};
  survivor.hp = survivor.maxHp = 1000;
  g.enemies.push_back(survivor);
  g.update(.02f);
  REQUIRE(g.towers[0].hp == 84);
  REQUIRE(g.towers[1].hp == 650);
  REQUIRE(g.playerHp == 100);
  REQUIRE(g.resources == 94);
  REQUIRE(g.enemies[0].stolenCredits == 6);
  g.enemies[0].hp = 0;
  g.update(.02f);
  REQUIRE(g.resources == 112);
  REQUIRE(g.kills == 1);
}
TEST_CASE("Specialists without valid towers leave without attacking the core",
          "[specialists]") {
  for (int kind : {10, 11}) {
    Game g;
    combatArena(g);
    g.world.routes[0] = {{8, 3, 10}, {8, 3, 11}, {8, 3, 12}};
    auto enemy = grunt();
    enemy.kind = kind;
    enemy.position = {8, 3, 10};
    enemy.speed = 2;
    g.enemies.push_back(enemy);
    tick(g, 2);
    REQUIRE(g.health == 20);
    REQUIRE(g.playerHp == 100);
    REQUIRE(g.kills == 0);
  }
}
TEST_CASE(
    "Resource nodes require living enemies and cannot be farmed between spawns",
    "[exploration]") {
  Game g;
  g.reset(0);
  g.start();
  g.update(.02f);
  REQUIRE(g.resourcePointsAvailable());
  auto it = std::find_if(
      g.resourcePoints.begin(), g.resourcePoints.end(),
      [](const auto &p) { return p.active && p.kind == PickupKind::Gold; });
  REQUIRE(it != g.resourcePoints.end());
  size_t index = size_t(it - g.resourcePoints.begin());
  g.playerPosition = it->position + Vec3{0, 1.65f, 0};
  REQUIRE(g.collectResourcePoint(index));
  int money = g.resources;
  for (auto &enemy : g.enemies)
    enemy.hp = 0;
  REQUIRE_FALSE(g.resourcePointsAvailable());
  REQUIRE(g.nearbyResourcePoint() == -1);
  REQUIRE_FALSE(g.collectResourcePoint(index));
  g.update(.02f);
  REQUIRE(std::none_of(g.resourcePoints.begin(), g.resourcePoints.end(),
                       [](const auto &p) { return p.active; }));
  auto enemy = grunt(900);
  enemy.hp = enemy.maxHp = 100000;
  g.enemies.push_back(enemy);
  g.update(.02f);
  REQUIRE(g.resourcePointsAvailable());
  REQUIRE_FALSE(g.resourcePoints[index].active);
  REQUIRE_FALSE(g.collectResourcePoint(index));
  REQUIRE(g.resources >= money);
  g.enemies.clear();
  g.finishWave();
  REQUIRE_FALSE(g.resourcePointsAvailable());
  REQUIRE(std::none_of(g.resourcePoints.begin(), g.resourcePoints.end(),
                       [](const auto &p) { return p.active; }));
  REQUIRE(g.choose(0));
  REQUIRE_FALSE(g.resourcePointsAvailable());
  g.start();
  REQUIRE_FALSE(g.resourcePointsAvailable());
}
