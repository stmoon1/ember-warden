#include "game/Game.h"
#include <algorithm>
namespace citadel::game {
Block World::at(int x, int y, int z) const {
  if (x < 0 || y < 0 || z < 0 || x >= Size || y >= Size || z >= Size)
    return Air;
  return Block(cells[x + Size * (z + Size * y)]);
}
void World::set(int x, int y, int z, Block block) {
  if (x >= 0 && y >= 0 && z >= 0 && x < Size && y < Size && z < Size)
    cells[x + Size * (z + Size * y)] = block;
}
void World::generate(int index) {
  level = index == 0 ? 0 : 1;
  cells.fill(Air);
  routes.clear();
  lanterns.clear();
  base = {29.5f, 3, 23.5f};
  routes = {{{2.5f, 3, 6.5f}, {12.5f, 3, 6.5f}, {12.5f, 3, 23.5f}, base},
            {{29.5f, 3, 2.5f},
             {22.5f, 3, 2.5f},
             {22.5f, 3, 14.5f},
             {29.5f, 3, 14.5f},
             base},
            {{2.5f, 3, 29.5f}, {19.5f, 3, 29.5f}, {19.5f, 3, 23.5f}, base}};
  if (level == 1) {
    routes[0] = {{2.5f, 3, 4.5f},   {10.5f, 3, 4.5f},  {10.5f, 3, 15.5f},
                 {15.5f, 3, 15.5f}, {15.5f, 3, 23.5f}, base};
    routes[2] = {{2.5f, 3, 29.5f},  {8.5f, 3, 29.5f},  {8.5f, 3, 24.5f},
                 {19.5f, 3, 24.5f}, {19.5f, 3, 23.5f}, base};
  }
  for (int z = 0; z < Size; ++z)
    for (int x = 0; x < Size; ++x) {
      set(x, 0, z, Stone);
      set(x, 1, z, Dirt);
      set(x, 2, z, Grass);
      if (x == 0 || z == 0 || x == Size - 1 || z == Size - 1) {
        set(x, 2, z, Carved);
        set(x, 3, z, Stone);
        if ((x + z) % 3 == 0)
          set(x, 4, z, Carved);
      }
    }
  for (const auto &route : routes)
    for (size_t i = 1; i < route.size(); ++i) {
      Vec3 delta = route[i] - route[i - 1];
      int samples = int(length(delta) * 4);
      for (int step = 0; step <= samples; ++step) {
        Vec3 p = route[i - 1] + delta * (float(step) / samples);
        set(int(p.x), 2, int(p.z), Road);
        // Worn road borders remain walkable and mark its width.
        if (step % 12 == 0)
          for (int side : {-1, 1}) {
            int bx = int(p.x) + (delta.z != 0 ? side : 0),
                bz = int(p.z) + (delta.x != 0 ? side : 0);
            if (at(bx, 2, bz) != Road)
              set(bx, 2, bz, Carved);
          }
      }
    }
  auto clearRoad = [&](int x, int z) {
    for (int dz = -2; dz <= 2; ++dz)
      for (int dx = -2; dx <= 2; ++dx)
        if (at(x + dx, 2, z + dz) == Road)
          return false;
    return x > 1 && z > 1 && x < 30 && z < 30 &&
           !(x >= 6 && x <= 11 && z >= 7 && z <= 13);
  };
  // Four ruined structures with open courtyards, crenellations and roof
  // fragments.
  for (auto center :
       std::array<Vec3, 4>{{{5, 3, 18}, {17, 3, 7}, {26, 3, 8}, {25, 3, 28}}}) {
    int cx = int(center.x), cz = int(center.z);
    for (int dz = -2; dz <= 2; ++dz)
      for (int dx = -2; dx <= 2; ++dx) {
        int x = cx + dx, z = cz + dz;
        if (!clearRoad(x, z))
          continue;
        bool pillar = std::abs(dx) == 2 && std::abs(dz) == 2;
        bool wall = (std::abs(dx) == 2 || std::abs(dz) == 2) && dx != 0;
        int height = pillar ? 6 : wall ? 2 : 0;
        for (int h = 0; h < height; ++h)
          set(x, 3 + h, z, h % 3 == 2 ? Carved : Stone);
        if (pillar) {
          set(x, 9, z, Roof);
          set(x, 10, z, Crystal);
          lanterns.push_back({x + .5f, 9.8f, z + .5f});
        }
        if (dx == 0 && dz == 0) {
          set(x, 2, z, Rune);
        }
      }
  }
  // Garden clusters and glowing shrine pedestals fill secondary courtyards.
  for (int z = 5; z < 29; z += 5)
    for (int x = 4; x < 29; x += 6) {
      if (!clearRoad(x, z) || at(x, 3, z) != Air)
        continue;
      if ((x + z) % 3 == 0) {
        set(x, 3, z, Carved);
        set(x, 4, z, Crystal);
        lanterns.push_back({x + .5f, 4.8f, z + .5f});
      } else {
        set(x, 3, z, Wood);
        set(x, 4, z, Wood);
        for (int dx = -1; dx <= 1; ++dx)
          for (int dz = -1; dz <= 1; ++dz)
            if (clearRoad(x + dx, z + dz))
              set(x + dx, 5, z + dz, Leaves);
        set(x, 6, z, Leaves);
      }
    }
  // Warm lanterns alternate along the streets and remain out of the walking
  // lane.
  for (const auto &route : routes)
    for (size_t i = 1; i < route.size(); ++i) {
      Vec3 delta = route[i] - route[i - 1];
      int count = int(length(delta) / 6);
      for (int j = 0; j < count; ++j) {
        Vec3 p = route[i - 1] + delta * ((j + .5f) / std::max(1, count));
        int x = int(p.x) + (delta.z != 0 ? 2 : 0),
            z = int(p.z) + (delta.x != 0 ? 2 : 0);
        if (x <= 1 || x >= 30 || z <= 1 || z >= 30 || at(x, 3, z) != Air ||
            at(x, 2, z) == Road)
          continue;
        set(x, 3, z, Carved);
        set(x, 4, z, Wood);
        set(x, 5, z, Core);
        lanterns.push_back({x + .5f, 5.5f, z + .5f});
      }
    }
  if (level == 1) {
    for (int z = 16; z <= 20; ++z)
      for (int x = 3; x <= 6; ++x)
        if (at(x, 3, z) == Air && at(x, 2, z) == Grass)
          set(x, 2, z, Water);
    for (int z = 8; z <= 11; ++z)
      for (int x = 26; x <= 28; ++x)
        if (at(x, 3, z) == Air && at(x, 2, z) == Grass)
          set(x, 2, z, Lava);
  }
  // Ensure the operator's deployment area and testable construction pads are
  // open.
  for (int z = 8; z <= 12; ++z)
    for (int x = 7; x <= 11; ++x)
      for (int y = 3; y <= 5; ++y)
        set(x, y, z, Air);
  set(11, 2, 8, Grass);
  rebuildMeshes();
}
bool World::buildable(int x, int z) const {
  return x > 0 && z > 0 && x < 31 && z < 31 && at(x, 2, z) == Grass &&
         at(x, 3, z) == Air;
}
bool World::walkable(float x, float z) const {
  return x > .25f && z > .25f && x < 31.75f && z < 31.75f &&
         at(int(x), 3, int(z)) == Air && at(int(x), 4, int(z)) == Air;
}
bool World::raycast(Vec3 origin, Vec3 direction, int &x, int &y, int &z) const {
  direction = normalized(direction);
  for (float distance = 0; distance <= 40; distance += .035f) {
    Vec3 point = origin + direction * distance;
    int px = int(std::floor(point.x)), py = int(std::floor(point.y)),
        pz = int(std::floor(point.z));
    if (at(px, py, pz) != Air) {
      x = px;
      y = py;
      z = pz;
      return true;
    }
  }
  return false;
}
void World::appendCube(MeshData &mesh, int x, int y, int z, Block block,
                       const World *world, bool exposedOnly) {
  const Vec3 normal[] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                         {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
  const Vec3 tangent[] = {{0, 0, -1}, {0, 0, 1}, {1, 0, 0},
                          {1, 0, 0},  {1, 0, 0}, {-1, 0, 0}};
  const Vec3 up[] = {{0, 1, 0}, {0, 1, 0}, {0, 0, -1},
                     {0, 0, 1}, {0, 1, 0}, {0, 1, 0}};
  for (int face = 0; face < 6; ++face) {
    Vec3 n = normal[face];
    if (exposedOnly && world &&
        world->at(x + int(n.x), y + int(n.y), z + int(n.z)) != Air)
      continue;
    auto first = uint32_t(mesh.vertices.size());
    int tile = int(block) - 1;
    if (block == Grass && face != 2)
      tile = int(Dirt) - 1;
    for (auto uv : std::array<std::array<float, 2>, 4>{
             {{{0, 0}}, {{1, 0}}, {{1, 1}}, {{0, 1}}}}) {
      Vec3 p = Vec3{x + .5f, y + .5f, z + .5f} + n * .5f +
               tangent[face] * (uv[0] - .5f) + up[face] * (uv[1] - .5f);
      mesh.vertices.push_back({p.x, p.y, p.z, n.x, n.y, n.z, uv[0], uv[1],
                               float(tile), tangent[face].x, tangent[face].y,
                               tangent[face].z, 1});
    }
    for (uint32_t corner : {0u, 1u, 2u, 0u, 2u, 3u})
      mesh.indices.push_back(first + corner);
  }
}
void World::rebuildMeshes() {
  mesh = {};
  for (int y = 0; y < Size; ++y)
    for (int z = 0; z < Size; ++z)
      for (int x = 0; x < Size; ++x)
        if (at(x, y, z) != Air)
          appendCube(mesh, x, y, z, at(x, y, z), this, true);
}
} // namespace citadel::game
