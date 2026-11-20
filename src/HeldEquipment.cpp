#include "Renderer.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace citadel::graphics {
void WorldRenderer::heldWeapon(const game::Game &g, const glm::mat4 &camera) {
  const glm::vec3 glove{.32f, .44f, .49f}, armor{.48f, .65f, .69f},
      brass{.78f, .65f, .41f}, steel{.72f, .81f, .84f};
  // Forearms run between an elbow outside the view and the actual wrist.
  // Fingers are built around the same local axis as each physical grip.
  auto limb = [&](const glm::mat4 &parent, glm::vec3 a, glm::vec3 b) {
    auto delta = b - a;
    auto basis = glm::inverse(glm::lookAt(
        (a + b) * .5f, b,
        std::abs(glm::normalize(delta).y) > .95f ? glm::vec3(1, 0, 0)
                                                 : glm::vec3(0, 1, 0)));
    part(parent * basis, {0, 0, 0}, {.105f, .105f, glm::length(delta)}, 7,
         glove);
    part(parent * basis, {0, .045f, -.025f},
         {.115f, .035f, glm::length(delta) * .55f}, 7, armor);
  };
  auto hand = [&](const glm::mat4 &parent, glm::vec3 anchor, int side,
                  float pitch) {
    auto rotation = glm::rotate(glm::mat4(1), pitch, {1, 0, 0});
    auto grip = glm::translate(parent, anchor) * rotation;
    auto wrist = anchor + glm::vec3(rotation *
                                    glm::vec4(side * .035f, -.085f, .025f, 0));
    auto cameraWrist =
        glm::vec3(glm::inverse(camera) * parent * glm::vec4(wrist, 1));
    limb(camera, {side * .43f, -.52f, -.14f}, cameraWrist);
    part(grip, {side * .035f, -.073f, .025f}, {.105f, .045f, .095f}, 5, brass);
    part(grip, {side * .035f, -.005f, .025f}, {.083f, .125f, .07f}, 7, glove);
    part(grip, {side * .04f, -.01f, .065f}, {.075f, .08f, .025f}, 7, armor);
    for (int finger = 0; finger < 4; ++finger) {
      float y = -.046f + finger * .026f;
      part(grip, {-side * .008f, y, .065f}, {.065f, .021f, .032f}, 7, armor);
      part(grip, {-side * .037f, y, .036f}, {.027f, .021f, .045f}, 7, glove);
    }
    auto thumb = glm::rotate(glm::translate(grip, {side * .06f, .044f, .035f}),
                             side * -.45f, {0, 0, 1});
    part(thumb, {0, 0, 0}, {.035f, .065f, .04f}, 7, armor);
  };
  float bob = g.moving ? std::sin(g.time * 10) * .008f
                       : std::sin(g.time * 1.7f) * .002f;
  float lower = g.equipmentRaise * g.equipmentRaise * .28f;
  if (g.heldTower >= 0) {
    auto root = glm::translate(camera, {.08f, -.28f + bob - lower, -.64f});
    root = glm::rotate(root, -.12f, {1, 0, 0});
    part(root, {0, 0, 0}, {.42f, .25f, .045f}, 5, glove);
    part(root, {0, .005f, .028f}, {.35f, .19f, .012f}, 10, {.12f, .43f, .5f},
         .18f);
    for (int i = -2; i <= 2; ++i) {
      part(root, {i * .056f, 0, .038f}, {.003f, .17f, .003f}, 10,
           {.25f, .8f, .85f}, .25f);
      part(root, {0, i * .035f, .039f}, {.32f, .003f, .003f}, 10,
           {.25f, .8f, .85f}, .25f);
    }
    auto hologram = glm::rotate(glm::translate(root, {0, .18f, -.025f}),
                                g.time * .4f, {0, 1, 0});
    glm::vec3 color =
        g.heldTower == 3 ? glm::vec3(1, .8f, .35f) : glm::vec3(.3f, .9f, 1);
    part(hologram, {0, 0, 0}, {.18f, .025f, .18f}, 10, color, .45f);
    part(hologram, {0, .06f, 0}, {.06f, .12f, .06f}, 10, color, .5f);
    if (g.heldTower == 4)
      part(hologram, {0, .10f, 0}, {.19f, .16f, .04f}, 10, color, .55f);
    else if (g.heldTower == 2 || g.heldTower == 3)
      part(hologram, {0, .15f, 0}, {.08f, .08f, .08f}, 10, color, .7f);
    else {
      part(hologram, {0, .12f, 0}, {.13f, .07f, .09f}, 10, color, .55f);
      part(hologram, {0, .12f, -.07f}, {.035f, .035f, .13f}, 10, color, .6f);
    }
    hand(root, {.18f, -.07f, .02f}, 1, -.25f);
    hand(root, {-.18f, -.07f, .02f}, -1, -.25f);
    return;
  }
  bool melee = game::Weapons[g.equipped].magazine == 0;
  float phase = g.swingTimer > 0 ? 1 - g.swingTimer / g.swingDuration : 0;
  float sweep = g.swingTimer > 0 ? std::sin(phase * 3.14159265f) : 0;
  float stroke = sweep * sweep;
  if (melee) {
    bool knife = g.equipped == 0, bat = g.equipped == 5;
    auto root = glm::translate(
        camera, knife ? glm::vec3(.27f - .09f * stroke,
                                  -.18f + bob - lower + .035f * stroke,
                                  -.65f - .18f * stroke)
                      : glm::vec3(.28f - .38f * stroke,
                                  -.27f + bob - lower + .12f * sweep,
                                  -.82f - .07f * stroke));
    root = glm::rotate(
        root, knife ? -.48f - .20f * stroke : -.22f + .3f * stroke, {1, 0, 0});
    root = glm::rotate(
        root, knife ? .18f + .35f * stroke : .18f + 1.1f * stroke, {0, 0, 1});
    if (!knife)
      root = glm::rotate(root, -.25f + 1.15f * stroke, {0, 1, 0});
    float handle = knife ? .16f : .34f;
    part(root, {0, 0, 0}, {.045f, handle, .045f}, 4, {.55f, .30f, .29f});
    for (int wrap = 0; wrap < (knife ? 4 : 8); ++wrap)
      part(root, {0, -handle * .5f + .015f + wrap * .04f, 0},
           {.05f, .012f, .05f}, 7, glove);
    part(root, {0, -handle * .5f, 0}, {.065f, .025f, .065f}, 5, brass);
    if (bat) {
      for (int section = 0; section < 5; ++section) {
        float radius = .045f + std::min(section, 3) * .012f;
        part(root, {0, .22f + section * .10f, 0},
             {radius * 2, .105f, radius * 2}, 4, {.8f, .62f, .35f});
      }
      part(root, {0, .69f, 0}, {.10f, .035f, .10f}, 4, brass);
    } else {
      float guard = handle * .5f + .018f;
      part(root, {0, guard, 0}, {knife ? .13f : .17f, .025f, .075f}, 5, brass);
      float length = knife ? .29f : .71f;
      for (int section = 0; section < 7; ++section) {
        float fraction = float(section) / 7;
        float width = (knife ? .072f : .06f) * (section < 5    ? 1.f
                                                : section == 5 ? .7f
                                                               : .35f);
        float curve = knife ? 0 : fraction * fraction * .05f;
        float y = guard + .02f + length * (fraction + 1.f / 14);
        part(root, {curve, y, 0}, {width, length / 7 + .002f, .018f}, 7, steel);
        part(root, {curve - width * .42f, y, .011f}, {.009f, length / 7, .008f},
             10, {.5f, .85f, .9f}, .24f);
      }
    }
    hand(root, {0, knife ? -.01f : .065f, 0}, 1, 0);
    if (!knife)
      hand(root, {0, -.09f, 0}, -1, 0);
    else {
      auto guardHand =
          glm::translate(camera, {-.24f, -.24f + bob - lower, -.68f});
      hand(guardHand, {0, 0, 0}, -1, -.45f);
    }
    if (!bat && g.swingTimer > 0 && phase > .15f && phase < .72f) {
      for (int trail = 1; trail <= 3; ++trail) {
        auto arc = glm::rotate(root, -trail * .065f, {0, 0, 1});
        part(arc, {-.025f, knife ? .25f : .57f, -.018f},
             {.008f, knife ? .20f : .40f, .006f}, 10, {.18f, .5f, .55f},
             .25f * (4 - trail));
      }
    }
    return;
  }
  const auto &spec = game::Weapons[g.equipped];
  float reloadPhase =
      g.reloadTimer > 0 ? std::clamp(1 - g.reloadTimer / spec.reload, 0.f, 1.f)
                        : 0;
  float reload = g.reloadTimer > 0 ? std::sin(reloadPhase * 3.14159265f) : 0;
  auto root = glm::translate(camera, {.23f, -.17f + bob - lower - reload * .14f,
                                      -.78f + g.weaponKick * .055f});
  root =
      glm::rotate(root, .045f + g.weaponKick * .07f - reload * .18f, {1, 0, 0});
  root = glm::rotate(root, reload * .38f, {0, 0, 1});
  bool machine = g.equipped == 2, sniper = g.equipped == 3,
       shotgun = g.equipped == 4;
  float barrel = sniper ? .67f : shotgun ? .48f : machine ? .46f : .36f;
  part(root, {0, 0, .015f}, {machine ? .13f : .10f, .115f, .31f}, 7, armor);
  part(root, {.045f, -.03f, .21f}, {.065f, .075f, .19f}, 7, glove);
  part(root, {.10f, -.03f, .31f}, {.065f, .075f, .13f}, 7, glove);
  part(root, {.13f, -.025f, .37f}, {.085f, .12f, .03f}, 7, glove);
  auto pistol =
      glm::rotate(glm::translate(root, {0, -.13f, .075f}), -.2f, {1, 0, 0});
  part(pistol, {0, 0, 0}, {.055f, .17f, .065f}, 4, {.55f, .3f, .29f});
  part(root, {0, -.07f, .005f}, {.045f, .05f, .015f}, 5, brass);
  part(root, {0, -.09f, .025f}, {.055f, .012f, .09f}, 5, glove);
  auto magazine = glm::translate(root, {0, -.14f - reload * .10f, -.08f});
  part(magazine, {0, 0, 0},
       {machine   ? .18f
        : shotgun ? .055f
                  : .065f,
        shotgun ? .06f : .19f, machine ? .15f : .095f},
       5, glove);
  float pump = shotgun ? g.weaponKick * .075f : 0;
  part(root, {0, 0, -.26f + pump}, {.095f, .095f, .27f}, 5, brass);
  for (int rib = 0; rib < 5; ++rib)
    part(root, {0, .047f, -.16f - rib * .045f + pump}, {.105f, .013f, .018f}, 7,
         glove);
  part(root, {0, .025f, -.38f - barrel * .5f}, {.035f, .035f, barrel}, 5,
       steel);
  if (shotgun)
    part(root, {0, -.025f, -.55f}, {.04f, .04f, .42f}, 5, glove);
  part(root, {0, .025f, -.38f - barrel}, {.065f, .065f, .045f}, 7, glove);
  part(root, {0, .025f, -.405f - barrel}, {.032f, .032f, .005f}, 7,
       {.08f, .11f, .13f});
  if (sniper) {
    part(root, {0, .105f, -.02f}, {.045f, .08f, .12f}, 5, brass);
    part(root, {0, .15f, -.07f}, {.085f, .085f, .30f}, 7, glove);
    part(root, {0, .15f, .085f}, {.065f, .065f, .012f}, 10, {.25f, .65f, .7f},
         .2f);
    part(root, {.08f, .02f, .04f + g.weaponKick * .07f}, {.07f, .035f, .035f},
         5, brass);
  } else {
    part(root, {0, .083f, .09f}, {.06f, .045f, .035f}, 7, glove);
    part(root, {0, .065f, -.34f}, {.02f, .055f, .025f}, 5, brass);
  }
  for (int i = 0; i < 3; ++i)
    part(root, {.055f, .012f, -.04f - i * .045f}, {.012f, .025f, .022f}, 10,
         {.25f, .8f, .85f}, .25f);
  hand(pistol, {0, .012f, 0}, 1, 0);
  glm::vec3 support = glm::mix(glm::vec3(-.015f, -.055f, -.29f + pump),
                               glm::vec3(-.04f, -.20f, -.08f), reload);
  hand(root, support, -1, 1.25f);
  if (g.weaponKick > .6f) {
    auto muzzle = glm::translate(root, {0, .025f, -.43f - barrel});
    part(muzzle, {0, 0, 0}, {.13f, .07f, .16f}, 6, {1, .7f, .3f}, 2);
    part(muzzle, {0, 0, 0}, {.06f, .15f, .10f}, 6, {1, .8f, .4f}, 2);
  }
}
} // namespace citadel::graphics
