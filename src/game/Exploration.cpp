#include "Game.h"
#include <limits>
namespace citadel::game {
namespace {
constexpr int Side = World::Size;
std::array<int, Side * Side> reachable(const World &world, int start) {
  std::array<int, Side * Side> parent;
  parent.fill(-1);
  std::array<int, Side * Side> queue{};
  int head = 0, tail = 0;
  if (start < 0 || start >= Side * Side)
    return parent;
  queue[tail++] = start;
  parent[start] = start;
  while (head < tail) {
    int cell = queue[head++], x = cell % Side, z = cell / Side;
    for (auto offset : std::array<std::array<int, 2>, 4>{
             {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
      int nx = x + offset[0], nz = z + offset[1], next = nx + nz * Side;
      if (nx < 0 || nz < 0 || nx >= Side || nz >= Side || parent[next] >= 0 ||
          !world.walkable(nx + .5f, nz + .5f))
        continue;
      parent[next] = cell;
      queue[tail++] = next;
    }
  }
  return parent;
}
} // namespace
int Game::dismantleRefund(size_t index) const {
  if (index >= towers.size() || towers[index].hp <= 0)
    return 0;
  return std::max(0, towers[index].invested * 60 / 100);
}
bool Game::dismantleTower(size_t index) {
  if (index >= towers.size() || towers[index].hp <= 0 || state == State::Lost ||
      state == State::Won)
    return false;
  int refund = dismantleRefund(index);
  burst(towers[index].position + Vec3{0, .5f, 0}, 10, 16);
  resources += refund;
  towers.erase(towers.begin() + index);
  message = "Tower dismantled. Credits refunded: " + std::to_string(refund);
  return true;
}
void Game::generateResourcePoints() {
  resourcePoints.clear();
  auto parent = reachable(world, 8 + 12 * Side);
  const Vec3 preferred[] = {{4.5f, 3, 12.5f},  {15.5f, 3, 12.5f},
                            {19.5f, 3, 18.5f}, {8.5f, 3, 24.5f},
                            {24.5f, 3, 19.5f}, {16.5f, 3, 27.5f},
                            {27.5f, 3, 12.5f}, {10.5f, 3, 18.5f}};
  const PickupKind kinds[] = {PickupKind::Gold, PickupKind::Fury,
                              PickupKind::Gold, PickupKind::Haste,
                              PickupKind::Gold, PickupKind::Ward,
                              PickupKind::Gold, PickupKind::Healing};
  for (int node = 0; node < 8; ++node) {
    float best = std::numeric_limits<float>::max();
    Vec3 position;
    bool found = false;
    for (int z = 2; z < 30; ++z)
      for (int x = 2; x < 30; ++x) {
        Vec3 candidate{x + .5f, 3, z + .5f};
        if (parent[x + z * Side] < 0 || !world.buildable(x, z) ||
            length(candidate - Vec3{8, 3, 12}) < 4 ||
            length(candidate - world.base) < 3 ||
            length(candidate - Vec3{11.5f, 3, 8.5f}) < 1.8f)
          continue;
        if (std::any_of(resourcePoints.begin(), resourcePoints.end(),
                        [&](const ResourcePoint &p) {
                          return length(candidate - p.position) < 3;
                        }))
          continue;
        float score = length(candidate - preferred[node]);
        if (score < best) {
          best = score;
          position = candidate;
          found = true;
        }
      }
    if (found)
      resourcePoints.push_back({position, kinds[node], false});
  }
  refreshResourcePoints();
}
void Game::refreshResourcePoints() {
  for (auto &point : resourcePoints)
    point.active = false;
  if (!resourcePointsAvailable())
    return;
  auto activate = [&](size_t index) {
    auto &point = resourcePoints[index];
    point.active =
        point.collectedWave != wave &&
        std::none_of(towers.begin(), towers.end(), [&](const Tower &tower) {
          return tower.hp > 0 && length(tower.position - point.position) < .8f;
        });
  };
  std::vector<size_t> gold, buffs;
  for (size_t i = 0; i < resourcePoints.size(); ++i)
    (resourcePoints[i].kind == PickupKind::Gold ? gold : buffs).push_back(i);
  for (size_t n = 0; n < std::min(size_t(2), gold.size()); ++n)
    activate(gold[((wave - 1) * 2 + n) % gold.size()]);
  if (!buffs.empty())
    activate(buffs[(wave - 1) % buffs.size()]);
}
bool Game::resourcePointsAvailable() const {
  return state == State::Playing &&
         std::any_of(enemies.begin(), enemies.end(), [](const Enemy &enemy) {
           return enemy.hp > 0 && !enemy.rewarded;
         });
}
int Game::nearbyResourcePoint() const {
  if (!resourcePointsAvailable())
    return -1;
  int nearest = -1;
  float distance = 2.2f;
  for (size_t i = 0; i < resourcePoints.size(); ++i)
    if (resourcePoints[i].active) {
      Vec3 goal = resourcePoints[i].position + Vec3{0, .7f, 0};
      float gap = length(playerPosition - goal);
      if (gap < distance && lineOfSight(playerPosition, goal)) {
        distance = gap;
        nearest = int(i);
      }
    }
  return nearest;
}
bool Game::collectResourcePoint(size_t index) {
  if (!resourcePointsAvailable() || index >= resourcePoints.size() ||
      !resourcePoints[index].active ||
      resourcePoints[index].collectedWave == wave)
    return false;
  auto &point = resourcePoints[index];
  Vec3 goal = point.position + Vec3{0, .7f, 0};
  if (length(playerPosition - goal) >= 2.2f ||
      !lineOfSight(playerPosition, goal))
    return false;
  switch (point.kind) {
  case PickupKind::Gold:
    resources += resourceGoldValue();
    message = "Gold cache collected: " + std::to_string(resourceGoldValue());
    break;
  case PickupKind::Fury:
    furyTime = 20;
    message = "Battle blessing: weapon hit damage +25% for 20s.";
    break;
  case PickupKind::Haste:
    hasteTime = 20;
    message = "Wind blessing: movement speed +20% for 20s.";
    break;
  case PickupKind::Ward:
    wardTime = 20;
    message = "Guardian blessing: incoming damage -20% for 20s.";
    break;
  case PickupKind::Healing:
    playerHp = std::min(maxPlayerHp, playerHp + 30);
    message = "Healing spring: restore 30 HP.";
    break;
  }
  point.active = false;
  point.collectedWave = wave;
  burst(goal, 10, 16);
  return true;
}
void Game::moveEnemyToward(Enemy &enemy, Vec3 destination, float movement,
                           float) {
  Vec3 delta = destination - enemy.position;
  delta.y = 0;
  if (length(delta) < .05f)
    return;
  if (enemy.kind == 3) {
    enemy.position =
        enemy.position + normalized(delta) * std::min(movement, length(delta));
    return;
  }
  Vec3 next =
      enemy.position + normalized(delta) * std::min(movement, length(delta));
  if (world.walkable(next.x, next.z) &&
      lineOfSight(enemy.position + Vec3{0, .4f, 0},
                  destination + Vec3{0, .4f, 0})) {
    enemy.position = next;
    enemy.navigationFailed = false;
    return;
  }
  int goal = int(destination.x) + int(destination.z) * Side;
  if (enemy.navigationGoal != goal || enemy.navigationClock <= 0) {
    enemy.navigationGoal = goal;
    enemy.navigationClock = .6f;
    enemy.navigationIndex = 0;
    enemy.navigation.clear();
    int start = int(enemy.position.x) + int(enemy.position.z) * Side;
    auto parent = reachable(world, start);
    enemy.navigationFailed =
        goal < 0 || goal >= Side * Side || parent[goal] < 0;
    if (!enemy.navigationFailed) {
      for (int cell = goal; cell != start; cell = parent[cell])
        enemy.navigation.push_back({cell % Side + .5f, 3, cell / Side + .5f});
      std::reverse(enemy.navigation.begin(), enemy.navigation.end());
    }
  }
  if (enemy.navigationIndex >= int(enemy.navigation.size()))
    return;
  Vec3 step = enemy.navigation[enemy.navigationIndex] - enemy.position;
  step.y = 0;
  float gap = length(step);
  if (gap <= movement) {
    enemy.position = enemy.navigation[enemy.navigationIndex++];
  } else
    enemy.position = enemy.position + step * (movement / gap);
}
} // namespace citadel::game
