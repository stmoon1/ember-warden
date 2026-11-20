#pragma once
#include "game/Arsenal.h"
#include "game/AugmentCatalog.h"
#include "game/EnemyCatalog.h"
#include "game/Sets.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
namespace citadel::game {
struct Vec3 {
  float x = 0, y = 0, z = 0;
  Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
  Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
  Vec3 operator*(float k) const { return {x * k, y * k, z * k}; }
};
inline float length(Vec3 v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
inline Vec3 normalized(Vec3 v) {
  float magnitude = length(v);
  return magnitude > .00001f ? v * (1 / magnitude) : Vec3{};
}
enum Block : uint8_t {
  Air,
  Grass,
  Dirt,
  Road,
  Stone,
  Wood,
  Metal,
  Core,
  EnemySkin,
  Leaves,
  Roof,
  Crystal,
  Lava,
  Carved,
  Bone,
  Rune,
  Water
};
struct Vertex {
  float x, y, z, nx, ny, nz, u, v, tile, tx, ty, tz, handedness;
};
struct MeshData {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};
class World {
public:
  static constexpr int Size = 32;
  std::array<uint8_t, Size * Size * Size> cells{};
  std::vector<std::vector<Vec3>> routes;
  std::vector<Vec3> lanterns;
  MeshData mesh;
  Vec3 base;
  int level = 0;
  Block at(int x, int y, int z) const;
  void set(int x, int y, int z, Block block);
  void generate(int index);
  bool buildable(int x, int z) const;
  bool walkable(float x, float z) const;
  bool raycast(Vec3 origin, Vec3 direction, int &x, int &y, int &z) const;
  void rebuildMeshes();
  static void appendCube(MeshData &mesh, int x, int y, int z, Block block,
                         const World *world, bool exposedOnly);
};
struct Enemy {
  uint32_t id = 0;
  Vec3 position;
  int route = 0, waypoint = 1;
  float hp = 0, maxHp = 0, speed = 0, slow = 0, phase = 0;
  int kind = 0, poisonStacks = 0;
  float yaw = 0, stride = 0;
  float height() const;
  float radius() const;
  bool rayHit(Vec3 origin, Vec3 direction, float range, float &distance) const;
  float abilityClock = 6, attackClock = 0, burn = 0, poison = 0, bleed = 0,
        freeze = 0, chill = 0;
  float burnDps = 0, bleedDps = 0, flash = 0, stagger = 0, overloadClock = 0;
  float dotDamage = 0, dotClock = .3f;
  Vec3 impulse;
  bool rewarded = false;
  int lastSource = 0;
  float playerThreat = 0;
  int stolenCredits = 0, navigationGoal = -1, navigationIndex = 0;
  float navigationClock = 0;
  bool navigationFailed = false;
  std::vector<Vec3> navigation;
};
struct Tower {
  Vec3 position;
  int type = 0;
  float cooldown = 0, yaw = 0, recoil = 0;
  float hp = 180, maxHp = 180;
  int level = 1, shots = 0;
  int invested = 0;
};
enum class PickupKind { Gold, Fury, Haste, Ward, Healing };
struct ResourcePoint {
  Vec3 position;
  PickupKind kind = PickupKind::Gold;
  bool active = false;
  int collectedWave = 0;
};
inline const char *pickupName(PickupKind kind) {
  constexpr const char *names[] = {"Gold cache", "Battle shrine", "Wind shrine",
                                   "Guardian shrine", "Healing spring"};
  return names[int(kind)];
}
struct Particle {
  Vec3 position, velocity;
  float life, maxLife;
  int tile;
  float size = .10f;
};
struct DamageNumber {
  Vec3 position;
  float damage, life = 0.8f;
  int style = 0;
};
struct Tracer {
  Vec3 from, to;
  float life = .09f;
  int element = 0;
};
enum class State { Preparing, Playing, Won, Lost, Choosing };
class Game {
  uint32_t nextId = 1, randomState = 9415;
  float spawnClock = 0;
  float random();
  void spawn();
  void burst(Vec3 position, int tile, int count);
  void step(float dt);
  void resolveDeaths();
  void nextWave();
  void generateResourcePoints();
  void refreshResourcePoints();
  void moveEnemyToward(Enemy &enemy, Vec3 destination, float movement,
                       float dt);

public:
  World world;
  std::vector<Enemy> enemies;
  std::vector<Tower> towers;
  std::vector<Particle> particles;
  std::vector<DamageNumber> damageNumbers;
  std::vector<Tracer> tracers;
  std::vector<ResourcePoint> resourcePoints;
  std::array<std::vector<int>, 6> blueprintCosts;
  float furyTime = 0, hasteTime = 0, wardTime = 0;
  State state = State::Preparing;
  int resources = 0, health = 20, kills = 0, spawned = 0, selectedType = 0;
  float time = 0, preparation = 20;
  int wave = 1, completedWaves = 0, spawnedInWave = 0;
  bool endedByPlayer = false;
  std::string message;
  Vec3 playerPosition{8, 4.65f, 12}, previousPlayerPosition;
  float playerHp = 100, maxPlayerHp = 100, attackTimer = 0, reloadTimer = 0,
        hitMarker = 0;
  float shield = 0, sinceDamage = 99, shake = 0, hitStop = 0, weaponKick = 0,
        comboClock = 0, continuousFire = 0;
  int equipped = 0, shots = 0, combo = 0, rerolls = 2;
  int heldTower = -1;
  float equipmentRaise = 0, swingTimer = 0, swingDuration = .3f;
  bool drawReady = true, emergencyUsed = false, moving = false;
  std::array<bool, 7> owned{{true, false, false, false, false, false, false}};
  std::array<int, 7> ammo{}, reserve{};
  std::array<int, 6> blueprints{};
  std::array<int, static_cast<int>(Perk::Count)> perkStacks{};
  std::array<int, 3> choices{};
  uint32_t runSeed = 9415;
  int perk(Perk p) const { return perkStacks[static_cast<int>(p)]; }
  float bonus(Bonus stat) const;
  int setPieces(size_t index) const;
  int remainingAugments() const;
  int availableChoices() const;
  bool bossReward() const { return wave % 5 == 0; }
  unsigned activeElements() const;
  bool augmentEligible(int index) const;
  static constexpr int FinalWave = 50;
  bool continueWithoutUpgrade();
  int magazine(int index) const;
  int weaponPrice(int index) const;
  int ammoPrice(int index) const;
  int towerPrice(int index) const;
  int upgradePrice(size_t index) const;
  int dismantleRefund(size_t index) const;
  bool dismantleTower(size_t index);
  int nearbyResourcePoint() const;
  bool resourcePointsAvailable() const;
  bool collectResourcePoint(size_t index);
  int resourceGoldValue() const { return 20 + std::min(20, (wave - 1) * 2); }
  float towerMaxHp(int type, int level) const;
  int cost() const { return towerPrice(selectedType); }
  static constexpr int KillReward = 12;
  static float towerRange(int type) { return TowerSpecs[type].range; }
  void reset(int level);
  void start();
  void endRun();
  void update(float dt);
  bool buyWeapon(int index);
  bool buyAmmo(int index);
  bool buyTower(int index);
  bool upgradeTower(size_t index);
  bool repairTower(size_t index);
  void equip(int index);
  bool equipTower(int index);
  void cycleEquipment(int direction);
  void reload();
  void attack(Vec3 origin, Vec3 direction);
  bool choose(int index);
  void rollChoices();
  bool reroll();
  void acquire(int index);
  bool lineOfSight(Vec3 from, Vec3 to) const;
  float aura(Vec3 position) const;
  void damageEnemy(Enemy &enemy, float damage, bool tower, bool melee);
  void directDamage(Enemy &enemy, float damage, int style = 0, int source = 0);
  void areaDamage(Vec3 position, float radius, float damage,
                  uint32_t exclude = 0, int style = 2, int source = 0);
  void hurtPlayer(float damage);
  void finishWave();
  std::string placementIssue(int x, int z) const;
  bool place(int x, int z);
  bool playerCanStand(float x, float z) const;
  int currentWaveSize() const;
  float currentWaveSpeed() const;
  float currentWaveHealth() const;
  float currentSpawnInterval() const;
};
} // namespace citadel::game
