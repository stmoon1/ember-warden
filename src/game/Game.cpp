#include "game/Game.h"
#include <algorithm>
#include <utility>
namespace citadel::game {
float Enemy::height() const {
  if (kind == 10)
    return 2.6f;
  if (kind == 9 || kind == 11)
    return 1.8f;
  if (kind == 8)
    return 3.9f;
  if (kind == 1)
    return 1.3f;
  if (kind == 6)
    return 1.5f;
  return kind == 2 || kind == 4 || kind == 5 ? 2.2f : 2.f;
}
float Enemy::radius() const {
  if (kind == 10)
    return .8f;
  if (kind == 9 || kind == 11)
    return .48f;
  return kind == 8 ? 1.1f : kind == 2 || kind == 6 ? .72f : .55f;
}
bool Enemy::rayHit(Vec3 origin, Vec3 direction, float range,
                   float &distance) const {
  float near = 0, far = range;
  float origins[] = {origin.x, origin.y, origin.z},
        rays[] = {direction.x, direction.y, direction.z};
  float lower[] = {position.x - radius(), position.y, position.z - radius()};
  float upper[] = {position.x + radius(), position.y + height(),
                   position.z + radius()};
  for (int axis = 0; axis < 3; ++axis) {
    if (std::abs(rays[axis]) < .00001f) {
      if (origins[axis] < lower[axis] || origins[axis] > upper[axis])
        return false;
      continue;
    }
    float entry = (lower[axis] - origins[axis]) / rays[axis],
          exit = (upper[axis] - origins[axis]) / rays[axis];
    if (entry > exit)
      std::swap(entry, exit);
    near = std::max(near, entry);
    far = std::min(far, exit);
    if (near > far)
      return false;
  }
  distance = near;
  return true;
}
float Game::bonus(Bonus stat) const {
  float value = stat == Bonus::MoveSpeed && hasteTime > 0 ? .2f : 0;
  for (size_t i = 0; i < Perks.size(); ++i)
    if (Perks[i].bonus == stat)
      value += Perks[i].amount * perkStacks[i];
  for (size_t index = 0; index < AugmentSets.size(); ++index) {
    int pieces = -1;
    for (const auto &tier : AugmentSets[index].tiers)
      for (const auto &modifier : tier.modifiers) {
        if (modifier.stat == Bonus::None || modifier.stat != stat)
          continue;
        if (pieces < 0)
          pieces = setPieces(index);
        if (pieces >= tier.pieces)
          value += modifier.amount;
      }
  }
  return value;
}
int Game::setPieces(size_t index) const {
  if (index >= AugmentSets.size())
    return 0;
  int count = 0;
  for (auto member : AugmentSets[index].members)
    if (perk(member) > 0)
      ++count;
  return count;
}
int Game::remainingAugments() const {
  return int(std::count(perkStacks.begin(), perkStacks.end(), 0));
}
int Game::magazine(int i) const {
  float multiplier =
      1 + bonus(Bonus::Magazine) + (i == 2 ? bonus(Bonus::MachineMagazine) : 0);
  return int(Weapons[i].magazine * multiplier);
}
int Game::weaponPrice(int i) const {
  return int(Weapons[i].price * (1 - std::min(.5f, bonus(Bonus::Discount))));
}
int Game::ammoPrice(int i) const {
  return int(Weapons[i].ammoPrice *
             (1 - std::min(.5f, bonus(Bonus::Discount))));
}
int Game::towerPrice(int i) const {
  return int(TowerSpecs[i].price *
             (1 - std::min(.5f, bonus(Bonus::TowerDiscount))));
}
int Game::upgradePrice(size_t i) const {
  if (i >= towers.size())
    return 0;
  return int(TowerSpecs[towers[i].type].price *
             (towers[i].level == 1 ? .75f : 1.25f) *
             (1 - std::min(.5f, bonus(Bonus::UpgradeDiscount))));
}
float Game::towerMaxHp(int type, int level) const {
  return TowerSpecs[type].hp * (1 + .5f * (level - 1)) *
         (1 + bonus(Bonus::TowerHealth) +
          (type == 3 ? bonus(Bonus::ExtractorArmor) : 0));
}
int Game::currentWaveSize() const { return std::min(80, 6 + (wave - 1) * 3); }
float Game::currentWaveHealth() const { return waveEnemyHealth(wave); }
float Game::currentWaveSpeed() const { return waveEnemySpeed(wave); }
float Game::currentSpawnInterval() const {
  return std::max(.32f, 1.9f - .09f * (wave - 1));
}
float Game::random() {
  randomState = 1664525u * randomState + 1013904223u;
  return float(randomState >> 8) / 16777216.f;
}
void Game::reset(int level) {
  world.generate(level);
  enemies.clear();
  towers.clear();
  particles.clear();
  tracers.clear();
  damageNumbers.clear();
  state = State::Preparing;
  resources = 0;
  health = 20;
  kills = spawned = spawnedInWave = completedWaves = 0;
  wave = 1;
  selectedType = 0;
  time = 0;
  preparation = 20;
  spawnClock = 0;
  nextId = 1;
  randomState = runSeed;
  endedByPlayer = false;
  playerPosition = previousPlayerPosition = {8, 4.65f, 12};
  playerHp = maxPlayerHp = 100;
  equipped = 0;
  heldTower = -1;
  equipmentRaise = swingTimer = 0;
  attackTimer = reloadTimer = hitMarker = shield = shake = hitStop =
      weaponKick = comboClock = continuousFire = 0;
  sinceDamage = 99;
  shots = combo = 0;
  rerolls = 2;
  drawReady = true;
  emergencyUsed = moving = false;
  owned.fill(false);
  owned[0] = true;
  ammo.fill(0);
  reserve.fill(0);
  blueprints.fill(0);
  for (auto &costs : blueprintCosts)
    costs.clear();
  furyTime = hasteTime = wardTime = 0;
  perkStacks.fill(0);
  choices.fill(-1);
  message = "Knife only. Two strikes per grunt. TAB supplies / SPACE incoming.";
  generateResourcePoints();
}
void Game::start() {
  if (state == State::Preparing) {
    state = State::Playing;
    spawnClock = 0;
    shield = bonus(Bonus::WaveShield);
    emergencyUsed = false;
  }
}
void Game::endRun() {
  if (state == State::Won || state == State::Lost)
    return;
  endedByPlayer = true;
  state = State::Lost;
}
bool Game::buyWeapon(int i) {
  if (i < 0 || i >= int(Weapons.size()) || owned[i] ||
      resources < weaponPrice(i) ||
      (state == State::Lost || state == State::Won))
    return false;
  resources -= weaponPrice(i);
  owned[i] = true;
  ammo[i] = magazine(i);
  reserve[i] = magazine(i) * 3;
  equip(i);
  return true;
}
bool Game::buyAmmo(int i) {
  if (i < 0 || i >= int(Weapons.size()) || !owned[i] ||
      Weapons[i].magazine == 0 || resources < ammoPrice(i) ||
      (state == State::Lost || state == State::Won))
    return false;
  resources -= ammoPrice(i);
  reserve[i] += int(magazine(i) * 3 * (1 + bonus(Bonus::AmmoPack)));
  return true;
}
bool Game::buyTower(int i) {
  if (i < 0 || i >= int(TowerSpecs.size()) || resources < towerPrice(i) ||
      (state == State::Lost || state == State::Won))
    return false;
  int price = towerPrice(i);
  resources -= price;
  ++blueprints[i];
  blueprintCosts[i].push_back(price);
  selectedType = i;
  if (heldTower >= 0)
    equipTower(i);
  message = "Blueprint secured. Select it in equipment belt, then LMB deploy.";
  return true;
}
void Game::equip(int i) {
  if (i >= 0 && i < int(Weapons.size()) && owned[i] &&
      (i != equipped || heldTower >= 0)) {
    equipped = i;
    heldTower = -1;
    equipmentRaise = 1;
    swingTimer = weaponKick = 0;
    reloadTimer = 0;
    drawReady = true;
    continuousFire = 0;
  }
}
bool Game::equipTower(int i) {
  if (i < 0 || i >= int(blueprints.size()) || blueprints[i] <= 0)
    return false;
  if (heldTower != i)
    equipmentRaise = 1;
  heldTower = selectedType = i;
  reloadTimer = swingTimer = weaponKick = 0;
  continuousFire = 0;
  return true;
}
void Game::cycleEquipment(int direction) {
  if (direction == 0)
    return;
  int current = heldTower >= 0 ? int(Weapons.size()) + heldTower : equipped;
  constexpr int count = int(Weapons.size() + TowerSpecs.size());
  for (int step = 1; step <= count; ++step) {
    int slot = (current + (direction > 0 ? step : -step) + count) % count;
    if (slot < int(Weapons.size())) {
      if (owned[slot]) {
        equip(slot);
        return;
      }
    } else if (equipTower(slot - int(Weapons.size())))
      return;
  }
}
void Game::reload() {
  if (heldTower < 0 && reloadTimer <= 0 &&
      magazine(equipped) > ammo[equipped] && reserve[equipped] > 0)
    reloadTimer = Weapons[equipped].reload /
                  (1 + .15f * perk(Perk::Tempo) + bonus(Bonus::ReloadSpeed));
}
bool Game::upgradeTower(size_t i) {
  if (i >= towers.size() || towers[i].level >= 3 ||
      resources < upgradePrice(i) ||
      (state == State::Lost || state == State::Won))
    return false;
  int price = upgradePrice(i);
  resources -= price;
  auto &t = towers[i];
  t.invested += price;
  float old = t.maxHp;
  ++t.level;
  t.maxHp = towerMaxHp(t.type, t.level);
  t.hp += t.maxHp - old;
  burst(t.position + Vec3{0, 1, 0}, 10, 24);
  message = "Tower ascended.";
  return true;
}
bool Game::repairTower(size_t i) {
  if (i >= towers.size() || towers[i].hp >= towers[i].maxHp || resources < 20 ||
      (state == State::Lost || state == State::Won))
    return false;
  resources -= 20;
  towers[i].hp =
      std::min(towers[i].maxHp, towers[i].hp + towers[i].maxHp * .5f);
  return true;
}
std::string Game::placementIssue(int x, int z) const {
  if (state == State::Won || state == State::Lost)
    return "The expedition has ended.";
  if (state == State::Choosing)
    return "Deploy after choosing an upgrade.";
  if (!world.buildable(x, z))
    return "Find clear mossy ground.";
  for (const auto &point : resourcePoints)
    if (point.active && resourcePointsAvailable() &&
        length(point.position - Vec3{x + .5f, 3, z + .5f}) < .8f)
      return "Keep resource points clear.";
  if (blueprints[selectedType] <= 0)
    return "Buy a blueprint in TAB supplies.";
  for (const auto &t : towers)
    if (int(t.position.x) == x && int(t.position.z) == z)
      return "Tile occupied.";
  if (length(Vec3{x + .5f, playerPosition.y, z + .5f} - playerPosition) < .9f)
    return "Step back to deploy.";
  return {};
}
bool Game::place(int x, int z) {
  message = placementIssue(x, z);
  if (!message.empty())
    return false;
  Tower t;
  t.position = {x + .5f, 3, z + .5f};
  t.type = selectedType;
  if (!blueprintCosts[selectedType].empty()) {
    t.invested = blueprintCosts[selectedType].front();
    blueprintCosts[selectedType].erase(blueprintCosts[selectedType].begin());
  }
  t.hp = t.maxHp = towerMaxHp(t.type, 1);
  towers.push_back(t);
  --blueprints[selectedType];
  if (heldTower == selectedType && blueprints[selectedType] == 0)
    equip(equipped);
  burst(t.position, 10, 18);
  message = "Defence online.";
  return true;
}
bool Game::playerCanStand(float x, float z) const {
  for (float dx : {-.22f, .22f})
    for (float dz : {-.22f, .22f})
      if (!world.walkable(x + dx, z + dz))
        return false;
  for (const auto &t : towers)
    if (std::abs(x - t.position.x) < .7f && std::abs(z - t.position.z) < .7f)
      return false;
  return !(std::abs(x - world.base.x) < .95f &&
           std::abs(z - world.base.z) < .95f);
}
bool Game::lineOfSight(Vec3 from, Vec3 to) const {
  int samples = int(length(to - from) * 8) + 1;
  for (int i = 1; i < samples; ++i) {
    Vec3 p = from + (to - from) * (float(i) / samples);
    if (world.at(int(std::floor(p.x)), int(std::floor(p.y)),
                 int(std::floor(p.z))) != Air)
      return false;
  }
  return true;
}
float Game::aura(Vec3 p) const {
  float strength = 0;
  for (const auto &t : towers)
    if (t.type == 2 && t.hp > 0 &&
        length(t.position - p) <
            6 * (1 + .1f * (t.level - 1) + bonus(Bonus::RelayRange)))
      strength = std::max(strength, .2f + .1f * (t.level - 1) +
                                        .1f * perk(Perk::Overclock));
  return strength;
}
void Game::burst(Vec3 position, int tile, int count) {
  for (int i = 0; i < std::max(1, count / 2) && particles.size() < 500; ++i) {
    float life = .18f + random() * .5f;
    particles.push_back(
        {position,
         {(random() - .5f) * 5, random() * 4, (random() - .5f) * 5},
         life,
         life,
         tile,
         .035f + random() * .10f});
  }
}
void Game::directDamage(Enemy &e, float damage, int style, int source) {
  if (e.hp <= 0 || damage <= 0)
    return;
  float dealt = std::min(e.hp, damage);
  e.hp -= damage;
  e.lastSource = source;
  e.flash = .10f;
  if (damageNumbers.size() < 180)
    damageNumbers.push_back({e.position + Vec3{(random() - .5f) * .5f, 1.7f, 0},
                             dealt, .75f, style});
}
void Game::areaDamage(Vec3 position, float radius, float damage,
                      uint32_t exclude, int style, int source) {
  for (auto &e : enemies)
    if (e.id != exclude && e.hp > 0 && length(e.position - position) < radius)
      directDamage(e, damage, style, source);
  burst(position + Vec3{0, .8f, 0}, style == 3 ? 10 : style == 4 ? 8 : 6, 18);
  shake = std::max(shake, .12f);
}
void Game::hurtPlayer(float damage) {
  if (random() < std::min(.5f, bonus(Bonus::Dodge)))
    return;
  damage *=
      1 - std::min(.7f, bonus(Bonus::Armor) + (aura(playerPosition) > 0
                                                   ? bonus(Bonus::RelayArmor)
                                                   : 0));
  damage *= 1 + bonus(Bonus::GlassCannon) / .45f * .20f;
  if (wardTime > 0)
    damage *= .8f;
  float absorbed = std::min(shield, damage);
  shield -= absorbed;
  playerHp -= damage - absorbed;
  sinceDamage = 0;
  shake = std::max(shake, .25f);
  if (bonus(Bonus::Retaliation) > 0)
    areaDamage(playerPosition, 3, bonus(Bonus::Retaliation), 0, 5);
  if (playerHp <= 0 && bonus(Bonus::Emergency) > 0 && !emergencyUsed) {
    playerHp = bonus(Bonus::Emergency);
    emergencyUsed = true;
    burst(playerPosition, 10, 30);
  }
}
void Game::damageEnemy(Enemy &e, float damage, bool tower, bool melee) {
  if (e.hp <= 0)
    return;
  int source = tower ? 2 : 1;
  if (!tower && furyTime > 0)
    damage *= 1.25f;
  if (e.kind == 2 && (tower || melee || perk(Perk::Piercing) == 0))
    damage *= .55f;
  if (e.freeze > 0 && perk(Perk::Shatter))
    damage *= 1.5f;
  if (e.slow > 0)
    damage *= 1 + bonus(Bonus::IceFragile);
  int ailments =
      int(e.burn > 0) + int(e.poison > 0) + int(e.bleed > 0) + int(e.slow > 0);
  damage *= 1 + ailments * bonus(Bonus::StatusHunter);
  if (e.hp >= e.maxHp - .01f)
    damage *= 1 + bonus(Bonus::FirstBlood);
  if (e.hp < e.maxHp * .3f)
    damage *= 1 + bonus(Bonus::Finisher);
  if (e.kind == 8)
    damage *= 1 + bonus(Bonus::BossDamage) +
              (!tower && equipped == 3 ? bonus(Bonus::SniperBoss) : 0);
  if (enemies.size() >= 8)
    damage *= 1 + bonus(Bonus::CrowdDamage);
  damage *= 1 + bonus(Bonus::GlassCannon);
  if (!tower) {
    if (playerHp >= maxPlayerHp - .01f)
      damage *= 1 + bonus(Bonus::FullHealth);
    if (playerHp < maxPlayerHp * .35f)
      damage *= 1 + bonus(Bonus::LowHealth);
  }
  float critical = tower ? bonus(Bonus::TowerCrit) : bonus(Bonus::CritChance);
  if (!tower && equipped == 3)
    critical += bonus(Bonus::SniperCrit);
  if (!tower && equipped == 0)
    critical += bonus(Bonus::KnifeCrit);
  if (e.bleed > 0)
    critical += bonus(Bonus::BleedCrit);
  bool crit = random() < std::min(.85f, critical);
  if (crit)
    damage *= 1.75f + bonus(Bonus::CritDamage) +
              (melee ? bonus(Bonus::MeleeCritDamage) : 0);
  if (e.burn > 0 && e.bleed > 0)
    damage += bonus(Bonus::BloodFire);
  directDamage(e, damage, crit ? 1 : 0, source);
  float force = melee ? .35f + bonus(Bonus::MeleeKnockback) : .08f;
  if (!tower && equipped == 5)
    force += bonus(Bonus::BatImpact) * 2;
  if (!tower && equipped == 4)
    force += bonus(Bonus::ShotgunKnockback);
  Vec3 away = e.position - playerPosition;
  away.y = 0;
  e.impulse = normalized(away) * force * 7;
  e.stagger = std::max(e.stagger, e.kind == 8 ? .015f : melee ? .09f : .025f);
  if (!tower) {
    hitMarker = .13f;
    e.playerThreat = 3.5f;
    hitStop = std::max(hitStop, melee ? .025f : crit ? .012f : 0.f);
    shake = std::max(shake, melee ? .055f : .018f);
  }
  int fire = perk(tower ? Perk::TowerFire : Perk::Incendiary),
      ice = perk(tower ? Perk::TowerFrost : Perk::CryoRounds);
  int poison = perk(tower ? Perk::TowerPoison : Perk::Venom),
      shock = perk(tower ? Perk::TowerShock : Perk::Shock);
  if (fire) {
    e.burn = 3 + bonus(Bonus::BurnDuration);
    e.burnDps = 6.f * fire;
  }
  if (poison) {
    e.poison = 5;
    e.poisonStacks = std::min(8 + int(bonus(Bonus::PoisonCapacity)),
                              e.poisonStacks + poison);
  }
  if (melee && perk(Perk::Bleed)) {
    e.bleed = 4;
    e.bleedDps = 8.f * perk(Perk::Bleed);
  }
  if (ice) {
    e.slow = (2 + bonus(Bonus::Permafrost)) *
             (tower ? 1 + bonus(Bonus::CryoDuration) : 1);
    e.chill += ice;
    if (e.chill >= 3) {
      e.freeze = (e.kind == 8 ? .35f : 1.2f) *
                 (tower ? 1 + bonus(Bonus::CryoDuration) : 1);
      e.chill = 0;
    }
  }
  if (e.burn > 0 && e.slow > 0 && perk(Perk::Volatile)) {
    e.burn = e.slow = 0;
    areaDamage(e.position, 3, 30, 0, 2, source);
  }
  if (shock) {
    int jumps = 0;
    for (auto &other : enemies)
      if (other.id != e.id && other.hp > 0 &&
          length(other.position - e.position) < 4 + bonus(Bonus::ShockRange)) {
        float power =
            8 * shock *
            (1 + bonus(Bonus::ShockPower) + bonus(Bonus::ElementPower));
        if (other.slow > 0 && perk(Perk::Conductor))
          power *= 1.6f;
        if (other.poison > 0)
          power *= 1 + bonus(Bonus::StormVenom);
        directDamage(other, power, 5, source);
        tracers.push_back({e.position + Vec3{0, 1, 0},
                           other.position + Vec3{0, 1, 0}, .16f, 5});
        if (++jumps == 3)
          break;
      }
  }
  if (e.burn > 0 && e.poison > 0 && e.slow > 0 && e.overloadClock <= 0 &&
      bonus(Bonus::Overload) > 0) {
    e.overloadClock = 2;
    areaDamage(e.position, 3, bonus(Bonus::Overload), 0, 5, source);
  }
  if (melee)
    playerHp = std::min(maxPlayerHp, playerHp + 2 * perk(Perk::Vampire));
  burst(e.position + Vec3{0, 1, 0},
        crit     ? 6
        : ice    ? 10
        : poison ? 8
        : fire   ? 11
                 : 13,
        crit ? 14 : 6);
}
void Game::attack(Vec3 origin, Vec3 direction) {
  if (heldTower >= 0 || state != State::Playing || attackTimer > 0 ||
      reloadTimer > 0)
    return;
  const auto &w = Weapons[equipped];
  bool melee = w.magazine == 0;
  if (!melee && ammo[equipped] <= 0) {
    reload();
    return;
  }
  ++shots;
  continuousFire = std::min(1.f, continuousFire + .08f);
  float rate = 1 + .12f * perk(Perk::Tempo);
  if (equipped == 0)
    rate += bonus(Bonus::KnifeSpeed);
  if (equipped == 6)
    rate += bonus(Bonus::KatanaSpeed);
  if (equipped == 2)
    rate += continuousFire * bonus(Bonus::MachineSpin);
  if (!melee) {
    rate *= 1 - .12f * (bonus(Bonus::HeavyRounds) / .35f);
    if (moving)
      rate += bonus(Bonus::MovingFire);
  }
  attackTimer = w.interval / (std::max(.2f, rate) * (1 + aura(playerPosition)));
  int roundsBefore = ammo[equipped];
  if (!melee && random() >= std::min(.8f, bonus(Bonus::FreeAmmo)))
    --ammo[equipped];
  weaponKick = 1;
  if (melee) {
    swingDuration = std::clamp(attackTimer, .16f, .48f);
    swingTimer = swingDuration;
  }
  shake = std::max(shake, melee           ? .025f
                          : equipped == 3 ? .08f
                          : equipped == 4 ? .07f
                                          : .025f);
  direction = normalized(direction);
  int pellets =
      w.pellets + (equipped == 4 ? int(bonus(Bonus::ShotgunPellets)) : 0);
  if (!melee && bonus(Bonus::TwinShot) && shots % 5 == 0)
    pellets *= 1 + int(bonus(Bonus::TwinShot));
  float spread = w.spread * std::max(.1f, 1 + bonus(Bonus::Spread));
  if (equipped == 4)
    spread *= std::max(.1f, 1 - .4f * bonus(Bonus::ShotgunChoke));
  for (int pellet = 0; pellet < pellets; ++pellet) {
    Vec3 ray = normalized(direction + Vec3{(random() - .5f) * spread,
                                           (random() - .5f) * spread,
                                           (random() - .5f) * spread});
    float reach = w.range * (melee ? 1.f : 1 + bonus(Bonus::GunRange));
    if (melee)
      reach += .25f * perk(Perk::Melee) + bonus(Bonus::MeleeReach);
    if (equipped == 4)
      reach += 4 * bonus(Bonus::ShotgunChoke);
    std::vector<std::pair<float, size_t>> hits;
    for (size_t i = 0; i < enemies.size(); ++i) {
      const auto &e = enemies[i];
      if (e.hp <= 0)
        continue;
      Vec3 delta = e.position + Vec3{0, 1, 0} - origin;
      float distance = length(delta),
            along = delta.x * ray.x + delta.y * ray.y + delta.z * ray.z;
      bool intersects = false;
      Vec3 impact = e.position + Vec3{0, 1, 0};
      if (melee)
        intersects = along > 0 && along / std::max(.01f, distance) > .55f &&
                     distance <= reach;
      else {
        intersects = e.rayHit(origin, ray, reach, distance);
        impact = origin + ray * distance;
      }
      if (intersects && lineOfSight(origin, impact))
        hits.push_back({distance, i});
    }
    std::sort(hits.begin(), hits.end());
    int limit =
        melee
            ? ((equipped == 5 || equipped == 6 || perk(Perk::Cleave)) ? 100 : 1)
            : 1 + perk(Perk::Piercing);
    if (equipped == 1)
      limit += int(bonus(Bonus::RiflePierce));
    if (equipped == 3)
      limit += int(bonus(Bonus::SniperPierce));
    Vec3 endpoint = origin + ray * std::min(reach, 40.f);
    int tileX, tileY, tileZ;
    if (world.raycast(origin, ray, tileX, tileY, tileZ) &&
        length(Vec3{tileX + .5f, tileY + .5f, tileZ + .5f} - origin) < reach)
      endpoint = {tileX + .5f, tileY + .5f, tileZ + .5f};
    for (auto [distance, index] : hits) {
      if (limit-- <= 0)
        break;
      auto &e = enemies[index];
      endpoint = e.position + Vec3{0, 1, 0};
      float damage = w.damage * (1 + aura(playerPosition)) *
                     (1 + (melee ? .25f * perk(Perk::Melee)
                                 : .2f * perk(Perk::Ballistics)));
      if (equipped == 1) {
        damage *= 1 + bonus(Bonus::RifleDamage);
        if (shots % 3 == 0)
          damage *= 1 + bonus(Bonus::RifleBurst);
      }
      if (equipped == 5)
        damage *= 1 + bonus(Bonus::BatImpact);
      if (equipped == 6 && drawReady)
        damage *= 1 + bonus(Bonus::KatanaDraw);
      if (melee)
        damage *= 1 + std::min(1.f, combo / 6.f) * bonus(Bonus::MeleeCombo);
      if (!melee) {
        damage *= 1 + bonus(Bonus::HeavyRounds);
        if (distance < 5)
          damage *= 1 + bonus(Bonus::CloseRange);
        if (distance > 12)
          damage *= 1 + bonus(Bonus::LongRange);
        if (!moving)
          damage *= 1 + bonus(Bonus::Stationary);
        if (roundsBefore == 1)
          damage *= 1 + bonus(Bonus::LastRound);
        if (roundsBefore == magazine(equipped))
          damage *= 1 + bonus(Bonus::FirstRound);
      }
      if (!melee && equipped == 3 && perk(Perk::Execution) && e.kind != 8 &&
          e.hp < e.maxHp * .3f)
        damage = e.hp * 2;
      damageEnemy(e, damage, false, melee);
      if (equipped == 2 && shots % 8 == 0 && bonus(Bonus::MachineStorm))
        areaDamage(e.position, 2.5f, 18 * bonus(Bonus::MachineStorm), e.id, 2,
                   1);
      if (!melee && perk(Perk::Ricochet))
        for (auto &other : enemies)
          if (other.id != e.id && other.hp > 0 &&
              length(other.position - e.position) <
                  4 + bonus(Bonus::ShockRange) &&
              lineOfSight(e.position + Vec3{0, 1, 0},
                          other.position + Vec3{0, 1, 0})) {
            directDamage(other, damage * .35f, 5, 1);
            tracers.push_back(
                {endpoint, other.position + Vec3{0, 1, 0}, .1f, 5});
            break;
          }
    }
    if (!melee)
      tracers.push_back({origin + direction * .45f + Vec3{0, -.12f, 0},
                         endpoint, .07f,
                         perk(Perk::Shock)        ? 5
                         : perk(Perk::CryoRounds) ? 3
                         : perk(Perk::Venom)      ? 4
                                                  : 2});
    else if (!hits.empty()) {
      combo = std::min(6, combo + 1);
      comboClock = 2;
    }
  }
  if (equipped == 5 && bonus(Bonus::BatShockwave))
    for (auto &e : enemies)
      if (e.hp > 0) {
        Vec3 delta = e.position - origin;
        float dot = delta.x * direction.x + delta.z * direction.z;
        if (dot > 0 && length(delta) < 4)
          directDamage(e, bonus(Bonus::BatShockwave), 1, 1);
      }
  if (melee) {
    for (int i = -4; i <= 4; ++i) {
      Vec3 side{direction.z, 0, -direction.x};
      Vec3 p = origin + direction * 1.4f + side * (i * .13f) +
               Vec3{0, -.25f + std::abs(i) * .03f, 0};
      burst(p, equipped == 6 ? 10 : 13, 2);
    }
  } else
    burst(origin + direction * .7f + Vec3{0, -.14f, 0}, 6,
          equipped == 4 ? 12 : 4);
  drawReady = false;
}
void Game::spawn() {
  int route = wave == 1 ? 0 : spawned % int(world.routes.size());
  Enemy e;
  e.id = nextId++;
  e.position = world.routes[route][0];
  e.route = route;
  e.kind = 0;
  if (wave > 1) {
    int total = 0;
    for (const auto &spec : EnemySpecs)
      if (wave >= spec.firstWave)
        total += spec.weight;
    int draw = int(random() * total);
    for (int kind = 0; kind < int(EnemySpecs.size()); ++kind) {
      const auto &spec = EnemySpecs[kind];
      if (wave < spec.firstWave || spec.weight == 0)
        continue;
      if (draw < spec.weight) {
        e.kind = kind;
        break;
      }
      draw -= spec.weight;
    }
  }
  if (wave % 5 == 0 && spawnedInWave == currentWaveSize() - 1)
    e.kind = 8;
  e.hp = currentWaveHealth() * enemySpec(e.kind).healthScale;
  e.speed = currentWaveSpeed() * enemySpec(e.kind).speedScale;
  if (e.kind == 3)
    e.position.y = 5;
  e.maxHp = e.hp;
  e.phase = float(spawned);
  enemies.push_back(e);
  ++spawned;
  ++spawnedInWave;
  burst(e.position + Vec3{0, 1, 0}, 14, 16);
}
unsigned Game::activeElements() const {
  unsigned result = 0;
  for (int i = 0; i < int(Perks.size()); ++i)
    if (perkStacks[i])
      result |= enabledElements(Perk(i));
  auto hasTower = [&](int type) {
    return blueprints[type] > 0 ||
           std::any_of(towers.begin(), towers.end(), [=](const Tower &tower) {
             return tower.type == type && tower.hp > 0;
           });
  };
  if (hasTower(1))
    result |= Ice;
  if (hasTower(5))
    result |= Lightning;
  return result;
}
bool Game::augmentEligible(int index) const {
  if (index < 0 || index >= int(Perks.size()) || perkStacks[index])
    return false;
  auto p = Perk(index);
  unsigned active = activeElements(), required = requiredElements(p);
  if (p == Perk::ShockRange && perk(Perk::Ricochet))
    return true;
  if ((active & required) != required)
    return false;
  if (requiresAnyAilment(p) &&
      !(active & (Fire | Ice | Lightning | Poison | Bleeding)))
    return false;
  if (p == Perk::StatusHunter && !(active & (Fire | Ice | Poison | Bleeding)))
    return false;
  auto hasTower = [&](int type) {
    return blueprints[type] > 0 ||
           std::any_of(towers.begin(), towers.end(), [=](const Tower &tower) {
             return tower.type == type && tower.hp > 0;
           });
  };
  if (p == Perk::TeslaJumps && !hasTower(5))
    return false;
  if (p == Perk::CryoDuration && !perk(Perk::TowerFrost) && !hasTower(1))
    return false;
  return true;
}
int Game::availableChoices() const {
  return int(std::count_if(choices.begin(), choices.end(),
                           [&](int id) { return augmentEligible(id); }));
}
void Game::rollChoices() {
  choices.fill(-1);
  std::array<std::vector<int>, 3> pools;
  for (int i = 0; i < int(Perks.size()); ++i)
    if (augmentEligible(i))
      pools[int(perkRarity(Perk(i)))].push_back(i);
  // Reserve source-independent golds so losing an elemental tower cannot
  // invalidate a future three-gold boss draft.
  int pendingBosses = std::max(0, FinalWave / 5 - (wave - 1) / 5);
  auto independentGold = [](int id) {
    return requiredElements(Perk(id)) == 0 && !requiresAnyAilment(Perk(id));
  };
  int independentCount =
      int(std::count_if(pools[2].begin(), pools[2].end(), independentGold));
  if (!bossReward() && independentCount <= pendingBosses + 2)
    pools[2].erase(
        std::remove_if(pools[2].begin(), pools[2].end(), independentGold),
        pools[2].end());
  for (int slot = 0; slot < 3; ++slot) {
    int total = 0;
    for (int tier = 0; tier < 3; ++tier)
      if (!pools[tier].empty() && (!bossReward() || tier == 2))
        total += RarityWeights[tier];
    if (total == 0)
      break;
    int draw = int(random() * total), selectedTier = 0;
    for (int tier = 0; tier < 3; ++tier) {
      if (pools[tier].empty() || (bossReward() && tier != 2))
        continue;
      if (draw < RarityWeights[tier]) {
        selectedTier = tier;
        break;
      }
      draw -= RarityWeights[tier];
    }
    auto &pool = pools[selectedTier];
    int selected = int(random() * pool.size());
    choices[slot] = pool[selected];
    pool.erase(pool.begin() + selected);
  }
}
bool Game::reroll() {
  if (state != State::Choosing || rerolls <= 0 || availableChoices() == 0)
    return false;
  --rerolls;
  rollChoices();
  return true;
}
void Game::acquire(int index) {
  if (index < 0 || index >= int(Perks.size()) || perkStacks[index] > 0)
    return;
  std::vector<float> oldHp;
  for (const auto &t : towers)
    oldHp.push_back(t.maxHp);
  ++perkStacks[index];
  if (index == int(Perk::Vitality)) {
    maxPlayerHp += 25;
    playerHp = std::min(maxPlayerHp, playerHp + 40);
  }
  for (size_t i = 0; i < towers.size(); ++i) {
    auto &t = towers[i];
    t.maxHp = towerMaxHp(t.type, t.level);
    t.hp += t.maxHp - oldHp[i];
  }
}
void Game::nextWave() {
  if (wave >= FinalWave) {
    state = State::Won;
    choices.fill(-1);
    message = "All 50 waves cleared. Victory!";
    return;
  }
  ++wave;
  refreshResourcePoints();
  spawnedInWave = 0;
  preparation = 20;
  state = State::Preparing;
  message = "Augment installed. TAB supplies / SPACE next wave.";
}
bool Game::choose(int index) {
  if (state != State::Choosing || index < 0 || index >= 3)
    return false;
  int selected = choices[index];
  if (!augmentEligible(selected))
    return false;
  std::array<int, AugmentSets.size()> before{};
  for (size_t i = 0; i < before.size(); ++i)
    before[i] = setPieces(i);
  acquire(selected);
  nextWave();
  for (size_t i = 0; i < before.size(); ++i)
    for (const auto &tier : AugmentSets[i].tiers)
      if (before[i] < tier.pieces && setPieces(i) >= tier.pieces)
        message += " [" + std::string(AugmentSets[i].name) + " " +
                   std::to_string(tier.pieces) + "pc activated]";
  choices.fill(-1);
  return true;
}
bool Game::continueWithoutUpgrade() {
  if (state != State::Choosing || availableChoices() != 0)
    return false;
  nextWave();
  if (state != State::Won)
    message = "No augments available. Proceed to the next wave.";
  return true;
}
void Game::finishWave() {
  if (state == State::Choosing || state == State::Lost || state == State::Won)
    return;
  completedWaves = wave;
  int income = int((50 + wave * 8) * (1 + bonus(Bonus::WaveBounty)));
  income += std::min(int(resources * bonus(Bonus::Interest)),
                     int(60 * bonus(Bonus::Interest) / .05f));
  int extractors =
      int(std::count_if(towers.begin(), towers.end(),
                        [](const Tower &t) { return t.type == 3; }));
  for (auto &t : towers) {
    if (t.type == 3)
      income += int(35 * (1 + .5f * (t.level - 1))) +
                15 * perk(Perk::Dividend) +
                int(std::min(25.f, std::max(0, extractors - 1) *
                                       bonus(Bonus::ExtractorNetwork))) +
                (aura(t.position) > 0 ? int(bonus(Bonus::RelayIncome)) : 0);
    if (perk(Perk::Repair))
      t.hp = std::min(t.maxHp, t.hp + t.maxHp * .2f * perk(Perk::Repair));
  }
  resources += income;
  playerHp = std::min(maxPlayerHp, playerHp + 15);
  rollChoices();
  state = State::Choosing;
  message = "Sector secured. Select one augment.";
  refreshResourcePoints();
}
void Game::resolveDeaths() {
  std::vector<Enemy> children;
  // Repeat only for actual new deaths so chained explosions resolve once each.
  bool pending = true;
  while (pending) {
    pending = false;
    for (auto &e : enemies)
      if (e.hp <= 0 && !e.rewarded) {
        pending = true;
        e.rewarded = true;
        ++kills;
        resources += e.stolenCredits +
                     int((e.kind == 8 ? 120 : KillReward) *
                         (1 + .2f * perk(Perk::Treasury))) +
                     (e.kind == 8 ? int(bonus(Bonus::BossBounty)) : 0) +
                     (e.lastSource == 2 ? int(bonus(Bonus::TowerBounty)) : 0);
        playerHp = std::min(maxPlayerHp, playerHp + bonus(Bonus::KillHeal));
        if (e.lastSource == 1 && Weapons[equipped].magazine)
          ammo[equipped] =
              std::min(magazine(equipped),
                       ammo[equipped] + int(bonus(Bonus::KillReload)));
        burst(e.position + Vec3{0, .8f, 0}, e.kind == 8 ? 14 : 13,
              e.kind == 8 ? 65 : 18);
        shake = std::max(shake, e.kind == 8 ? .4f : .055f);
        if (e.kind == 7) {
          for (auto &t : towers)
            if (length(t.position - e.position) < 3)
              t.hp -= 45;
          if (length(playerPosition - e.position) < 3)
            hurtPlayer(20);
        }
        if (e.burn > 0 && bonus(Bonus::FireExplosion))
          areaDamage(e.position, 3, bonus(Bonus::FireExplosion), e.id, 2,
                     e.lastSource);
        if (e.freeze > 0 && bonus(Bonus::IceBurst))
          areaDamage(e.position, 3, bonus(Bonus::IceBurst), e.id, 3,
                     e.lastSource);
        float fireRadius = 3 + std::max(0.f, bonus(Bonus::FireSpread) - 1);
        float poisonRadius = 3 + std::max(0.f, bonus(Bonus::PoisonSpread) - 1);
        for (auto &other : enemies)
          if (other.hp > 0) {
            float distance = length(other.position - e.position);
            if (e.burn > 0 && bonus(Bonus::FireSpread) > 0 &&
                distance < fireRadius) {
              other.burn = 3 + bonus(Bonus::BurnDuration);
              other.burnDps = e.burnDps;
              other.lastSource = e.lastSource;
            }
            if (e.poison > 0 && bonus(Bonus::PoisonSpread) > 0 &&
                distance < poisonRadius) {
              other.poison = 5;
              other.poisonStacks =
                  std::min(8 + int(bonus(Bonus::PoisonCapacity)),
                           other.poisonStacks + e.poisonStacks);
              other.lastSource = e.lastSource;
            }
          }
        if (e.kind == 6)
          for (int side : {-1, 1}) {
            Enemy child;
            child.id = nextId++;
            child.kind = 1;
            child.position = e.position + Vec3{side * .25f, 0, 0};
            child.route = e.route;
            child.waypoint = e.waypoint;
            child.hp = child.maxHp = currentWaveHealth() * .3f;
            child.speed = currentWaveSpeed() * 1.3f;
            children.push_back(child);
          }
      }
  }
  enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                               [](const Enemy &e) { return e.hp <= 0; }),
                enemies.end());
  enemies.insert(enemies.end(), children.begin(), children.end());
  towers.erase(std::remove_if(towers.begin(), towers.end(),
                              [](const Tower &t) { return t.hp <= 0; }),
               towers.end());
}
void Game::update(float dt) {
  dt = std::clamp(dt, 0.f, .25f);
  while (dt > 0) {
    float h = std::min(dt, .02f);
    step(h);
    dt -= h;
  }
  refreshResourcePoints();
}
void Game::step(float dt) {
  if (state == State::Lost || state == State::Choosing || state == State::Won)
    return;
  attackTimer = std::max(0.f, attackTimer - dt);
  hitMarker = std::max(0.f, hitMarker - dt);
  weaponKick = std::max(0.f, weaponKick - dt * 8);
  equipmentRaise = std::max(0.f, equipmentRaise - dt * 5);
  furyTime = std::max(0.f, furyTime - dt);
  hasteTime = std::max(0.f, hasteTime - dt);
  wardTime = std::max(0.f, wardTime - dt);
  swingTimer = std::max(0.f, swingTimer - dt);
  shake = std::max(0.f, shake - dt * 2);
  hitStop = std::max(0.f, hitStop - dt);
  comboClock = std::max(0.f, comboClock - dt);
  if (comboClock <= 0)
    combo = 0;
  if (attackTimer <= 0)
    continuousFire = std::max(0.f, continuousFire - dt);

  sinceDamage += dt;
  if (sinceDamage > 4)
    playerHp =
        std::min(maxPlayerHp, playerHp + bonus(Bonus::Regeneration) * dt);
  for (auto &number : damageNumbers) {
    number.life -= dt;
    number.position.y += dt * .8f;
  }
  damageNumbers.erase(
      std::remove_if(damageNumbers.begin(), damageNumbers.end(),
                     [](const DamageNumber &n) { return n.life <= 0; }),
      damageNumbers.end());
  for (auto &tracer : tracers)
    tracer.life -= dt;
  tracers.erase(std::remove_if(tracers.begin(), tracers.end(),
                               [](const Tracer &t) { return t.life <= 0; }),
                tracers.end());
  for (auto &p : particles) {
    p.life -= dt;
    p.velocity.y -= 5 * dt;
    p.position = p.position + p.velocity * dt;
  }
  particles.erase(std::remove_if(particles.begin(), particles.end(),
                                 [](const Particle &p) { return p.life <= 0; }),
                  particles.end());
  if (reloadTimer > 0) {
    reloadTimer -= dt;
    if (reloadTimer <= 0) {
      int count =
          std::min(reserve[equipped], magazine(equipped) - ammo[equipped]);
      ammo[equipped] += count;
      reserve[equipped] -= count;
      if (bonus(Bonus::ReloadNova))
        areaDamage(playerPosition, 5 + bonus(Bonus::ShockRange),
                   bonus(Bonus::ReloadNova) * (1 + bonus(Bonus::ShockPower) +
                                               bonus(Bonus::ElementPower)),
                   0, 5, 1);
    }
  }
  if (state == State::Preparing) {
    preparation -= dt;
    if (preparation <= 0)
      start();
    return;
  }
  time += dt;
  if (hitStop > 0) {
    resolveDeaths();
    return;
  }
  if (aura(playerPosition) > 0)
    playerHp =
        std::min(maxPlayerHp, playerHp + 3 * perk(Perk::RelayMedic) * dt);
  spawnClock -= dt;
  if (spawnedInWave < currentWaveSize() && spawnClock <= 0) {
    spawn();
    spawnClock = currentSpawnInterval();
  }
  for (const auto &healer : enemies)
    if (healer.kind == 5 && healer.hp > 0)
      for (auto &e : enemies)
        if (e.hp > 0 && length(e.position - healer.position) < 4)
          e.hp = std::min(e.maxHp, e.hp + 4 * dt);
  std::vector<Enemy> summons;
  for (auto &e : enemies) {
    if (e.hp <= 0)
      continue;
    e.flash = std::max(0.f, e.flash - dt);
    e.stagger = std::max(0.f, e.stagger - dt);
    e.overloadClock = std::max(0.f, e.overloadClock - dt);
    e.playerThreat = std::max(0.f, e.playerThreat - dt);
    e.navigationClock = std::max(0.f, e.navigationClock - dt);
    Vec3 displaced = e.position + e.impulse * dt;
    if (e.kind == 3 || world.walkable(displaced.x, displaced.z))
      e.position = displaced;
    e.impulse = e.impulse * std::max(0.f, 1 - dt * 14);
    e.attackClock -= dt;
    e.phase += dt * e.speed * 7;
    e.stride = std::max(0.f, e.stride - dt * 9);
    float dot = 0;
    int dotStyle = 2;
    if (e.burn > 0) {
      e.burn -= dt;
      dot += e.burnDps *
             (1 + bonus(Bonus::BurnPower) + bonus(Bonus::ElementPower)) *
             (e.poison > 0 ? 1 + bonus(Bonus::ToxicFlame) : 1) * dt;
    }
    if (e.poison > 0) {
      e.poison -= dt;
      dot += 2 * e.poisonStacks *
             (1 + bonus(Bonus::PoisonPower) + bonus(Bonus::ElementPower)) * dt;
      dotStyle = 4;
    } else
      e.poisonStacks = 0;
    if (e.bleed > 0) {
      e.bleed -= dt;
      dot += e.bleedDps *
             (1 + bonus(Bonus::BleedPower) + bonus(Bonus::ElementPower)) * dt;
      dotStyle = 6;
    }
    // Fractional status damage is accumulated for readable periodic numbers.
    e.dotDamage += dot;
    e.dotClock -= dt;
    if (e.dotClock <= 0 && e.dotDamage > 0) {
      directDamage(e, e.dotDamage, dotStyle, e.lastSource);
      e.dotDamage = 0;
      e.dotClock = .3f;
    }
    e.slow = std::max(0.f, e.slow - dt);
    e.freeze = std::max(0.f, e.freeze - dt);
    if (e.hp <= 0 || e.freeze > 0 || e.stagger > 0)
      continue;
    if (e.kind == 8) {
      e.abilityClock -= dt;
      if (e.abilityClock <= 0) {
        e.abilityClock = 7;
        int variant = (wave / 5 - 1) % 3;
        if (variant == 0) {
          for (auto &t : towers)
            if (length(t.position - e.position) < 8)
              t.hp -= 25;
          if (length(playerPosition - e.position) < 8)
            hurtPlayer(15);
          burst(e.position + Vec3{0, 1, 0}, 11, 40);
          message = "SIEGE PULSE! Keep your distance.";
        } else if (variant == 1 && enemies.size() + summons.size() < 100)
          for (int side : {-1, 1}) {
            Enemy child;
            child.id = nextId++;
            child.kind = 1;
            child.position = e.position + Vec3{side * .5f, 0, 0};
            child.route = e.route;
            child.waypoint = e.waypoint;
            child.hp = child.maxHp = currentWaveHealth() * .4f;
            child.speed = currentWaveSpeed() * 1.4f;
            summons.push_back(child);
          }
        else if (variant == 2)
          e.hp = std::min(e.maxHp, e.hp + e.maxHp * .04f);
      }
    }
    auto goal = enemySpec(e.kind).goal;
    bool exclusiveTowers =
        goal == EnemyGoal::TowersOnly || goal == EnemyGoal::ExtractorsOnly;
    Tower *target = nullptr;
    float nearest = 1e9f;
    float towerScore = 1e9f;
    for (auto &t : towers)
      if (t.hp > 0 && goal != EnemyGoal::CrystalOnly &&
          (goal != EnemyGoal::ExtractorsOnly || t.type == 3)) {
        int towerCell = int(t.position.x) + int(t.position.z) * World::Size;
        if (exclusiveTowers && e.navigationFailed && e.navigationClock > 0 &&
            e.navigationGoal == towerCell)
          continue;
        Vec3 delta = t.position - e.position;
        delta.y = 0;
        float distance = length(delta);
        float aggro = exclusiveTowers ? 100.f
                      : t.type == 4   ? 5 + bonus(Bonus::BulwarkTaunt)
                      : e.kind == 6   ? 7.f
                                      : 2.1f;
        float score = distance * (t.type == 4 ? .45f : 1.f);
        if (distance < aggro && score < towerScore &&
            (exclusiveTowers || lineOfSight(e.position + Vec3{0, 1, 0},
                                            t.position + Vec3{0, 1, 0}))) {
          target = &t;
          nearest = distance;
          towerScore = score;
        }
      }
    float playerGap = length(playerPosition - (e.position + Vec3{0, 1.65f, 0}));
    const float detection[] = {6, 9, 4.5f, 7.5f, 12, 5, 7, 6.5f, 11};
    bool pursuePlayer =
        goal == EnemyGoal::Mixed &&
        playerGap < detection[std::clamp(e.kind, 0, 8)] +
                        (e.playerThreat > 0 ? 4.f : 0.f) &&
        playerGap * (e.playerThreat > 0 ? .45f : .65f) < towerScore &&
        lineOfSight(e.position + Vec3{0, 1, 0}, playerPosition);
    if (pursuePlayer)
      target = nullptr;
    Vec3 visualGoal =
        target ? target->position
               : world.routes[e.route][std::min(
                     e.waypoint, int(world.routes[e.route].size() - 1))];
    if (!target && e.kind == 3)
      visualGoal = world.base;
    if (pursuePlayer)
      visualGoal = playerPosition;
    Vec3 facing = visualGoal - e.position;
    if (std::abs(facing.x) + std::abs(facing.z) > .001f) {
      float desired = std::atan2(facing.x, facing.z);
      e.yaw +=
          std::remainder(desired - e.yaw, 6.2831853f) * std::min(1.f, dt * 10);
    }
    float damage = enemySpec(e.kind).damage * (1 + .10f * (wave - 1));
    bool ranged = e.kind == 4 || e.kind == 8;
    if (target && nearest < (ranged ? 7.f : 1.8f) &&
        lineOfSight(e.position + Vec3{0, 1, 0},
                    target->position + Vec3{0, 1, 0})) {
      if (e.attackClock <= 0) {
        target->hp -= damage;
        if (goal == EnemyGoal::ExtractorsOnly) {
          int stolen = std::min({6, resources, 60 - e.stolenCredits});
          resources -= stolen;
          e.stolenCredits += stolen;
        }
        directDamage(
            e,
            damage * (.4f * perk(Perk::Thorns) +
                      (target->type == 4 ? bonus(Bonus::BulwarkThorns) : 0)),
            1, 2);
        if (e.kind == 7) {
          target->hp -= 50;
          e.hp = 0;
        }
        e.attackClock = 1;
        burst(target->position + Vec3{0, 1, 0}, 6, 8);
        if (ranged)
          tracers.push_back({e.position + Vec3{0, 1, 0},
                             target->position + Vec3{0, 1, 0}, .2f, 2});
      }
      continue;
    }
    float playerDistance =
        length(playerPosition - (e.position + Vec3{0, 1.65f, 0}));
    if (goal == EnemyGoal::Mixed && (pursuePlayer || !target) &&
        playerDistance < (ranged ? 9.f : 1.7f) &&
        lineOfSight(e.position + Vec3{0, 1, 0}, playerPosition)) {
      if (e.attackClock <= 0) {
        hurtPlayer(damage);
        e.attackClock = ranged ? 1.5f : 1.2f;
        if (ranged)
          tracers.push_back(
              {e.position + Vec3{0, 1, 0}, playerPosition, .2f, 2});
      }
      continue;
    }
    float enrage =
        e.kind == 8 && (wave / 5 - 1) % 3 == 2 && e.hp < e.maxHp * .5f ? 1.7f
                                                                       : 1;
    float movement = dt * e.speed * (e.slow > 0 ? .5f : 1) * enrage;
    e.stride = std::min(1.f, e.stride + dt * 16);
    if (pursuePlayer) {
      Vec3 destination = playerPosition - Vec3{0, 1.1f, 0};
      Vec3 d = destination - e.position;
      if (e.kind != 3)
        d.y = 0;
      Vec3 next = e.position + normalized(d) * movement;
      if (e.kind == 3 || world.walkable(next.x, next.z))
        e.position = next;
      else if (world.walkable(next.x, e.position.z))
        e.position.x = next.x;
      else if (world.walkable(e.position.x, next.z))
        e.position.z = next.z;
    } else if (target) {
      moveEnemyToward(e, target->position, movement, dt);
    } else if (e.kind == 3) {
      Vec3 destination = world.base;
      destination.y = 5;
      e.position = e.position + normalized(destination - e.position) * movement;
      if (length(destination - e.position) < .7f) {
        --health;
        e.rewarded = true;
        e.hp = 0;
      }
    } else {
      const auto &path = world.routes[e.route];
      if (e.waypoint < int(path.size())) {
        Vec3 d = path[e.waypoint] - e.position;
        float distance = length(d);
        if (distance <= movement) {
          e.position = path[e.waypoint];
          ++e.waypoint;
        } else
          e.position = e.position + d * (movement / distance);
      }
      if (e.waypoint >= int(path.size())) {
        health -= enemySpec(e.kind).coreDamage;
        if (e.stolenCredits > 0)
          message = "A thief escaped with stolen credits: " +
                    std::to_string(e.stolenCredits);
        e.rewarded = true;
        e.hp = 0;
      }
    }
  }
  enemies.insert(enemies.end(), summons.begin(), summons.end());
  for (auto &t : towers) {
    if (t.hp <= 0)
      continue;
    t.hp = std::min(
        t.maxHp, t.hp + dt * (bonus(Bonus::TowerRegen) +
                              (t.type == 4 ? bonus(Bonus::BulwarkMend) : 0)));
    t.cooldown -= dt;
    t.recoil = std::max(0.f, t.recoil - dt * 3);
    if (t.type == 2 || t.type == 3 || t.type == 4)
      continue;
    Enemy *target = nullptr;
    float nearest = TowerSpecs[t.type].range *
                    (1 + .1f * (t.level - 1) + bonus(Bonus::TowerRange));
    for (auto &e : enemies)
      if (e.hp > 0) {
        float distance = length(e.position - t.position);
        if (distance < nearest && lineOfSight(t.position + Vec3{0, 1.35f, 0},
                                              e.position + Vec3{0, 1, 0})) {
          nearest = distance;
          target = &e;
        }
      }
    if (!target)
      continue;
    Vec3 d = target->position - t.position;
    t.yaw = std::atan2(d.x, d.z);
    if (t.cooldown <= 0) {
      ++t.shots;
      float boost = aura(t.position),
            damage = TowerSpecs[t.type].damage * (1 + .5f * (t.level - 1)) *
                     (1 + .2f * perk(Perk::Engineering)) * (1 + boost);
      if (t.type == 0)
        damage *= 1 + bonus(Bonus::CannonHeavy);
      if (t.shots % 3 == 0)
        damage *= 1 + bonus(Bonus::TwinTurret);
      if (t.type == 1) {
        target->slow =
            (2 + bonus(Bonus::Permafrost)) * (1 + bonus(Bonus::CryoDuration));
        ++target->chill;
        if (target->chill >= 3) {
          target->freeze = (target->kind == 8 ? .35f : 1.2f) *
                           (1 + bonus(Bonus::CryoDuration));
          target->chill = 0;
        }
      }
      damageEnemy(*target, damage, true, false);
      if (t.type == 0 && bonus(Bonus::CannonSplash))
        areaDamage(target->position, 2.5f, damage * bonus(Bonus::CannonSplash),
                   target->id, 2, 2);
      if (t.type == 5) {
        int jumps = 0;
        for (auto &e : enemies)
          if (e.id != target->id && e.hp > 0 &&
              length(e.position - target->position) <
                  4 + bonus(Bonus::ShockRange)) {
            float power =
                damage * .6f *
                (1 + bonus(Bonus::ShockPower) + bonus(Bonus::ElementPower));
            if (e.slow > 0 && perk(Perk::Conductor))
              power *= 1.6f;
            if (e.poison > 0)
              power *= 1 + bonus(Bonus::StormVenom);
            directDamage(e, power, 5, 2);
            tracers.push_back({target->position + Vec3{0, 1, 0},
                               e.position + Vec3{0, 1, 0}, .15f, 5});
            if (++jumps == 2 + t.level + int(bonus(Bonus::TeslaJumps)))
              break;
          }
      }
      tracers.push_back({t.position + Vec3{0, 1.5f, 0},
                         target->position + Vec3{0, 1, 0}, .12f,
                         t.type == 1   ? 3
                         : t.type == 5 ? 5
                                       : 2});
      burst(t.position + Vec3{0, 1.5f, 0}, t.type == 1 ? 10 : 6, 6);
      t.recoil = .18f;
      float speed = 1 + bonus(Bonus::TowerRate);
      if (t.type == 0)
        speed *= std::max(.3f, 1 - .15f * bonus(Bonus::CannonHeavy) / .5f);
      t.cooldown = TowerSpecs[t.type].interval / ((1 + boost) * speed);
    }
  }
  resolveDeaths();
  if (health <= 0 || playerHp <= 0) {
    health = std::max(0, health);
    playerHp = std::max(0.f, playerHp);
    state = State::Lost;
    message = "Defence breached. F5 begins a new expedition.";
    return;
  }
  if (spawnedInWave >= currentWaveSize() && enemies.empty())
    finishWave();
}
} // namespace citadel::game
