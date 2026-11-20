#pragma once
#include "game/Game.h"
#include <glm/glm.hpp>
namespace citadel::graphics {
class WorldRenderer {
  struct Mesh {
    unsigned vao = 0, vbo = 0, ebo = 0;
    int count = 0;
  };
  Mesh terrain, cube;
  unsigned sceneProgram = 0, postProgram = 0, shadowProgram = 0, atlas = 0;
  unsigned sceneBuffer = 0, sceneTexture = 0, sceneDepth = 0, shadowBuffer = 0,
           shadowTexture = 0, screenVao = 0;
  int sceneWidth = 0, sceneHeight = 0;
  glm::mat4 currentView, lightView;
  bool depthPass = false;
  int modelLocation = -1, shadowModelLocation = -1, materialLocation = -1,
      tintLocation = -1, glowLocation = -1, flashLocation = -1;
  float currentFlash = 0;
  void resize(int width, int height);
  Mesh uploadMesh(const game::MeshData &data);
  void releaseMesh(Mesh &mesh);
  void renderMesh(const Mesh &mesh, const glm::mat4 &model, int material = -1,
                  glm::vec3 tint = {1, 1, 1}, float glow = 0);
  void part(const glm::mat4 &parent, glm::vec3 center, glm::vec3 size, int tile,
            glm::vec3 tint = {1, 1, 1}, float glow = 0);
  void tower(const game::Tower &tower, float time);
  void enemy(const game::Enemy &enemy, float time, int bossVariant);
  void environment(const game::Game &game);
  void scene(const game::Game &game);
  void heldWeapon(const game::Game &game, const glm::mat4 &camera);

public:
  float pixelScale = 2, bloom = .55f, shakeStrength = .45f;
  float materialDetail = .28f;
  bool lighting = true, numbersEnabled = true;
  WorldRenderer();
  ~WorldRenderer();
  void upload(const game::World &world);
  void draw(const game::Game &game, const glm::mat4 &viewProjection,
            glm::vec3 eye, int px, int py, int pz, bool pick, bool valid);
};
} // namespace citadel::graphics
