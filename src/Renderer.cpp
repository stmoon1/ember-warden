#include "Renderer.h"
#include "Engine.h"
#include "imgui.h"
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iterator>
#include <stdexcept>
namespace citadel::graphics {
namespace {
glm::vec3 vector(game::Vec3 p) { return {p.x, p.y, p.z}; }
std::string source(const char *name) {
  std::ifstream file(asset(std::string("shaders/") + name));
  return {std::istreambuf_iterator<char>(file), {}};
}
glm::vec3 elementColor(int element) {
  switch (element) {
  case 3:
    return {.25f, .85f, 1.2f};
  case 4:
    return {.45f, 1.1f, .25f};
  case 5:
    return {.7f, .5f, 1.4f};
  case 6:
    return {1.2f, .15f, .25f};
  default:
    return {1.3f, .65f, .16f};
  }
}
} // namespace
WorldRenderer::Mesh WorldRenderer::uploadMesh(const game::MeshData &data) {
  Mesh mesh;
  mesh.count = int(data.indices.size());
  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);
  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(game::Vertex),
               data.vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(uint32_t),
               data.indices.data(), GL_STATIC_DRAW);
  const int sizes[] = {3, 3, 2, 1};
  const size_t offsets[] = {
      offsetof(game::Vertex, x), offsetof(game::Vertex, nx),
      offsetof(game::Vertex, u), offsetof(game::Vertex, tile)};
  for (unsigned i = 0; i < 4; ++i) {
    glEnableVertexAttribArray(i);
    glVertexAttribPointer(i, sizes[i], GL_FLOAT, GL_FALSE, sizeof(game::Vertex),
                          reinterpret_cast<void *>(offsets[i]));
  }
  glBindVertexArray(0);
  return mesh;
}
void WorldRenderer::releaseMesh(Mesh &mesh) {
  glDeleteBuffers(1, &mesh.ebo);
  glDeleteBuffers(1, &mesh.vbo);
  glDeleteVertexArrays(1, &mesh.vao);
  mesh = {};
}
WorldRenderer::WorldRenderer() {
  sceneProgram = compileProgram(source("citadel.vert"), source("citadel.frag"));
  postProgram = compileProgram(source("screen.vert"), source("screen.frag"));
  shadowProgram =
      compileProgram("#version 330 core\nlayout(location=0)in vec3 p;uniform "
                     "mat4 model,viewProjection;void "
                     "main(){gl_Position=viewProjection*model*vec4(p,1);}",
                     "#version 330 core\nvoid main(){}");
  modelLocation = glGetUniformLocation(sceneProgram, "model");
  shadowModelLocation = glGetUniformLocation(shadowProgram, "model");
  materialLocation = glGetUniformLocation(sceneProgram, "materialOverride");
  tintLocation = glGetUniformLocation(sceneProgram, "tint");
  glowLocation = glGetUniformLocation(sceneProgram, "glow");
  flashLocation = glGetUniformLocation(sceneProgram, "flash");
  atlas = loadTexture(asset("pixel/citadel-atlas.png"));
  game::MeshData data;
  game::World::appendCube(data, 0, 0, 0, game::Metal, nullptr, false);
  cube = uploadMesh(data);
  glGenVertexArrays(1, &screenVao);
  glGenFramebuffers(1, &sceneBuffer);
  glGenTextures(1, &sceneTexture);
  glGenRenderbuffers(1, &sceneDepth);
  glGenFramebuffers(1, &shadowBuffer);
  glGenTextures(1, &shadowTexture);
  glBindTexture(GL_TEXTURE_2D, shadowTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1536, 1536, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float border[] = {1, 1, 1, 1};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         shadowTexture, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("Shadow framebuffer failed");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  lightView = glm::ortho(-29.f, 29.f, -29.f, 29.f, 1.f, 110.f) *
              glm::lookAt(glm::vec3(-12, 48, -6), glm::vec3(16, 0, 16),
                          glm::vec3(0, 1, 0));
}
WorldRenderer::~WorldRenderer() {
  releaseMesh(terrain);
  releaseMesh(cube);
  glDeleteProgram(sceneProgram);
  glDeleteProgram(postProgram);
  glDeleteProgram(shadowProgram);
  glDeleteTextures(1, &atlas);
  glDeleteTextures(1, &sceneTexture);
  glDeleteTextures(1, &shadowTexture);
  glDeleteFramebuffers(1, &sceneBuffer);
  glDeleteFramebuffers(1, &shadowBuffer);
  glDeleteRenderbuffers(1, &sceneDepth);
  glDeleteVertexArrays(1, &screenVao);
}
void WorldRenderer::upload(const game::World &world) {
  releaseMesh(terrain);
  terrain = uploadMesh(world.mesh);
}
void WorldRenderer::resize(int width, int height) {
  if (width == sceneWidth && height == sceneHeight)
    return;
  sceneWidth = width;
  sceneHeight = height;
  glBindTexture(GL_TEXTURE_2D, sceneTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
               GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindRenderbuffer(GL_RENDERBUFFER, sceneDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
  glBindFramebuffer(GL_FRAMEBUFFER, sceneBuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         sceneTexture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, sceneDepth);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("Scene framebuffer failed");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void WorldRenderer::renderMesh(const Mesh &mesh, const glm::mat4 &model,
                               int material, glm::vec3 tint, float glow) {
  glUniformMatrix4fv(depthPass ? shadowModelLocation : modelLocation, 1,
                     GL_FALSE, glm::value_ptr(model));
  if (!depthPass) {
    glUniform1i(materialLocation, material);
    glUniform3fv(tintLocation, 1, glm::value_ptr(tint));
    glUniform1f(glowLocation, glow);
    glUniform1f(flashLocation, currentFlash);
  }
  glBindVertexArray(mesh.vao);
  glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, nullptr);
}
void WorldRenderer::part(const glm::mat4 &parent, glm::vec3 center,
                         glm::vec3 size, int tile, glm::vec3 tint, float glow) {
  glm::mat4 model = glm::translate(parent, center);
  model = glm::scale(model, size);
  model = glm::translate(model, glm::vec3(-.5f));
  renderMesh(cube, model, tile, tint, glow);
}
void WorldRenderer::tower(const game::Tower &t, float time) {
  auto root = glm::translate(glm::mat4(1), vector(t.position));
  const glm::vec3 colors[] = {{1, .75f, .35f},   {.35f, .8f, 1},
                              {.3f, 1, .75f},    {1, .8f, .3f},
                              {.85f, .45f, .3f}, {.7f, .5f, 1}};
  auto color = colors[t.type];
  part(root, {0, .12f, 0}, {.95f, .24f, .95f}, 12);
  part(root, {0, .35f, 0}, {.7f, .28f, .7f}, 5, color);
  for (int side : {-1, 1}) {
    part(root, {side * .38f, .5f, 0}, {.12f, .42f, .75f}, 5);
    part(root, {0, .48f, side * .38f}, {.75f, .18f, .10f}, 6, color, .8f);
  }
  if (t.type == 4) {
    part(root, {0, 1, 0}, {1.2f, 1.55f, .55f}, 3);
    part(root, {0, 1.05f, .31f}, {.9f, .8f, .12f}, 5);
    for (float x : {-.45f, 0.f, .45f})
      part(root, {x, 1.95f, 0}, {.14f, .38f, .48f}, 5);
    part(root, {0, 1.2f, .4f}, {.3f, .5f, .08f}, 6, color, .5f);
  } else if (t.type == 2) {
    part(root, {0, 1.15f, 0}, {.22f, 1.6f, .22f}, 5);
    auto orbit =
        glm::rotate(glm::translate(root, {0, 1.9f, 0}), time, {0, 1, 0});
    for (int side : {-1, 1}) {
      part(orbit, {side * .45f, 0, 0}, {.2f, .45f, .2f}, 10, color, 1);
      part(orbit, {0, 0, side * .45f}, {.2f, .45f, .2f}, 10, color, 1);
    }
    part(root, {0, 2.25f, 0}, {.25f, .25f, .25f}, 10, color, 1.4f);
  } else if (t.type == 3) {
    part(root, {0, .9f, 0}, {.7f, .9f, .7f}, 5);
    part(root, {0, 1.4f, 0}, {.3f, .7f, .3f}, 10, {1, .8f, .2f}, .6f);
    auto rotor =
        glm::rotate(glm::translate(root, {0, 1.5f, 0}), time * 2, {0, 1, 0});
    part(rotor, {0, 0, 0}, {.95f, .1f, .18f}, 5);
    part(root, {0, .95f, .37f}, {.35f, .35f, .08f}, 6, color, .8f);
  } else {
    part(root, {0, .85f, 0}, {.34f, .85f, .34f}, 5);
    auto turret =
        glm::rotate(glm::translate(root, {0, 1.35f, 0}), t.yaw, {0, 1, 0});
    part(turret, {0, 0, 0}, {.72f, .45f, .65f}, 5);
    part(turret, {0, .28f, 0}, {.42f, .12f, .45f}, 12);
    if (t.type == 5) {
      for (int side : {-1, 1})
        part(turret, {side * .22f, .25f, .1f}, {.1f, .7f, .1f}, 10, color, .9f);
    } else {
      part(turret, {0, 0, .6f - t.recoil}, {.24f, .24f, .9f}, 5);
      part(turret, {0, 0, 1.05f - t.recoil}, {.3f, .3f, .12f},
           t.type == 1 ? 10 : 6, color, t.recoil > 0 ? 1.2f : .3f);
    }
  }
  for (int level = 0; level < t.level; ++level)
    part(root, {-.18f + level * .18f, .5f, .45f}, {.10f, .12f, .06f}, 10, color,
         1);
  if (t.hp < t.maxHp) {
    part(root, {0, 2.45f, 0}, {1, .08f, .08f}, 3);
    part(root, {-.5f + .5f * t.hp / t.maxHp, 2.46f, 0},
         {std::max(.01f, t.hp / t.maxHp), .07f, .09f}, 10, color, .5f);
  }
}
void WorldRenderer::enemy(const game::Enemy &e, float time, int bossVariant) {
  const bool frozen = e.freeze > 0;
  const float stride = frozen ? 0.f : e.stride;
  const float gait = std::sin(e.phase) * stride;
  const float attack = std::clamp((e.attackClock - .55f) * 2.2f, 0.f, 1.f);
  const glm::vec3 iron{.72f, .85f, .90f}, dark{.36f, .48f, .55f},
      bone{.92f, .86f, .71f};
  const auto &color = game::enemySpec(e.kind).color;
  glm::vec3 accent = frozen ? glm::vec3(.4f, .85f, 1)
                            : glm::vec3(color[0], color[1], color[2]);
  auto root = glm::translate(glm::mat4(1), vector(e.position));
  root = glm::rotate(root, e.yaw, {0, 1, 0});
  root = glm::rotate(root, e.flash > 0 ? std::sin(time * 65) * .10f : 0.f,
                     {0, 0, 1});
  currentFlash = e.flash > 0 ? .75f : 0;
  auto eyePair = [&](const glm::mat4 &head, float width, float z) {
    for (int side : {-1, 1})
      part(head, {side * width, .025f, z}, {.08f, .055f, .035f}, 13, accent,
           .9f);
  };
  auto claw = [&](const glm::mat4 &limb, float y) {
    for (int finger : {-1, 0, 1})
      part(limb, {finger * .055f, y, .10f}, {.035f, .10f, .15f}, 13, bone);
  };
  if (e.kind == 1 || e.kind == 6) {
    // Hunters and broodlings have low, multi-legged silhouettes.
    bool brood = e.kind == 6;
    float torsoHeight = brood ? .70f : .62f;
    auto body =
        glm::translate(root, {0, torsoHeight + std::abs(gait) * .055f, 0});
    part(body, {0, 0, 0}, {brood ? .83f : .48f, .42f, brood ? .92f : .9f}, 7,
         dark);
    part(body, {0, .22f, -.1f}, {brood ? .65f : .3f, .16f, .65f}, 7, iron);
    int pairs = brood ? 3 : 2;
    for (int side : {-1, 1})
      for (int joint = 0; joint < pairs; ++joint) {
        float z = brood ? -.34f + joint * .34f : -.3f + joint * .58f;
        auto leg =
            glm::translate(body, {side * (brood ? .36f : .22f), -.12f, z});
        leg = glm::rotate(leg, gait * .55f * side * (joint % 2 ? 1 : -1),
                          {1, 0, 0});
        part(leg, {side * .12f, -.1f, 0}, {.24f, .16f, .18f}, 7, iron);
        auto shin = glm::rotate(glm::translate(leg, {side * .2f, -.1f, 0}),
                                side * .3f, {0, 0, 1});
        part(shin, {0, -.2f, 0}, {.13f, .4f, .14f}, 7, dark);
        part(shin, {0, -.38f, .08f}, {.19f, .11f, .27f}, 13, bone);
      }
    auto head = glm::translate(body, {0, .08f, .48f + attack * .09f});
    part(head, {0, 0, 0}, {.42f, .33f, .40f}, 13, bone);
    part(head, {0, -.06f, .24f}, {.3f, .15f, .3f}, 7, dark);
    eyePair(head, .13f, .212f);
    for (int side : {-1, 1}) {
      auto ear = glm::rotate(glm::translate(head, {side * .17f, .23f, -.08f}),
                             side * -.25f, {0, 0, 1});
      part(ear, {0, 0, 0}, {.1f, .27f, .14f}, 7, iron);
      part(head, {side * .1f, -.16f, .32f}, {.05f, .13f, .06f}, 13, bone);
    }
    if (brood)
      for (int pod = 0; pod < 3; ++pod) {
        auto egg = glm::translate(body, {(pod - 1) * .23f, .35f, -.2f});
        part(egg, {0, 0, 0}, {.20f, .38f, .3f}, 14, accent, .12f);
        part(egg, {0, .22f, 0}, {.12f, .12f, .2f}, 7, iron);
      }
    else {
      auto tail = glm::rotate(glm::translate(body, {0, .05f, -.5f}),
                              float(std::sin(e.phase * .5f)) * .2f, {0, 1, 0});
      part(tail, {0, .02f, -.2f}, {.10f, .12f, .45f}, 7, dark);
      for (int ridge = 0; ridge < 3; ++ridge)
        part(body, {0, .32f, -.25f + ridge * .22f}, {.08f, .17f, .14f}, 13,
             bone);
    }
  } else if (e.kind == 3) {
    auto body = glm::translate(
        root, {0, .95f + float(std::sin(time * 4 + e.id)) * .06f, 0});
    part(body, {0, 0, 0}, {.4f, .48f, .45f}, 7, iron);
    part(body, {0, .31f, .12f}, {.34f, .32f, .4f}, 13, bone);
    eyePair(glm::translate(body, {0, .32f, 0}), .1f, .33f);
    part(body, {0, .25f, .42f}, {.12f, .13f, .25f}, 7, iron);
    part(body, {0, .53f, 0}, {.09f, .28f, .2f}, 10, accent, .35f);
    for (int side : {-1, 1}) {
      float flap = std::sin(time * 10 + e.phase) * .3f;
      auto wing = glm::rotate(glm::translate(body, {side * .24f, .1f, 0}),
                              side * flap, {0, 0, 1});
      part(wing, {side * .38f, 0, 0}, {.8f, .12f, .43f}, 7, iron);
      for (int feather = 0; feather < 4; ++feather) {
        auto blade =
            glm::rotate(glm::translate(wing, {side * (.23f + feather * .21f),
                                              -.05f, -.08f - feather * .08f}),
                        side * -.16f, {0, 0, 1});
        part(blade, {0, -.12f, 0}, {.16f, .35f + feather * .07f, .18f}, 9,
             iron);
        part(blade, {0, -.31f - feather * .025f, 0}, {.08f, .09f, .16f}, 10,
             accent, .25f);
      }
      part(body, {side * .12f, -.37f, .04f}, {.08f, .35f, .09f}, 7, iron);
      for (int toe : {-1, 1})
        part(body, {side * .12f + toe * .035f, -.52f, .13f}, {.04f, .08f, .2f},
             13, bone);
    }
  } else {
    bool boss = e.kind == 8, heavy = e.kind == 2 || e.kind == 10,
         caster = e.kind == 4 || e.kind == 5;
    float scale = boss                          ? 1.8f
                  : e.kind == 10                ? 1.23f
                  : heavy                       ? 1.13f
                  : e.kind == 9 || e.kind == 11 ? .9f
                                                : 1.f;
    root = glm::scale(root, glm::vec3(scale));
    auto torso =
        glm::translate(root, {0,
                              1.02f + std::abs(gait) * .035f +
                                  float(std::sin(time * 2.2f + e.id)) * .008f,
                              attack * .07f});
    part(torso, {0, 0, 0}, {heavy || boss ? .68f : .48f, .63f, .35f}, 7, dark);
    part(torso, {0, -.34f, 0}, {.57f, .14f, .40f}, 4, {.68f, .56f, .43f});
    part(torso, {0, -.34f, .22f}, {.13f, .12f, .06f}, 5, {1, .8f, .43f});
    part(torso, {0, .06f, .22f}, {heavy || boss ? .55f : .39f, .43f, .12f}, 7,
         iron);
    part(torso, {0, .12f, .3f}, {.10f, .17f, .045f}, 13, accent,
         boss ? .55f : .22f);
    for (int rib : {-1, 1})
      part(torso, {rib * .19f, -.11f, .25f}, {.12f, .07f, .07f}, 7, iron);
    auto head = glm::translate(torso, {0, .50f, 0});
    if (e.kind == 9) {
      auto shard =
          glm::rotate(glm::translate(torso, {0, .2f, -.30f}), .55f, {0, 0, 1});
      part(shard, {0, 0, 0}, {.18f, .70f, .18f}, 10, accent, .8f);
      for (int side : {-1, 1})
        part(head, {side * .30f, .12f, 0}, {.12f, .30f, .16f}, 10, accent, .4f);
    }
    if (e.kind == 10) {
      part(torso, {.66f, -.15f, .38f}, {.13f, 1.1f, .13f}, 4);
      part(torso, {.66f, .42f, .38f}, {.57f, .35f, .38f}, 5, iron);
      for (int stripe = -1; stripe <= 1; ++stripe)
        part(torso, {stripe * .15f, .12f, .30f}, {.06f, .28f, .03f}, 6, accent,
             .15f);
    }
    if (e.kind == 11) {
      part(head, {0, .12f, -.06f}, {.56f, .42f, .43f}, 9, {.33f, .40f, .30f});
      part(head, {0, .01f, .22f}, {.46f, .13f, .04f}, 9, {.15f, .20f, .17f});
      eyePair(head, .13f, .25f);
      part(torso, {0, -.05f, -.32f}, {.47f, .55f, .30f}, 4, {.7f, .57f, .3f});
      for (int coin = -1; coin <= 1; ++coin)
        part(torso, {coin * .11f, .21f, -.40f}, {.09f, .08f, .07f}, 5, accent,
             e.stolenCredits > 0 ? .6f : .1f);
    }
    if (caster) {
      part(head, {0, .04f, -.015f}, {.52f, .48f, .44f}, 9, {.5f, .6f, .62f});
      part(head, {0, 0, .23f}, {.35f, .31f, .055f}, 7, {.08f, .12f, .16f});
      part(head, {0, .26f, -.04f}, {.30f, .12f, .33f}, 9, dark);
      eyePair(head, .10f, .265f);
      for (int layer = 0; layer < 4; ++layer)
        part(torso, {0, -.23f - layer * .17f, -.13f},
             {.58f + layer * .065f, .2f, .4f}, 9, {.4f, .55f, .58f});
      part(torso, {0, -.48f, .29f}, {.18f, .70f, .05f}, e.kind == 5 ? 8 : 14,
           accent, .07f);
    } else {
      part(head, {0, 0, 0}, {.40f, .37f, .36f}, 13, bone);
      part(head, {0, .18f, -.01f}, {.47f, .10f, .40f}, 7, iron);
      part(head, {0, .03f, .20f}, {.39f, .11f, .05f}, 7, {.2f, .24f, .25f});
      eyePair(head, .105f, .24f);
      part(head, {0, -.16f, .14f}, {.30f, .12f, .23f}, 13, bone);
      for (int tooth : {-1, 0, 1})
        part(head, {tooth * .075f, -.19f, .27f}, {.045f, .065f, .04f}, 7, dark);
      for (int side : {-1, 1})
        part(head, {side * .2f, -.05f, .1f}, {.08f, .19f, .18f}, 7, iron);
    }
    for (int side : {-1, 1}) {
      part(torso, {side * (heavy || boss ? .43f : .33f), .18f, 0},
           {heavy || boss ? .36f : .25f, .24f, .43f}, 7, iron);
      part(torso, {side * (heavy || boss ? .43f : .33f), .30f, 0},
           {.24f, .07f, .34f}, 5, {.95f, .77f, .43f});
      float swing =
          gait * .52f * side - (side == 1 ? attack * .9f : attack * .15f);
      auto arm = glm::rotate(
          glm::translate(torso,
                         {side * (heavy || boss ? .52f : .38f), .12f, 0}),
          swing, {1, 0, 0});
      part(arm, {0, -.15f, 0}, {.18f, .31f, .20f}, 7, dark);
      part(arm, {0, -.33f, .03f}, {.23f, .24f, .25f}, 7, iron);
      part(arm, {0, -.51f, .05f}, {.17f, .15f, .18f}, 13, bone);
      claw(arm, -.54f);
      if (side == 1 && !caster) {
        auto weapon =
            glm::rotate(glm::translate(arm, {0, -.48f, .2f}), -.35f, {1, 0, 0});
        part(weapon, {0, 0, .18f}, {.065f, .065f, .5f}, 4);
        part(weapon, {0, .04f, .43f}, {heavy || boss ? .35f : .22f, .12f, .40f},
             7, iron);
        part(weapon, {-.15f, .04f, .5f}, {.035f, .14f, .28f}, 13, bone);
      }
      if (side == -1 && heavy) {
        part(arm, {-.12f, -.22f, .25f}, {.58f, .72f, .14f}, 7, iron);
        part(arm, {-.12f, -.22f, .34f}, {.13f, .46f, .05f}, 10, accent, .12f);
      }
      auto leg = glm::rotate(glm::translate(torso, {side * .17f, -.41f, 0}),
                             -gait * .48f * side, {1, 0, 0});
      part(leg, {0, -.19f, 0}, {.20f, .34f, .24f}, 7, dark);
      part(leg, {0, -.27f, .15f}, {.25f, .20f, .10f}, 7, iron);
      part(leg, {0, -.46f, .05f}, {.26f, .24f, .35f}, 7, iron);
      part(leg, {0, -.58f, .10f}, {.29f, .07f, .42f}, 7, dark);
      if (boss) {
        auto horn =
            glm::rotate(glm::translate(head, {side * .22f, .18f, -.02f}),
                        side * -.45f, {0, 0, 1});
        part(horn, {0, .16f, 0}, {.12f, .36f, .15f}, 13, bone);
        part(horn, {side * .07f, .36f, 0}, {.09f, .17f, .11f}, 13, bone);
        part(torso, {side * .5f, .43f, -.03f}, {.11f, .33f, .17f}, 13, bone);
        if (bossVariant == 0)
          part(torso, {side * .40f, .28f, -.32f}, {.22f, .23f, .75f}, 7, iron);
        if (bossVariant == 1) {
          part(torso, {side * .24f, .10f, -.38f}, {.24f, .58f, .27f}, 14,
               {.6f, .8f, .3f}, .25f);
        }
      }
    }
    if (caster) {
      auto staff = glm::translate(torso, {-.55f, -.05f, .23f});
      part(staff, {0, 0, 0}, {.08f, 1.8f, .09f}, 4);
      part(staff, {0, .76f, 0}, {.34f, .12f, .28f}, 7, iron);
      auto crystal = glm::rotate(glm::translate(staff, {0, .98f, 0}),
                                 time * .8f, {0, 1, 0});
      part(crystal, {0, 0, 0}, {.21f, .35f, .21f}, 10, accent, .4f);
      if (e.kind == 5)
        for (int side : {-1, 1})
          part(torso, {side * .24f, 0, -.30f}, {.18f, .65f, .22f}, 8,
               {.45f, .85f, .6f});
    }
    if (e.kind == 7) {
      part(torso, {0, .1f, -.3f}, {.55f, .65f, .32f}, 4);
      for (int side : {-1, 1}) {
        part(torso, {side * .28f, .03f, -.41f}, {.19f, .42f, .24f}, 7, iron);
        part(torso, {side * .28f, .03f, -.55f}, {.07f, .24f, .035f}, 11, accent,
             .45f);
      }
      part(torso, {0, .53f, -.36f}, {.055f, .36f, .055f}, 4);
      part(torso, {0, .76f + float(sin(time * 13)) * .025f, -.36f},
           {.08f, .12f, .08f}, 11, accent, .6f);
    }
    if (boss) {
      part(torso, {0, .25f, -.28f}, {.62f, .58f, .16f}, 9, {.55f, .2f, .25f});
      for (int strip = 0; strip < 3; ++strip)
        part(torso, {(strip - 1) * .2f, -.35f - strip % 2 * .11f, -.27f},
             {.18f, .78f, .08f}, 9, {.6f, .22f, .24f});
      part(head, {0, .38f, -.04f}, {.12f, .30f, .19f}, 5, {1, .8f, .4f});
    }
  }
  if (e.burn > 0)
    for (int side : {-1, 1}) {
      auto flame = glm::translate(
          root, {side * .24f, 1.25f + float(sin(time * 14 + side)) * .06f, 0});
      part(flame, {0, 0, 0}, {.14f, .25f, .12f}, 11, {1, .6f, .2f}, .55f);
      part(flame, {side * .03f, .17f, 0}, {.07f, .14f, .07f}, 6, {1, .8f, .4f},
           .5f);
    }
  if (frozen)
    for (int side : {-1, 1})
      for (float z : {-.18f, .18f}) {
        auto ice = glm::rotate(glm::translate(root, {side * .31f, .55f, z}),
                               side * .2f, {0, 0, 1});
        part(ice, {0, 0, 0}, {.10f, .62f, .12f}, 10, {.45f, .8f, 1}, .12f);
      }
  currentFlash = 0;
  if (e.hp < e.maxHp) {
    float height = e.kind == 1 || e.kind == 6 ? 1.4f
                   : e.kind == 3              ? 1.8f
                   : e.kind == 8              ? 2.12f
                                              : 2.02f;
    float hp = std::clamp(e.hp / e.maxHp, 0.f, 1.f);
    part(root, {0, height, 0}, {.82f, .045f, .055f}, 7, dark);
    part(root, {-.41f + hp * .41f, height + .01f, 0},
         {std::max(.01f, .82f * hp), .035f, .06f}, 10, {1, .4f, .25f}, .2f);
  }
}

void WorldRenderer::environment(const game::Game &g) {
  glm::mat4 identity(1);
  auto core = glm::translate(identity, vector(g.world.base));
  for (int layer = 0; layer < 3; ++layer)
    part(core, {0, .13f + layer * .23f, 0},
         {2.3f - layer * .4f, .25f, 2.3f - layer * .4f}, 12);
  auto crystal = glm::rotate(
      glm::translate(core, {0, 1.6f + float(sin(g.time * 2)) * .12f, 0}),
      g.time * .5f, {0, 1, 0});
  crystal = glm::rotate(crystal, .6f, {0, 0, 1});
  part(crystal, {0, 0, 0}, {.7f, 1.25f, .7f}, 10, {.4f, 1, .9f}, 1.2f);
  for (int side : {-1, 1}) {
    part(core, {side * .95f, 1, 0}, {.25f, 2, .25f}, 12);
    part(core, {0, 1, side * .95f}, {.25f, 2, .25f}, 12);
  }
  for (const auto &path : g.world.routes) {
    auto gate = glm::translate(identity, vector(path[0]));
    for (int side : {-1, 1}) {
      part(gate, {side * 1.25f, 1.75f, 0}, {.5f, 3.5f, .6f}, 12);
      part(gate, {side * 1.25f, 3.6f, 0}, {.8f, .3f, .8f}, 9);
      part(gate, {side * .8f, 2.4f, 0}, {.12f, .5f, .2f}, 14, {.6f, .3f, 1}, 1);
    }
    part(gate, {0, 3.3f, 0}, {2.2f, .4f, .6f}, 3);
    part(gate, {0, 2.7f, 0}, {1.5f, .12f, .18f}, 14, {.8f, .4f, 1}, .8f);
  }
  // Distant silhouettes and crenellated towers extend the citadel past its
  // walls.
  for (int i = 0; i < 16; ++i) {
    float angle = i * 6.2831853f / 16;
    glm::vec3 p(16 + cos(angle) * 34, 0, 16 + sin(angle) * 34);
    auto building = glm::translate(identity, p);
    float h = 6 + (i * 7) % 10;
    part(building, {0, h * .5f, 0}, {4, h, 4}, 3, {.45f, .65f, .75f});
    part(building, {0, h + .5f, 0}, {4.6f, 1, 4.6f}, 9);
    for (int side : {-1, 1})
      part(building, {side * 1.7f, h + 1.5f, 0}, {.6f, 1.5f, 4}, 12);
  }
  // Pixel moon is part of the backdrop, with a stepped outline.
  if (!depthPass) {
    auto moon = glm::translate(identity, {-35, 35, -55});
    part(moon, {0, 0, 0}, {7, 9, .3f}, 13, {.65f, .85f, 1}, 1);
    part(moon, {0, 0, 0}, {9, 7, .3f}, 13, {.65f, .85f, 1}, 1);
  }
}
void WorldRenderer::scene(const game::Game &g) {
  renderMesh(terrain, glm::mat4(1));
  environment(g);
  for (const auto &t : g.towers)
    tower(t, g.time);
  for (const auto &point : g.resourcePoints) {
    if (!point.active || !g.resourcePointsAvailable())
      continue;
    auto pedestal = glm::translate(glm::mat4(1), vector(point.position));
    part(pedestal, {0, .07f, 0}, {.65f, .14f, .65f}, 12, {.6f, .68f, .68f});
    const glm::vec3 colors[] = {{1, .78f, .25f},
                                {1, .35f, .26f},
                                {.35f, 1, .62f},
                                {.35f, .65f, 1},
                                {1, .52f, .70f}};
    auto color = colors[int(point.kind)];
    auto item = glm::rotate(
        glm::translate(
            pedestal,
            {0, .48f + float(sin(g.time * 2 + point.position.x)) * .06f, 0}),
        g.time * .65f, {0, 1, 0});
    if (point.kind == game::PickupKind::Gold) {
      for (int coin = 0; coin < 4; ++coin)
        part(item, {(coin % 2 - .5f) * .19f, coin / 2 * .10f, 0},
             {.17f, .07f, .24f}, 5, color, .55f);
    } else {
      item = glm::rotate(item, .5f, {0, 0, 1});
      part(item, {0, .10f, 0}, {.22f, .40f, .22f}, 10, color, .7f);
    }
    for (int side : {-1, 1}) {
      part(pedestal, {side * .25f, .17f, 0}, {.035f, .04f, .52f}, 10, color,
           .35f);
      part(pedestal, {0, .17f, side * .25f}, {.52f, .04f, .035f}, 10, color,
           .35f);
    }
  }
  for (const auto &e : g.enemies)
    enemy(e, g.time, std::max(0, (g.wave / 5 - 1) % 3));
}
void WorldRenderer::draw(const game::Game &g, const glm::mat4 &vp,
                         glm::vec3 eye, int px, int py, int pz, bool pick,
                         bool valid) {
  int viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  int width = viewport[2], height = viewport[3];
  resize(std::max(160, int(width / pixelScale)),
         std::max(100, int(height / pixelScale)));
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  if (lighting) {
    depthPass = true;
    glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer);
    glViewport(0, 0, 1536, 1536);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(shadowProgram);
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "viewProjection"), 1,
                       GL_FALSE, glm::value_ptr(lightView));
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2, 4);
    scene(g);
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
  depthPass = false;
  glBindFramebuffer(GL_FRAMEBUFFER, sceneBuffer);
  glViewport(0, 0, sceneWidth, sceneHeight);
  glClearColor(.055f, .095f, .14f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(sceneProgram);
  glm::mat4 projection =
      glm::perspective(glm::radians(65.f), float(width) / height, .08f, 180.f);
  auto camera = glm::inverse(glm::inverse(projection) * vp);
  if (g.shake > 0) {
    float amount = g.shake * shakeStrength;
    camera = glm::rotate(camera, float(sin(g.time * 137)) * amount * .025f,
                         {0, 0, 1});
    camera =
        glm::translate(camera, {float(sin(g.time * 173)) * amount * .03f,
                                float(cos(g.time * 131)) * amount * .025f, 0});
  }
  currentView = projection * glm::inverse(camera);
  glUniformMatrix4fv(glGetUniformLocation(sceneProgram, "viewProjection"), 1,
                     GL_FALSE, glm::value_ptr(currentView));
  glUniformMatrix4fv(glGetUniformLocation(sceneProgram, "lightProjection"), 1,
                     GL_FALSE, glm::value_ptr(lightView));
  glUniform3fv(glGetUniformLocation(sceneProgram, "eye"), 1,
               glm::value_ptr(eye));
  glUniform1i(glGetUniformLocation(sceneProgram, "lighting"), lighting);
  glUniform1f(glGetUniformLocation(sceneProgram, "materialDetail"),
              materialDetail);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, atlas);
  glUniform1i(glGetUniformLocation(sceneProgram, "atlas"), 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, shadowTexture);
  glUniform1i(glGetUniformLocation(sceneProgram, "shadowMap"), 1);
  std::array<glm::vec3, 12> positions{}, colors{};
  std::vector<game::Vec3> lamps = g.world.lanterns;
  std::sort(lamps.begin(), lamps.end(), [&](auto a, auto b) {
    return glm::length(vector(a) - eye) < glm::length(vector(b) - eye);
  });
  for (size_t i = 0; i < std::min(size_t(10), lamps.size()); ++i) {
    positions[i] = vector(lamps[i]);
    bool crystal = g.world.at(int(lamps[i].x), int(lamps[i].y + .4f),
                              int(lamps[i].z)) == game::Crystal;
    float flicker = .92f + .08f * sin(g.time * 7 + lamps[i].x * 3 + lamps[i].z);
    colors[i] =
        (crystal ? glm::vec3(.18f, .65f, .9f) : glm::vec3(1.4f, .65f, .18f)) *
        flicker;
  }
  positions[10] = vector(g.world.base) + glm::vec3(0, 2, 0);
  colors[10] = {.2f, 1.1f, .9f};
  positions[11] = eye + glm::vec3(0, -.1f, 0);
  colors[11] =
      g.heldTower < 0 && g.weaponKick > .6f && g.equipped > 0 && g.equipped < 5
          ? glm::vec3(1.7f, .8f, .2f)
          : glm::vec3(.05f, .1f, .12f);
  glUniform3fv(glGetUniformLocation(sceneProgram, "lampPosition"), 12,
               glm::value_ptr(positions[0]));
  glUniform3fv(glGetUniformLocation(sceneProgram, "lampColor"), 12,
               glm::value_ptr(colors[0]));
  scene(g);
  glm::mat4 identity(1);
  for (const auto &p : g.particles) {
    float life = p.life / p.maxLife;
    part(identity, vector(p.position), glm::vec3(p.size * (.3f + .7f * life)),
         p.tile, {1, 1, 1}, life * 1.8f);
  }
  for (const auto &t : g.tracers) {
    glm::vec3 from = vector(t.from), to = vector(t.to), delta = to - from;
    if (glm::length(delta) < .001f)
      continue;
    int segments = t.element == 5 ? 6 : 1;
    glm::vec3 previous = from;
    for (int i = 1; i <= segments; ++i) {
      glm::vec3 end = glm::mix(from, to, float(i) / segments);
      if (i < segments)
        end +=
            glm::vec3(sin(i * 7 + g.time * 30), cos(i * 13 + g.time * 20), 0) *
            .18f;
      glm::vec3 d = end - previous, mid = (previous + end) * .5f;
      auto trail = glm::inverse(glm::lookAt(mid, end,
                                            std::abs(glm::normalize(d).y) > .95f
                                                ? glm::vec3(1, 0, 0)
                                                : glm::vec3(0, 1, 0)));
      part(trail, {0, 0, 0}, {.025f, .025f, glm::length(d)}, 10,
           elementColor(t.element), 2);
      previous = end;
    }
  }
  if (pick) {
    glm::vec3 color = valid ? glm::vec3(.3f, 1, .7f) : glm::vec3(1, .3f, .2f);
    auto marker = glm::translate(identity, {px + .5f, py + 1.035f, pz + .5f});
    if (g.heldTower >= 0) {
      game::Tower preview;
      preview.type = g.heldTower;
      preview.position = {px + .5f, py + 1.f, pz + .5f};
      preview.hp = preview.maxHp = 1;
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      tower(preview, g.time);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    for (float side : {-.47f, .47f}) {
      part(marker, {side, 0, 0}, {.04f, .03f, 1}, 10, color, .8f);
      part(marker, {0, 0, side}, {1, .03f, .04f}, 10, color, .8f);
    }
  }
  glClear(GL_DEPTH_BUFFER_BIT);
  heldWeapon(g, camera);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, width, height);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glUseProgram(postProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sceneTexture);
  glUniform1i(glGetUniformLocation(postProgram, "scene"), 0);
  glUniform1f(glGetUniformLocation(postProgram, "hurt"),
              std::max(0.f, 1 - g.sinceDamage * 2));
  glUniform1f(glGetUniformLocation(postProgram, "bloom"), bloom);
  glBindVertexArray(screenVao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  if (numbersEnabled) {
    auto *draw = ImGui::GetBackgroundDrawList();
    auto display = ImGui::GetIO().DisplaySize;
    for (const auto &number : g.damageNumbers) {
      glm::vec4 clip = currentView * glm::vec4(vector(number.position), 1);
      if (clip.w <= .1f)
        continue;
      glm::vec3 ndc = glm::vec3(clip) / clip.w;
      if (std::abs(ndc.x) > 1 || std::abs(ndc.y) > 1 || ndc.z > 1)
        continue;
      ImVec2 p((ndc.x * .5f + .5f) * display.x,
               (.5f - ndc.y * .5f) * display.y);
      char text[24];
      snprintf(text, sizeof(text), number.style == 1 ? "%d!" : "%d",
               std::max(1, int(number.damage)));
      ImU32 color = number.style == 1   ? IM_COL32(255, 222, 105, 255)
                    : number.style == 4 ? IM_COL32(145, 255, 106, 255)
                    : number.style == 3 ? IM_COL32(115, 225, 255, 255)
                    : number.style == 5 ? IM_COL32(210, 160, 255, 255)
                    : number.style == 2 ? IM_COL32(255, 164, 89, 255)
                    : number.style == 6 ? IM_COL32(255, 110, 130, 255)
                                        : IM_COL32(245, 244, 221, 255);
      float size =
          (number.style == 1 ? 23.f : 18.f) *
          (1 + .25f * std::sin(std::min(1.f, (.75f - number.life) / .15f) *
                               3.14159f));
      unsigned alpha = unsigned(std::clamp(number.life * 5, 0.f, 1.f) * 255);
      color = (color & 0x00ffffff) | (alpha << 24);
      draw->AddText(ImGui::GetFont(), size, {p.x + 2, p.y + 2},
                    IM_COL32(12, 17, 27, 220), text);
      draw->AddText(ImGui::GetFont(), size, p, color, text);
    }
  }
  glBindVertexArray(0);
}
} // namespace citadel::graphics
