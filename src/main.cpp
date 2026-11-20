#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <glm/vec3.hpp>
#include <iostream>
#include <string>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "BestiaryUi.h"
#include "Engine.h"
#include "Localization.h"
#include "Renderer.h"
#include "ShopUi.h"
#include "Sound.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "imgui.h"
#include <glad/gl.h>
#include <stb_image_write.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {
using namespace citadel;
using locale::tr;
void applyStyle() {
  ImGui::StyleColorsDark();
  ImGui::GetStyle().FontScaleMain = 1.0f;
  auto &style = ImGui::GetStyle();
  style.WindowRounding = 0;
  style.WindowBorderSize = 1;
  style.ChildBorderSize = 1;
  style.GrabMinSize = 14;
  style.FrameRounding = 0;
  style.FrameBorderSize = 0;
  style.FramePadding = {12, 8};
  style.ScrollbarSize = 10;
  style.ItemSpacing = {12, 10};
  style.WindowPadding = {22, 20};
  style.Colors[ImGuiCol_WindowBg] = {.035f, .060f, .078f, .98f};
  style.Colors[ImGuiCol_ChildBg] = {.045f, .075f, .095f, .97f};
  style.Colors[ImGuiCol_Border] = {.18f, .28f, .31f, .8f};
  style.Colors[ImGuiCol_Separator] = {.18f, .28f, .31f, .7f};
  style.Colors[ImGuiCol_Text] = {.85f, .90f, .89f, 1};
  style.Colors[ImGuiCol_TextDisabled] = {.46f, .57f, .59f, 1};
  style.Colors[ImGuiCol_TitleBg] = {.04f, .075f, .09f, 1};
  style.Colors[ImGuiCol_TitleBgActive] = {.055f, .12f, .14f, 1};
  style.Colors[ImGuiCol_Button] = {.09f, .25f, .27f, 1};
  style.Colors[ImGuiCol_ButtonHovered] = {.14f, .37f, .36f, 1};
  style.Colors[ImGuiCol_ButtonActive] = {.18f, .45f, .41f, 1};
  style.Colors[ImGuiCol_Header] = {.09f, .22f, .24f, 1};
  style.Colors[ImGuiCol_HeaderHovered] = {.13f, .32f, .32f, 1};
  style.Colors[ImGuiCol_FrameBg] = {.07f, .13f, .16f, 1};
  style.Colors[ImGuiCol_CheckMark] = {.46f, .85f, .72f, 1};
  style.Colors[ImGuiCol_SliderGrab] = {.32f, .66f, .63f, 1};
  style.Colors[ImGuiCol_PlotHistogram] = {.36f, .77f, .62f, 1};
  style.Colors[ImGuiCol_Tab] = {.07f, .14f, .17f, 1};
  style.Colors[ImGuiCol_TabSelected] = {.12f, .28f, .30f, 1};
}
ImVec4 rarityColor(game::Rarity rarity) {
  if (rarity == game::Rarity::Gold)
    return {1, .8f, .24f, 1};
  if (rarity == game::Rarity::Silver)
    return {.76f, .85f, .92f, 1};
  return {.83f, .54f, .32f, 1};
}
const char *rarityName(game::Rarity rarity) {
  if (rarity == game::Rarity::Gold)
    return "Gold";
  if (rarity == game::Rarity::Silver)
    return "Silver";
  return "Bronze";
}
void languagePicker(graphics::Window &window) {
  int current = int(locale::language());
  const char *names[] = {"简体中文", "English", "日本語"};
  if (ImGui::Combo(tr("Language"), &current, names, 3)) {
    locale::setLanguage(locale::Language(current));
    glfwSetWindowTitle(window.nativeHandle(), tr("Ember Warden"));
  }
}
void displaySettings(graphics::Window &window,
                     app::FirstPersonController &controller,
                     graphics::WorldRenderer &renderer, Sound &sound) {
  languagePicker(window);
  bool fullscreen = window.fullscreen();
  if (ImGui::Checkbox(tr("Fullscreen / F11"), &fullscreen))
    window.setFullscreen(fullscreen);
  ImGui::SliderFloat(tr("Mouse sensitivity"), &controller.mouseSensitivityScale,
                     .1f, 2.f, "%.2fx");
  if (ImGui::CollapsingHeader(tr("Graphics & audio"))) {
    ImGui::SliderFloat(tr("Pixel scale"), &renderer.pixelScale, 1, 4, "%.0fx");
    ImGui::SliderFloat(tr("Material detail"), &renderer.materialDetail, 0, 1,
                       "%.2f");
    ImGui::SliderFloat(tr("Bloom"), &renderer.bloom, 0, 2, "%.1f");
    ImGui::SliderFloat(tr("Screen shake"), &renderer.shakeStrength, 0, 1,
                       "%.1f");
    ImGui::SliderFloat(tr("Volume"), &sound.volume, 0, 1, "%.1f");
    ImGui::Checkbox(tr("Lighting and shadows"), &renderer.lighting);
    ImGui::Checkbox(tr("Damage numbers"), &renderer.numbersEnabled);
  }
}
void augmentCodex(const game::Game &g, bool &open);
void mainMenu(graphics::Window &window, const app::MenuArtwork &art, int &level,
              bool &selectingCharacter, bool &settings, bool &help, bool &codex,
              bool &bestiary, const game::Game &g,
              app::FirstPersonController &controller,
              graphics::WorldRenderer &renderer, Sound &sound) {
  if (codex) {
    augmentCodex(g, codex);
    return;
  }
  if (bestiary) {
    app::monsterCodex(bestiary);
    return;
  }
  auto size = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
  ImGui::SetNextWindowSize(size, ImGuiCond_Always);
  ImGui::Begin("##Main menu", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings);
  auto *draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled({0, 0}, size, IM_COL32(8, 16, 24, 250));
  float artSize = std::min(size.x * .46f, size.y * .76f);
  ImVec2 artOrigin{size.x * .72f - artSize * .5f, (size.y - artSize) * .48f};
  draw->AddImage(ImTextureRef(ImTextureID(art.emblem)), artOrigin,
                 {artOrigin.x + artSize, artOrigin.y + artSize});
  draw->AddRectFilledMultiColor(
      {size.x * .43f, 0}, {size.x * .6f, size.y}, IM_COL32(8, 16, 24, 255),
      IM_COL32(8, 16, 24, 0), IM_COL32(8, 16, 24, 0), IM_COL32(8, 16, 24, 255));
  draw->AddLine({size.x * .075f, size.y - 65}, {size.x * .925f, size.y - 65},
                IM_COL32(57, 84, 89, 255));
  ImGui::SetCursorPos({size.x * .075f, size.y - 48});
  ImGui::TextDisabled("%s", tr("50 WAVES / ONE CITADEL / YOUR BUILD"));
  float width = std::min(490.f, size.x * .4f);
  ImGui::SetCursorPos({size.x * .075f, std::max(45.f, size.y * .11f)});
  ImGui::BeginChild("Menu content", {width, size.y * .76f});
  ImGui::TextColored({.58f, .84f, .77f, 1}, "%s",
                     tr("FIRST-PERSON / TOWER DEFENCE / ROGUELIKE"));
  ImGui::PushFont(nullptr, std::clamp(size.x * .037f, 36.f, 64.f));
  ImGui::TextUnformatted(tr("Ember Warden"));
  ImGui::PopFont();
  ImGui::TextDisabled("%s", tr("Hold the last light."));
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  if (settings) {
    ImGui::SeparatorText(tr("Settings"));
    displaySettings(window, controller, renderer, sound);
    if (ImGui::Button(tr("Back to main menu"), {-1, 46}))
      settings = false;
  } else if (help) {
    ImGui::SeparatorText(tr("Controls"));
    ImGui::TextWrapped("%s", tr("Equipment: 1-7 weapons; 8,9,0,-,=,T towers. "
                                "Wheel cycles owned items. Hold Q to click a "
                                "slot. LMB attacks or deploys the held item."));
    ImGui::TextWrapped(
        "%s",
        tr("Mouse look   WASD move   SHIFT sprint   F5 new run   ESC pause"));
    ImGui::TextWrapped(
        "%s",
        tr("F collects nearby resource points. X dismantles the aimed tower "
           "within 7m and refunds 60% of construction and upgrade spending."));
    ImGui::Spacing();
    ImGui::TextWrapped("%s",
                       tr("TAB shop   LMB attack   R reload   1-7 weapons   E "
                          "build   B preview   U upgrade   P pause"));
    ImGui::Spacing();
    ImGui::TextWrapped(
        "%s", tr("Hold the core against a growing siege. Start with a knife, "
                 "earn your arsenal, construct defences and combine elemental "
                 "upgrades. Every fifth wave brings a boss."));
    if (ImGui::Button(tr("Back to main menu"), {-1, 46}))
      help = false;
  } else {
    ImGui::TextWrapped(
        "%s",
        tr("Build your arsenal. Shape your fortress. Survive the siege."));
    ImGui::Spacing();
    ImGui::SeparatorText(tr("Battlefield"));
    ImGui::RadioButton(tr("Ruined grasslands"), &level, 0);
    ImGui::SameLine();
    ImGui::RadioButton(tr("Split canyon"), &level, 1);
    ImGui::Spacing();
    if (ImGui::Button(tr("Start game"), {-1, 58}))
      selectingCharacter = true;
    if (ImGui::Button(tr("Settings"), {-1, 46}))
      settings = true;
    if (ImGui::Button(tr("Controls"), {-1, 46}))
      help = true;
    if (ImGui::Button(tr("Augment codex"), {-1, 46}))
      codex = true;
    if (ImGui::Button(tr("Monster codex"), {-1, 46}))
      bestiary = true;
    if (ImGui::Button(tr("Quit game"), {-1, 46}))
      window.requestClose();
    ImGui::Spacing();
    languagePicker(window);
  }
  ImGui::EndChild();
  ImGui::End();
}
void heroPortrait(bool available) {
  auto origin = ImGui::GetCursorScreenPos();
  ImGui::InvisibleButton("portrait", {136, 200});
  auto *draw = ImGui::GetWindowDrawList();
  auto shade = [&](ImU32 active, ImU32 dormant) {
    return available ? active : dormant;
  };
  const ImU32 outline = IM_COL32(17, 29, 37, 255),
              dark =
                  shade(IM_COL32(33, 59, 71, 255), IM_COL32(36, 43, 49, 255));
  const ImU32 armor =
      shade(IM_COL32(65, 106, 121, 255), IM_COL32(53, 61, 67, 255));
  const ImU32 edge =
      shade(IM_COL32(136, 174, 178, 255), IM_COL32(74, 81, 85, 255));
  const ImU32 gold =
      shade(IM_COL32(185, 151, 88, 255), IM_COL32(68, 72, 76, 255));
  const ImU32 light =
      shade(IM_COL32(101, 225, 205, 255), IM_COL32(67, 77, 80, 255));
  auto block = [&](int x, int y, int w, int h, ImU32 color) {
    draw->AddRectFilled({origin.x + x * 4, origin.y + y * 4},
                        {origin.x + (x + w) * 4, origin.y + (y + h) * 4},
                        color);
  };
  block(3, 48, 29, 1, IM_COL32(18, 28, 34, 220));
  block(7, 47, 21, 1, IM_COL32(18, 28, 34, 220));
  // Layered cloak, helmet, pauldrons and equipment keep a clean 32-pixel
  // silhouette.
  block(5, 19, 7, 23, shade(IM_COL32(92, 49, 53, 255), dark));
  block(3, 25, 5, 16, shade(IM_COL32(69, 37, 45, 255), dark));
  block(2, 37, 5, 8, shade(IM_COL32(69, 37, 45, 255), dark));
  block(4, 43, 7, 3, shade(IM_COL32(92, 49, 53, 255), dark));
  block(13, 2, 8, 2, edge);
  block(10, 4, 14, 2, armor);
  block(9, 6, 16, 10, outline);
  block(10, 6, 14, 3, armor);
  block(11, 5, 12, 1, edge);
  block(10, 9, 14, 5, dark);
  block(12, 9, 10, 3, light);
  block(13, 9, 8, 1, shade(IM_COL32(180, 246, 226, 255), edge));
  block(9, 11, 3, 6, armor);
  block(22, 11, 3, 6, armor);
  block(11, 15, 12, 3, edge);
  block(13, 15, 8, 2, armor);
  block(10, 14, 2, 1, gold);
  block(22, 14, 2, 1, gold);
  block(14, 18, 7, 2, outline);
  block(12, 18, 11, 2, shade(IM_COL32(123, 66, 60, 255), dark));
  block(5, 20, 8, 6, dark);
  block(23, 20, 8, 6, dark);
  block(5, 20, 8, 2, edge);
  block(23, 20, 8, 2, edge);
  block(4, 22, 8, 3, armor);
  block(24, 22, 8, 3, armor);
  block(6, 25, 5, 1, gold);
  block(25, 25, 5, 1, gold);
  block(10, 20, 15, 13, dark);
  block(12, 21, 11, 7, armor);
  block(12, 21, 11, 1, edge);
  block(15, 22, 5, 4, dark);
  block(16, 22, 3, 3, light);
  block(12, 28, 11, 1, edge);
  for (int strap = 0; strap < 5; ++strap)
    block(12 + strap * 2, 22 + strap * 2, 2, 2, gold);
  block(5, 26, 5, 8, dark);
  block(26, 26, 5, 8, dark);
  block(4, 29, 6, 4, armor);
  block(26, 29, 6, 4, armor);
  block(4, 33, 6, 3, gold);
  block(26, 33, 6, 3, gold);
  block(5, 36, 4, 4, dark);
  block(27, 36, 4, 4, dark);
  for (int finger = 0; finger < 3; ++finger) {
    block(5 + finger, 39, 1, 2, edge);
    block(27 + finger, 39, 1, 2, edge);
  }
  block(10, 32, 15, 3, outline);
  block(11, 32, 13, 1, gold);
  block(16, 32, 4, 3, gold);
  block(17, 33, 2, 1, light);
  block(10, 35, 5, 4, armor);
  block(21, 35, 5, 4, armor);
  block(11, 35, 3, 1, gold);
  block(22, 35, 3, 1, gold);
  block(11, 39, 5, 8, dark);
  block(20, 39, 5, 8, dark);
  block(12, 40, 4, 3, edge);
  block(20, 40, 4, 3, edge);
  block(11, 43, 5, 3, armor);
  block(20, 43, 5, 3, armor);
  block(9, 46, 8, 2, outline);
  block(20, 46, 8, 2, outline);
  block(10, 46, 7, 1, edge);
  block(20, 46, 7, 1, edge);
  if (available) {
    block(28, 24, 3, 13, outline);
    block(27, 27, 5, 6, armor);
    block(29, 18, 2, 9, dark);
    block(29, 16, 2, 3, gold);
    block(28, 32, 3, 7, dark);
    block(27, 36, 4, 2, gold);
    block(7, 30, 2, 15, outline);
    block(6, 29, 4, 2, gold);
    block(7, 26, 2, 4, edge);
  }
}
void augmentIcon(int augment) {
  int category = 10;
  for (size_t i = 0; i < game::AugmentSets.size(); ++i) {
    const auto &members = game::AugmentSets[i].members;
    if (std::find(members.begin(), members.end(), game::Perk(augment)) !=
        members.end()) {
      category = int(i);
      break;
    }
  }
  auto origin = ImGui::GetCursorScreenPos();
  ImGui::Dummy({44, 44});
  auto *draw = ImGui::GetWindowDrawList();
  const ImU32 colors[] = {
      IM_COL32(236, 151, 82, 255),  IM_COL32(137, 213, 230, 255),
      IM_COL32(186, 168, 242, 255), IM_COL32(150, 207, 112, 255),
      IM_COL32(219, 117, 129, 255), IM_COL32(185, 204, 215, 255),
      IM_COL32(157, 219, 208, 255), IM_COL32(155, 183, 196, 255),
      IM_COL32(223, 188, 110, 255), IM_COL32(144, 205, 180, 255),
      IM_COL32(165, 192, 198, 255)};
  if (augment == int(game::Perk::Piercing))
    category = 11;
  if (augment == int(game::Perk::Ricochet))
    category = 12;
  ImU32 color = colors[std::min(category, 10)];
  draw->AddRectFilled(origin, {origin.x + 40, origin.y + 40},
                      IM_COL32(26, 41, 49, 255));
  auto rect = [&](float x, float y, float w, float h) {
    draw->AddRectFilled({origin.x + x, origin.y + y},
                        {origin.x + x + w, origin.y + y + h}, color);
  };
  auto line = [&](float x, float y, float a, float b) {
    draw->AddLine({origin.x + x, origin.y + y}, {origin.x + a, origin.y + b},
                  color, 2);
  };
  switch (category) {
  case 0:
    rect(18, 6, 5, 11);
    rect(12, 13, 16, 15);
    rect(15, 28, 10, 5);
    rect(9, 19, 4, 8);
    rect(27, 16, 4, 12);
    break;
  case 1:
    for (int side : {-1, 1}) {
      line(20, 7, 20, 33);
      line(9, 14, 31, 26);
      line(9, 26, 31, 14);
      rect(18 + side * 9, 9, 3, 3);
    }
    break;
  case 2:
    rect(21, 5, 8, 6);
    rect(17, 11, 8, 7);
    rect(13, 18, 12, 5);
    rect(17, 23, 6, 5);
    rect(13, 28, 6, 7);
    break;
  case 3:
    rect(16, 7, 9, 5);
    rect(18, 12, 5, 5);
    rect(12, 17, 17, 14);
    rect(15, 31, 11, 3);
    rect(9, 23, 4, 6);
    break;
  case 4:
    rect(18, 7, 5, 8);
    rect(14, 15, 13, 12);
    rect(17, 27, 7, 6);
    break;
  case 5:
    rect(7, 13, 23, 6);
    rect(28, 12, 6, 4);
    rect(13, 19, 6, 12);
    rect(8, 19, 5, 6);
    break;
  case 6:
    line(10, 30, 30, 8);
    line(12, 32, 32, 10);
    line(8, 23, 18, 33);
    rect(7, 31, 4, 5);
    break;
  case 7:
    rect(9, 15, 23, 18);
    rect(9, 8, 5, 9);
    rect(18, 8, 5, 9);
    rect(27, 8, 5, 9);
    rect(18, 25, 5, 8);
    break;
  case 8:
    draw->AddCircle({origin.x + 20, origin.y + 20}, 12, color, 8, 3);
    rect(18, 13, 4, 14);
    line(14, 16, 25, 16);
    line(14, 24, 25, 24);
    break;
  case 9:
    rect(10, 9, 22, 16);
    rect(14, 25, 14, 5);
    rect(18, 30, 6, 4);
    rect(18, 14, 6, 13);
    rect(13, 18, 16, 5);
    break;
  case 11:
    line(8, 30, 30, 8);
    line(29, 8, 29, 17);
    line(29, 8, 20, 8);
    rect(8, 12, 4, 4);
    rect(21, 26, 4, 4);
    break;
  case 12:
    line(7, 29, 21, 14);
    line(21, 14, 32, 20);
    line(32, 20, 26, 22);
    line(32, 20, 29, 14);
    rect(19, 12, 4, 4);
    break;
  default:
    rect(10, 10, 21, 21);
    rect(15, 5, 11, 5);
    rect(15, 31, 11, 5);
    rect(5, 15, 5, 11);
    rect(31, 15, 5, 11);
    draw->AddRectFilled({origin.x + 17, origin.y + 17},
                        {origin.x + 24, origin.y + 24},
                        IM_COL32(26, 41, 49, 255));
    break;
  }
}

void setPreview(const game::Game &g, int augment) {
  for (size_t i = 0; i < game::AugmentSets.size(); ++i) {
    const auto &set = game::AugmentSets[i];
    if (std::find(set.members.begin(), set.members.end(),
                  game::Perk(augment)) == set.members.end())
      continue;
    int pieces = g.setPieces(i);
    ImGui::TextColored({.8f, .73f, .45f, 1}, tr("%s  %d -> %d"), tr(set.name),
                       pieces, pieces + 1);
    for (const auto &tier : set.tiers)
      if (pieces < tier.pieces) {
        if (pieces + 1 >= tier.pieces) {
          ImGui::TextColored({.35f, .95f, .7f, 1},
                             tr("Activates %d-piece bonus"), tier.pieces);
          ImGui::TextWrapped(tr("%s"), tr(tier.description));
        } else
          ImGui::Text(tr("Next bonus: %d pieces"), tier.pieces);
        break;
      }
  }
}
void augmentCodex(const game::Game &g, bool &open) {
  static int category = [] {
    const char *value = std::getenv("CITADEL_CODEX_CATEGORY");
    return value ? std::clamp(std::atoi(value), -1,
                              int(game::CatalogCategory::Count) - 1)
                 : 0;
  }();
  static int selected = 0, rarity = 0;
  static ImGuiTextFilter search;
  auto display = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos(display * .5f, ImGuiCond_Always, {.5f, .5f});
  ImGui::SetNextWindowSize({std::min(1420.f, display.x - 40), display.y - 60},
                           ImGuiCond_Always);
  ImGui::Begin("##Augment codex", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoTitleBar);
  if (ImGui::Button(tr("Back to main menu")))
    open = false;
  ImGui::SameLine();
  ImGui::PushFont(nullptr, 27);
  ImGui::Text("%s  /  %zu", tr("Augment codex"), game::Perks.size());
  ImGui::PopFont();
  ImGui::TextWrapped("%s",
                     tr("All augments are visible here. Drafts only offer "
                        "unpicked augments whose trigger requirements are met. "
                        "Element reactions require every listed source."));
  ImGui::Separator();
  ImGui::BeginChild("Categories", {190, 0}, true);
  for (int i = 0; i < int(game::CatalogCategory::Count); ++i) {
    int count = i == int(game::CatalogCategory::Sets)
                    ? int(game::AugmentSets.size())
                    : 0;
    if (i != int(game::CatalogCategory::Sets))
      for (int id = 0; id < int(game::Perks.size()); ++id)
        if (game::categoryContains(game::CatalogCategory(i), game::Perk(id)))
          ++count;
    std::string label =
        std::string(tr(game::categoryName(game::CatalogCategory(i)))) + "  " +
        std::to_string(count);
    if (ImGui::Selectable(label.c_str(), category == i, 0, {0, 30}))
      category = i;
  }
  ImGui::Separator();
  if (ImGui::Selectable(tr("How to upgrade towers"), category == -1, 0,
                        {0, 34}))
    category = -1;
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("Codex content", {0, 0});
  if (category == -1) {
    ImGui::SeparatorText(tr("How to upgrade towers"));
    ImGui::TextWrapped(
        "%s", tr("Aim at an installed tower within 7m and press U, or open TAB "
                 "> Installed towers and click Upgrade. Towers have three "
                 "levels. Level 2 costs 75% of the base price; level 3 costs "
                 "125%, rounded down before discounts."));
    ImGui::TextWrapped(
        "%s", tr("Each level adds 50% base HP, damage and extractor income, "
                 "and 10% range. Relays gain 10 percentage points of aura per "
                 "level. Upgrading preserves existing damage."));
    for (int i = 0; i < int(game::TowerSpecs.size()); ++i) {
      const auto &tower = game::TowerSpecs[i];
      ImGui::SeparatorText(tr(tower.name));
      ImGui::TextWrapped("%s", tr(tower.role));
      ImGui::Text("%s: %d  |  L2: %d  |  L3: %d", tr("Price"), tower.price,
                  int(tower.price * .75f), int(tower.price * 1.25f));
      for (int level = 1; level <= 3; ++level)
        ImGui::Text("L%d  %s %.0f  |  %s %.0f  |  %s %.1fm  |  %s %d", level,
                    tr("Health"), tower.hp * (1 + .5f * (level - 1)),
                    tr("Damage"), tower.damage * (1 + .5f * (level - 1)),
                    tr("Range"), tower.range * (1 + .1f * (level - 1)),
                    tr("Income per wave"),
                    int(tower.income * (1 + .5f * (level - 1))));
      if (i == 4)
        ImGui::TextWrapped(
            "%s", tr("Bulwarks have no innate damage reflection. Spiked "
                     "rampart grants 50% reflection; Reactive armor grants 40% "
                     "to all towers. These bonuses stack."));
    }
  } else if (category == int(game::CatalogCategory::Sets)) {
    ImGui::TextWrapped(
        "%s",
        tr("Collect different augments in a set. Bonuses at 2, 4 and 6 pieces "
           "are cumulative. An augment may belong to more than one set."));
    for (const auto &set : game::AugmentSets) {
      ImGui::SeparatorText(tr(set.name));
      for (const auto &tier : set.tiers) {
        ImGui::TextColored({.85f, .73f, .43f, 1}, "%d %s", tier.pieces,
                           tr("Set pieces"));
        ImGui::TextWrapped("%s", tr(tier.description));
      }
      ImGui::TextDisabled("%s", tr("Set members"));
      for (auto member : set.members)
        ImGui::BulletText("%s", tr(game::Perks[int(member)].name));
    }
  } else {
    search.Draw(tr("Search"), 240);
    ImGui::SameLine();
    const char *rarities[] = {tr("All rarities"), tr("Bronze"), tr("Silver"),
                              tr("Gold")};
    ImGui::SetNextItemWidth(140);
    ImGui::Combo("##Codex rarity", &rarity, rarities, 4);
    std::vector<int> visible;
    for (int i = 0; i < int(game::Perks.size()); ++i)
      if (game::categoryContains(game::CatalogCategory(category),
                                 game::Perk(i)) &&
          (rarity == 0 || int(game::perkRarity(game::Perk(i))) == rarity - 1) &&
          (search.PassFilter(tr(game::Perks[i].name)) ||
           search.PassFilter(tr(game::Perks[i].description))))
        visible.push_back(i);
    ImGui::Text(tr("Showing %d augments"), int(visible.size()));
    if (!visible.empty() &&
        std::find(visible.begin(), visible.end(), selected) == visible.end())
      selected = visible.front();
    float detailWidth =
        std::clamp(ImGui::GetContentRegionAvail().x * .34f, 235.f, 340.f);
    ImGui::BeginChild("Augment list", {-detailWidth - 12, 0});
    if (ImGui::BeginTable("Codex cards", 2,
                          ImGuiTableFlags_SizingStretchSame)) {
      for (int id : visible) {
        ImGui::TableNextColumn();
        ImGui::PushID(id);
        ImGui::PushStyleColor(ImGuiCol_Border,
                              selected == id ? ImVec4(.45f, .8f, .7f, 1)
                                             : ImVec4(.18f, .28f, .31f, 1));
        ImGui::BeginChild("Card", {0, 230}, true);
        auto rarityColorValue = rarityColor(game::perkRarity(game::Perk(id)));
        ImGui::TextColored(rarityColorValue, "%s",
                           tr(rarityName(game::perkRarity(game::Perk(id)))));
        ImGui::TextWrapped("%s", tr(game::Perks[id].name));
        ImGui::TextWrapped("%s", tr(game::Perks[id].description));
        if (game::enabledElements(game::Perk(id)))
          ImGui::TextColored({.45f, .85f, .72f, 1}, "%s", tr("Base trigger"));
        if (ImGui::Button(tr("View details"), {-1, 30}))
          selected = id;
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopID();
      }
      ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Augment details", {0, 0}, true);
    if (!visible.empty()) {
      auto perk = game::Perk(selected);
      augmentIcon(selected);
      ImGui::TextColored(rarityColor(game::perkRarity(perk)), "%s",
                         tr(rarityName(game::perkRarity(perk))));
      ImGui::PushFont(nullptr, 24);
      ImGui::TextWrapped("%s", tr(game::Perks[selected].name));
      ImGui::PopFont();
      ImGui::Separator();
      ImGui::TextWrapped("%s", tr(game::Perks[selected].description));
      ImGui::SeparatorText(tr("Trigger requirements"));
      unsigned required = game::requiredElements(perk),
               enabled = game::enabledElements(perk);
      if (enabled)
        ImGui::TextWrapped("%s",
                           tr("This base augment adds an elemental trigger and "
                              "needs no elemental prerequisite."));
      else if (perk == game::Perk::StatusHunter)
        ImGui::TextWrapped("%s", tr("Requires fire, ice, poison or bleeding."));
      else if (game::requiresAnyAilment(perk))
        ImGui::TextWrapped("%s", tr("Requires at least one fire, ice, "
                                    "lightning, poison or bleeding source."));
      else if (!required)
        ImGui::TextWrapped("%s", tr("No elemental prerequisite."));
      if (perk == game::Perk::TeslaJumps)
        ImGui::TextWrapped(
            "%s", tr("Requires a Tesla blueprint or a living Tesla tower."));
      else if (perk == game::Perk::CryoDuration)
        ImGui::TextWrapped("%s", tr("Requires Glacial network, a Cryo "
                                    "blueprint or a living Cryo tower."));
      else if (perk == game::Perk::ShockRange)
        ImGui::TextWrapped("%s", tr("Requires lightning or Ricochet."));
      else {
        const char *requirements[] = {
            "Fire source: Incendiary or Furnace turrets.",
            "Ice source: Cryo rounds, Glacial network or a Cryo "
            "tower/blueprint.",
            "Lightning source: Static charge, Storm grid or a Tesla "
            "tower/blueprint.",
            "Poison source: Venom or Toxic industry.",
            "Bleeding source: Serrated edge.",
            "Penetration source: Armor piercing."};
        for (unsigned bit = 0; bit < 6; ++bit)
          if (required & (1u << bit))
            ImGui::TextWrapped("%s", tr(requirements[bit]));
      }
      ImGui::SeparatorText(tr("Set bonuses"));
      bool inSet = false;
      for (const auto &set : game::AugmentSets)
        if (std::find(set.members.begin(), set.members.end(), perk) !=
            set.members.end()) {
          inSet = true;
          ImGui::TextColored({.7f, .85f, .75f, 1}, "%s", tr(set.name));
          for (const auto &tier : set.tiers) {
            ImGui::Text("%d %s", tier.pieces, tr("Set pieces"));
            ImGui::TextWrapped("%s", tr(tier.description));
          }
        }
      if (!inSet)
        ImGui::TextDisabled("%s", tr("Independent augment"));
    }
    ImGui::EndChild();
  }
  ImGui::EndChild();
  ImGui::End();
}
bool shop(game::Game &g, const app::MenuArtwork &art) {
  ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize * .5f, ImGuiCond_Always,
                          {.5f, .5f});
  ImGui::SetNextWindowSize({std::min(1540.f, ImGui::GetIO().DisplaySize.x - 40),
                            std::min(950.f, ImGui::GetIO().DisplaySize.y - 60)},
                           ImGuiCond_Always);
  ImGui::Begin(tr("CITADEL // SUPPLY TERMINAL"), nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
  bool closeShop = false;
  ImGui::TextColored({.3f, .95f, .8f, 1}, tr("%d CREDITS"), g.resources);
  ImGui::SameLine();
  ImGui::Text(tr("Wave %d | TAB returns to combat"), g.wave);
  ImGui::SameLine();
  if (ImGui::Button(tr("Return to battle")))
    closeShop = true;
  ImGui::TextWrapped(
      tr("Simulation paused. Purchased towers become blueprints. Close the "
         "shop, aim at grass and press E to deploy."));
  if (ImGui::BeginTabBar("Stock")) {
    if (ImGui::BeginTabItem(tr("Weapons & ammunition"))) {
      app::weaponCatalog(g, art);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(tr("Tower blueprints"))) {
      app::towerCatalog(g, art);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(tr("Installed towers"))) {
      int dismantleIndex = -1;
      ImGui::TextWrapped(
          tr("Level 2 costs 75%% of base price, level 3 costs 125%%. Each "
             "level adds 50%% base HP, damage or income, 10%% range. Relays "
             "gain +10 percentage points of aura per level."));
      for (size_t i = 0; i < g.towers.size(); ++i) {
        auto &t = g.towers[i];
        ImGui::PushID(int(i));
        ImGui::Text(tr("%s L%d [%.0f, %.0f] HP %.0f / %.0f"),
                    tr(game::TowerSpecs[t.type].name), t.level, t.position.x,
                    t.position.z, t.hp, t.maxHp);
        int price = g.upgradePrice(i);
        ImGui::BeginDisabled(t.level >= 3 || g.resources < price);
        if (ImGui::Button((tr("Upgrade / ") + std::to_string(price)).c_str()))
          g.upgradeTower(i);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(g.resources < 20 || t.hp >= t.maxHp);
        if (ImGui::Button(tr("Repair 50% / 20")))
          g.repairTower(i);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button((std::string(tr("Dismantle +")) +
                           std::to_string(g.dismantleRefund(i)))
                              .c_str()))
          dismantleIndex = int(i);
        ImGui::PopID();
      }
      if (dismantleIndex >= 0)
        g.dismantleTower(size_t(dismantleIndex));
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(tr("Run upgrades"))) {
      for (int i = 0; i < int(game::Perk::Count); ++i)
        if (g.perkStacks[i]) {
          ImGui::TextColored(rarityColor(game::perkRarity(game::Perk(i))),
                             "%s | %s",
                             tr(rarityName(game::perkRarity(game::Perk(i)))),
                             tr(game::Perks[i].name));
          ImGui::TextWrapped(tr("%s"), tr(game::Perks[i].description));
        }
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(tr("Set bonuses"))) {
      ImGui::TextWrapped(tr(
          "Collect different augments in a set. Bonuses at 2, 4 and 6 pieces "
          "are cumulative. An augment may belong to more than one set."));
      for (size_t i = 0; i < game::AugmentSets.size(); ++i) {
        const auto &set = game::AugmentSets[i];
        int pieces = g.setPieces(i);
        std::string title = std::string(tr(set.name)) + " / " +
                            std::to_string(pieces) + tr(" of 8");
        if (ImGui::CollapsingHeader(title.c_str())) {
          for (const auto &tier : set.tiers) {
            ImGui::TextColored(
                pieces >= tier.pieces ? ImVec4(.35f, .95f, .7f, 1)
                                      : ImVec4(.6f, .64f, .67f, 1),
                tr("%d-piece %s"), tier.pieces,
                pieces >= tier.pieces ? tr("ACTIVE") : tr("locked"));
            ImGui::TextWrapped(tr("%s"), tr(tier.description));
          }
          for (auto member : set.members)
            ImGui::TextColored(g.perk(member) > 0 ? ImVec4(.35f, .95f, .7f, 1)
                                                  : ImVec4(.65f, .68f, .7f, 1),
                               tr("%s %s"), g.perk(member) > 0 ? "[+]" : "[ ]",
                               tr(game::Perks[int(member)].name));
        }
      }
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  ImGui::End();
  return closeShop;
}
} // namespace
int main() {
  try {
    using namespace citadel;
    locale::load(graphics::asset("i18n/translations.tsv"));
    if (const char *language = std::getenv("CITADEL_LANGUAGE")) {
      if (std::string(language) == "en")
        locale::setLanguage(locale::Language::English);
      if (std::string(language) == "ja")
        locale::setLanguage(locale::Language::Japanese);
    }
    graphics::Window window(1440, 900, tr("Ember Warden"),
                            std::getenv("CITADEL_WINDOWED") == nullptr);
    app::AppUi ui(window);
    app::MenuArtwork artwork;
    Sound sound;
    applyStyle();
    graphics::FirstPersonCamera camera({8, 4.65f, 12}, -65, -18);
    app::FirstPersonController controller;
    game::Game g;
    g.runSeed =
        uint32_t(std::chrono::steady_clock::now().time_since_epoch().count());
    g.reset(0);
    graphics::WorldRenderer renderer;
    renderer.upload(g.world);
    bool shopping = false, paused = false, started = false;
    int level = 0;
    bool selectingCharacter = false, menuSettings = false, menuHelp = false,
         menuCodex = false, menuBestiary = false;
    std::array<bool, GLFW_KEY_LAST + 1> previous{};
    double last = glfwGetTime();
    int frames = 0;
    const bool smoke = std::getenv("CITADEL_SMOKE_TEST") != nullptr;
    if (smoke)
      started = true;
    const bool showcase = std::getenv("CITADEL_SHOWCASE") != nullptr;
    if (showcase) {
      started = true;
      camera = graphics::FirstPersonCamera({8, 4.65f, 12}, -72, -6);
      g.resources = 1500;
      g.buyWeapon(1);
      g.buyWeapon(4);
      g.equip(1);
      g.start();
      g.wave = 5;
      g.playerHp = g.maxPlayerHp = 10000;
      for (int i = 0; i < 18; ++i) {
        game::Enemy e;
        e.id = 10000 + i;
        e.kind = i % 8;
        e.position = {7.f + (i % 6) * 1.5f, 3, 4.f + (i / 6) * 2};
        e.hp = e.maxHp = 120;
        e.speed = 0;
        g.enemies.push_back(e);
      }
      for (int i = 0; i < 6; ++i) {
        g.buyTower(i);
        g.selectedType = i;
        for (int z = 8; z < 27; ++z) {
          if (g.placementIssue(15 + i * 2, z).empty()) {
            g.place(15 + i * 2, z);
            break;
          }
        }
      }
      g.acquire(int(game::Perk::Incendiary));
      g.acquire(int(game::Perk::Shock));
      g.acquire(int(game::Perk::CannonSplash));
    }
    if (smoke && std::getenv("CITADEL_CHOICE_CHECK")) {
      started = true;
      g.acquire(int(game::Perk::Incendiary));
      g.acquire(int(game::Perk::TowerFire));
      g.acquire(int(game::Perk::BurnPower));
      g.finishWave();
      g.choices = {int(game::Perk::BurnDuration), int(game::Perk::CryoRounds),
                   int(game::Perk::Shock)};
    }
    if (smoke && std::getenv("CITADEL_BOSS_CHECK")) {
      started = true;
      g.wave = 5;
      g.finishWave();
    }
    if (smoke && std::getenv("CITADEL_MENU_CHECK")) {
      started = false;
      selectingCharacter = true;
    }
    if (smoke && std::getenv("CITADEL_VICTORY_CHECK")) {
      started = true;
      g.wave = g.completedWaves = game::Game::FinalWave;
      g.state = game::State::Won;
    }
    if (smoke && std::getenv("CITADEL_MODEL_CHECK")) {
      started = true;
      camera = graphics::FirstPersonCamera({14, 5.5f, 20}, -90, -9);
      g.playerPosition = {14, 4.65f, 20};
      for (int y = 3; y < 13; ++y)
        for (int z = 5; z < 23; ++z)
          for (int x = 6; x < 24; ++x)
            g.world.set(x, y, z, game::Air);
      g.world.rebuildMeshes();
      renderer.upload(g.world);
      g.enemies.clear();
      for (int kind = 0; kind < 9; ++kind) {
        game::Enemy enemy;
        enemy.id = 20000 + kind;
        enemy.kind = kind;
        enemy.position = {11.f + (kind % 3) * 3, 3, 14.f - (kind / 3) * 4};
        enemy.hp = enemy.maxHp = 200;
        enemy.stride = 1;
        enemy.phase = kind * .7f;
        g.enemies.push_back(enemy);
      }
    }
    if (smoke && std::getenv("CITADEL_HOME_CHECK"))
      started = false;
    if (smoke && std::getenv("CITADEL_SHOP_CHECK")) {
      started = true;
      shopping = true;
      g.resources = 600;
    }
    const int smokeFrames =
        std::getenv("CITADEL_SMOKE_FRAMES")
            ? std::max(3, std::atoi(std::getenv("CITADEL_SMOKE_FRAMES")))
            : 900;
    if (smoke && std::getenv("CITADEL_HAND_CHECK")) {
      started = true;
      g.resources = 10000;
      for (int i = 1; i < 7; ++i)
        g.buyWeapon(i);
      g.equip(std::clamp(std::atoi(std::getenv("CITADEL_HAND_CHECK")), 0, 6));
      g.equipmentRaise = 0;
      if (std::getenv("CITADEL_SWING_CHECK")) {
        g.swingDuration = .48f;
        g.swingTimer = .25f;
      }
    }
    if (smoke && std::getenv("CITADEL_HELD_TOWER_CHECK")) {
      started = true;
      g.resources = 10000;
      for (int i = 0; i < 6; ++i)
        g.buyTower(i);
      g.equipTower(
          std::clamp(std::atoi(std::getenv("CITADEL_HELD_TOWER_CHECK")), 0, 5));
      g.equipmentRaise = 0;
      camera = graphics::FirstPersonCamera({8, 4.65f, 12}, -65, -35);
    }
    if (smoke && std::getenv("CITADEL_CODEX_CHECK")) {
      started = false;
      menuCodex = true;
    }
    if (smoke && std::getenv("CITADEL_BESTIARY_CHECK")) {
      started = false;
      menuBestiary = true;
    }
    if (smoke && std::getenv("CITADEL_RESOURCE_CHECK")) {
      started = true;
      g.start();
      g.update(.02f);
      for (const auto &point : g.resourcePoints)
        if (point.active) {
          for (auto offset : std::array<game::Vec3, 4>{{{0, 1.65f, 1.6f},
                                                        {1.6f, 1.65f, 0},
                                                        {0, 1.65f, -1.6f},
                                                        {-1.6f, 1.65f, 0}}}) {
            auto eye = point.position + offset;
            if (g.playerCanStand(eye.x, eye.z)) {
              g.playerPosition = eye;
              float yaw = std::atan2(-offset.z, -offset.x) * 57.2957795f;
              camera =
                  graphics::FirstPersonCamera({eye.x, eye.y, eye.z}, yaw, -25);
              break;
            }
          }
          break;
        }
    }
    if (smoke && std::getenv("CITADEL_SPECIAL_CHECK")) {
      started = true;
      camera = graphics::FirstPersonCamera({14, 5.5f, 20}, -90, -9);
      g.playerPosition = {14, 4.65f, 20};
      for (int y = 3; y < 13; ++y)
        for (int z = 5; z < 23; ++z)
          for (int x = 6; x < 24; ++x)
            g.world.set(x, y, z, game::Air);
      g.world.rebuildMeshes();
      renderer.upload(g.world);
      g.enemies.clear();
      for (int kind = 9; kind < 12; ++kind) {
        game::Enemy enemy;
        enemy.id = 30000 + kind;
        enemy.kind = kind;
        enemy.position = {11.f + (kind - 9) * 3, 3, 12};
        enemy.hp = enemy.maxHp = 200;
        enemy.stride = 1;
        g.enemies.push_back(enemy);
      }
    }
    bool previousLeft = false;
    auto reset = [&]() {
      g.runSeed += 7919;
      g.reset(level);
      camera = graphics::FirstPersonCamera({8, 4.65f, 12}, -65, -18);
      renderer.upload(g.world);
      shopping = paused = false;
      started = true;
      selectingCharacter = false;
    };
    while (!window.shouldClose()) {
      window.pollEvents();
      double now = glfwGetTime();
      float dt = std::min(.05f, float(now - last));
      last = now;
      auto pressed = [&](int key) {
        bool down = window.keyDown(key);
        bool edge = down && !previous[key];
        previous[key] = down;
        return edge;
      };
      bool tab = pressed(GLFW_KEY_TAB), pause = pressed(GLFW_KEY_P),
           restart = pressed(GLFW_KEY_F5);
      bool escape = pressed(GLFW_KEY_ESCAPE),
           fullscreenKey = pressed(GLFW_KEY_F11);
      bool startWave = pressed(GLFW_KEY_SPACE), build = pressed(GLFW_KEY_E),
           toggleBuild = pressed(GLFW_KEY_B), reload = pressed(GLFW_KEY_R),
           upgrade = pressed(GLFW_KEY_U);
      bool dismantle = pressed(GLFW_KEY_X), collect = pressed(GLFW_KEY_F);
      std::array<bool, 7> select{};
      for (int i = 0; i < 7; ++i)
        select[i] = pressed(GLFW_KEY_1 + i);
      ui.beginFrame();
      if (fullscreenKey)
        window.setFullscreen(!window.fullscreen());
      if (escape) {
        if (!started) {
          selectingCharacter = false;
          menuSettings = menuHelp = menuCodex = menuBestiary = false;
        } else if (shopping)
          shopping = false;
        else if (g.state != game::State::Choosing &&
                 g.state != game::State::Lost && g.state != game::State::Won)
          paused = !paused;
      }
      if (restart && started)
        reset();
      bool lost = g.state == game::State::Lost;
      bool choosing = g.state == game::State::Choosing;
      bool finished = lost || g.state == game::State::Won;
      if (tab && started && !finished && !choosing) {
        shopping = !shopping;
        paused = false;
      }
      if (pause && started && !finished && !choosing) {
        paused = !paused;
        shopping = false;
      }
      bool active =
          started && !shopping && !paused && !finished && !choosing &&
          (smoke || glfwGetWindowAttrib(window.nativeHandle(), GLFW_FOCUSED));
      if (active) {
        for (int i = 0; i < 7; ++i)
          if (select[i])
            g.equip(i);
        if (toggleBuild) {
          if (g.heldTower >= 0)
            g.equip(g.equipped);
          else
            g.equipTower(g.selectedType);
        }
        const int towerKeys[] = {GLFW_KEY_8,     GLFW_KEY_9,     GLFW_KEY_0,
                                 GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_T};
        for (int i = 0; i < 6; ++i)
          if (pressed(towerKeys[i]))
            g.equipTower(i);
        if (ImGui::GetIO().MouseWheel != 0)
          g.cycleEquipment(ImGui::GetIO().MouseWheel > 0 ? -1 : 1);
        if (startWave)
          g.start();
        if (reload)
          g.reload();
      }
      bool choosingEquipment = active && window.keyDown(GLFW_KEY_Q);
      if (started && !shopping && !paused && !finished && !choosing)
        app::equipmentBar(g, artwork, choosingEquipment);
      bool building = g.heldTower >= 0;
      if (building)
        g.selectedType = g.heldTower;
      bool leftDown = glfwGetMouseButton(window.nativeHandle(),
                                         GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
      bool leftPressed = leftDown && !previousLeft;
      previousLeft = leftDown;
      controller.moveScale = 1 + g.bonus(game::Bonus::MoveSpeed);
      controller.update(window, camera, dt,
                        active && !smoke && !choosingEquipment,
                        active && !smoke && !choosingEquipment);
      // Keep the eye at standing height and slide against terrain and
      // structures.
      auto desired = camera.position();
      auto old = g.playerPosition;
      desired.y = 4.65f;
      if (!g.playerCanStand(desired.x, desired.z)) {
        if (g.playerCanStand(desired.x, old.z))
          desired.z = old.z;
        else if (g.playerCanStand(old.x, desired.z))
          desired.x = old.x;
        else
          desired = {old.x, old.y, old.z};
      }
      g.moving = glm::length(desired - glm::vec3(old.x, old.y, old.z)) > .0001f;
      camera.setPosition(desired);
      g.playerPosition = {desired.x, desired.y, desired.z};
      auto forward = camera.forward();
      int px = 0, py = 0, pz = 0;
      bool hit = g.world.raycast(g.playerPosition,
                                 {forward.x, forward.y, forward.z}, px, py, pz);
      if (active && !choosingEquipment && building && (build || leftPressed)) {
        if (hit && py == 2 &&
            game::length(game::Vec3{px + .5f, 4.65f, pz + .5f} -
                         g.playerPosition) < 7)
          g.place(px, pz);
        else
          g.message = "Aim at grass within 7m. Buy blueprints with TAB.";
      }
      size_t aimedTower = g.towers.size();
      if (active) {
        size_t nearest = g.towers.size();
        float best = 7;
        for (size_t i = 0; i < g.towers.size(); ++i) {
          game::Vec3 delta =
              g.towers[i].position + game::Vec3{0, 1, 0} - g.playerPosition;
          float along =
              delta.x * forward.x + delta.y * forward.y + delta.z * forward.z;
          if (along > 0 && along < best &&
              game::length(delta - game::Vec3{forward.x, forward.y, forward.z} *
                                       along) < .85f) {
            nearest = i;
            best = along;
          }
        }
        aimedTower = nearest;
        if (upgrade && nearest < g.towers.size())
          g.upgradeTower(nearest);
        if (dismantle && !choosingEquipment && nearest < g.towers.size()) {
          g.dismantleTower(nearest);
          aimedTower = g.towers.size();
        }
      }
      if (active && !choosingEquipment && collect) {
        int point = g.nearbyResourcePoint();
        if (point >= 0)
          g.collectResourcePoint(size_t(point));
      }
      int previousShots = g.shots, previousKills = g.kills;
      float previousHp = g.playerHp, previousMarker = g.hitMarker;
      if (active && !choosingEquipment && (showcase || leftDown) && !building)
        g.attack(g.playerPosition, {forward.x, forward.y, forward.z});
      if (active)
        g.update(dt);
      building = g.heldTower >= 0;
      if (g.shots > previousShots)
        sound.play(g.equipped);
      if (g.kills > previousKills)
        sound.play(8);
      else if (g.hitMarker > previousMarker)
        sound.play(7);
      if (g.playerHp < previousHp)
        sound.play(7);
      lost = g.state == game::State::Lost;
      choosing = g.state == game::State::Choosing;
      finished = lost || g.state == game::State::Won;
      auto [width, height] = window.framebufferSize();
      if (width <= 0 || height <= 0) {
        ui.render();
        continue;
      }
      glViewport(0, 0, width, height);
      glClearColor(.055f, .095f, .14f, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      bool valid = active && building && hit && py == 2 &&
                   g.placementIssue(px, pz).empty() &&
                   game::length(game::Vec3{px + .5f, 4.65f, pz + .5f} -
                                g.playerPosition) < 7;
      renderer.draw(g, camera.viewProjection(float(width) / height),
                    camera.position(), px, py, pz, active && building && hit,
                    valid);
      if (started) {
        ImGui::SetNextWindowPos({18, 18});
        ImGui::SetNextWindowSize({350, 0});
        ImGui::Begin(tr("BREACH PROTOCOL"), nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoInputs);
        ImGui::PushFont(nullptr, 22);
        ImGui::Text(tr("WAVE %02d / 50 %s | %d CREDITS"), g.wave,
                    g.wave % 5 == 0 ? tr("BOSS") : "", g.resources);
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Text(tr("HP %.0f / %.0f | Core %d / 20"), g.playerHp,
                    g.maxPlayerHp, g.health);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                              g.playerHp < g.maxPlayerHp * .3f
                                  ? ImVec4(.85f, .35f, .30f, 1)
                                  : ImVec4(.36f, .77f, .62f, 1));
        ImGui::ProgressBar(g.playerHp / g.maxPlayerHp, {-1, 7}, "");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                              ImVec4(.35f, .64f, .77f, 1));
        ImGui::ProgressBar(g.health / 20.f, {-1, 4}, "");
        ImGui::PopStyleColor();
        if (g.furyTime > 0)
          ImGui::Text(tr("Fury %.0fs"), g.furyTime);
        if (g.hasteTime > 0)
          ImGui::Text(tr("Haste %.0fs"), g.hasteTime);
        if (g.wardTime > 0)
          ImGui::Text(tr("Ward %.0fs"), g.wardTime);
        if (g.shield > 0)
          ImGui::Text(tr("Shield %.0f"), g.shield);
        if (building)
          ImGui::Text("%s | %s", tr("Holding blueprint"),
                      tr(game::TowerSpecs[g.heldTower].name));
        else
          ImGui::Text(tr("%s [%d]"), tr(game::Weapons[g.equipped].name),
                      g.equipped + 1);
        if (!building && game::Weapons[g.equipped].magazine)
          ImGui::Text(tr("AMMO %d / %d %s"), g.ammo[g.equipped],
                      g.reserve[g.equipped],
                      g.reloadTimer > 0 ? tr("RELOADING") : "");
        if (g.state == game::State::Preparing)
          ImGui::Text(tr("Prepare %.0fs | SPACE starts wave"), g.preparation);
        else
          ImGui::Text(tr("Enemies %zu | Spawned %d / %d"), g.enemies.size(),
                      g.spawnedInWave, g.currentWaveSize());
        ImGui::Text(tr("%s | Blueprints %d"),
                    tr(game::TowerSpecs[g.selectedType].name),
                    g.blueprints[g.selectedType]);
        ImGui::TextWrapped(tr("%s"), locale::message(g.message).c_str());
        ImGui::End();
        if (active && !choosingEquipment) {
          auto center = ImGui::GetIO().DisplaySize * .5f;
          auto *draw = ImGui::GetForegroundDrawList();
          ImU32 color = g.hitMarker > 0 ? IM_COL32(255, 96, 70, 255)
                                        : IM_COL32(185, 245, 232, 255);
          draw->AddLine({center.x - 8, center.y}, {center.x - 3, center.y},
                        color, 2);
          draw->AddLine({center.x + 3, center.y}, {center.x + 8, center.y},
                        color, 2);
          draw->AddLine({center.x, center.y - 8}, {center.x, center.y - 3},
                        color, 2);
          draw->AddLine({center.x, center.y + 3}, {center.x, center.y + 8},
                        color, 2);
        }
      }
      int nearbyPoint = active ? g.nearbyResourcePoint() : -1;
      if (nearbyPoint >= 0) {
        ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x * .5f,
                                 ImGui::GetIO().DisplaySize.y - 205},
                                ImGuiCond_Always, {.5f, 1});
        ImGui::Begin("Resource interaction", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("%s | %s",
                    tr(game::pickupName(g.resourcePoints[nearbyPoint].kind)),
                    tr("Press F to collect"));
        if (g.resourcePoints[nearbyPoint].kind == game::PickupKind::Gold)
          ImGui::Text(tr("Collect %d credits"), g.resourceGoldValue());
        else {
          const char *effects[] = {
              "", "Weapon hits +25% / 20s", "Movement speed +20% / 20s",
              "Incoming damage -20% / 20s", "Restore 30 HP"};
          ImGui::TextUnformatted(
              tr(effects[int(g.resourcePoints[nearbyPoint].kind)]));
        }
        ImGui::End();
      }
      if (active && aimedTower < g.towers.size()) {
        const auto &tower = g.towers[aimedTower];
        ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x * .5f,
                                 ImGui::GetIO().DisplaySize.y - 135},
                                ImGuiCond_Always, {.5f, 1});
        ImGui::Begin("Tower interaction", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("%s L%d | %s %.0f / %.0f",
                    tr(game::TowerSpecs[tower.type].name), tower.level,
                    tr("Health"), tower.hp, tower.maxHp);
        if (tower.level < 3)
          ImGui::Text(tr("Press U to upgrade / %d credits"),
                      g.upgradePrice(aimedTower));
        else
          ImGui::TextUnformatted(tr("Maximum tower level"));
        ImGui::Text(tr("Press X to dismantle / refund %d credits"),
                    g.dismantleRefund(aimedTower));
        ImGui::End();
      }
      if (active) {
        const game::Enemy *target = nullptr;
        float nearest = 28;
        for (const auto &enemy : g.enemies)
          if (enemy.hp > 0) {
            float distance = 0;
            if (enemy.rayHit(g.playerPosition,
                             {forward.x, forward.y, forward.z}, nearest,
                             distance) &&
                g.lineOfSight(g.playerPosition,
                              g.playerPosition +
                                  game::Vec3{forward.x, forward.y, forward.z} *
                                      distance)) {
              target = &enemy;
              nearest = distance;
            }
          }
        if (target) {
          ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x * .5f, 22},
                                  ImGuiCond_Always, {.5f, 0});
          ImGui::SetNextWindowSize({340, 0});
          ImGui::Begin("Target health", nullptr,
                       ImGuiWindowFlags_NoDecoration |
                           ImGuiWindowFlags_NoInputs |
                           ImGuiWindowFlags_AlwaysAutoResize);
          ImGui::Text("%s", tr(game::enemySpec(target->kind).name));
          ImGui::SameLine();
          ImGui::TextDisabled("%.0f / %.0f", target->hp, target->maxHp);
          ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                                ImVec4(.78f, .40f, .34f, 1));
          ImGui::ProgressBar(std::clamp(target->hp / target->maxHp, 0.f, 1.f),
                             {-1, 5}, "");
          ImGui::PopStyleColor();
          ImGui::TextDisabled(
              "%s", tr(game::goalName(game::enemySpec(target->kind).goal)));
          if (target->stolenCredits > 0)
            ImGui::Text(tr("Stolen credits carried: %d"),
                        target->stolenCredits);
          ImGui::End();
        }
      }
      if (shopping && shop(g, artwork))
        shopping = false;
      if (choosing) {
        ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize * .5f,
                                ImGuiCond_Always, {.5f, .5f});
        ImGui::SetNextWindowSize(
            {std::min(1120.f, ImGui::GetIO().DisplaySize.x - 40), 580});
        ImGui::Begin(tr("WAVE CLEARED // SELECT ONE UPGRADE"), nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Text(
            tr("Wave %d survived. One upgrade becomes permanent for this run."),
            g.completedWaves);
        if (g.bossReward())
          ImGui::TextWrapped(tr("%s"),
                             tr("Boss reward: three distinct GOLD augments."));
        else
          ImGui::TextWrapped(
              tr("%s"), tr("Base odds: Bronze 60% / Silver 30% / Gold 10%."));
        if (g.wave == game::Game::FinalWave)
          ImGui::TextWrapped(tr("%s"), tr("Final boss defeated. Choose your "
                                          "last augment to claim victory."));
        ImGui::Text(tr("%d unseen augments remaining"), g.remainingAugments());
        if (g.availableChoices() == 0) {
          ImGui::TextWrapped(tr("Every augment has been collected. Your full "
                                "build and set bonuses remain active."));
          if (ImGui::Button(tr("Continue to next wave"), {-1, 50}))
            g.continueWithoutUpgrade();
        } else {
          if (ImGui::BeginTable("choices", 3, ImGuiTableFlags_BordersInnerV)) {
            int selected = -1;
            for (int i = 0; i < 3; ++i) {
              ImGui::TableNextColumn();
              int id = g.choices[i];
              if (id < 0)
                continue;
              const auto &p = game::Perks[id];
              ImGui::PushID(i);
              auto rarity = game::perkRarity(game::Perk(id));
              auto frameColor = rarityColor(rarity);
              ImGui::PushStyleColor(ImGuiCol_Border,
                                    ImVec4(frameColor.x * .55f,
                                           frameColor.y * .55f,
                                           frameColor.z * .55f, .8f));
              ImGui::BeginChild("Augment description", {0, 270}, true);
              augmentIcon(id);
              ImGui::SameLine();
              ImGui::BeginGroup();
              ImGui::PushFont(nullptr, 15);
              ImGui::TextColored(rarityColor(rarity), tr("%s"),
                                 tr(rarityName(rarity)));
              ImGui::PopFont();
              ImGui::PushFont(nullptr, 21);
              ImGui::TextWrapped("%s", tr(p.name));
              ImGui::PopFont();
              ImGui::EndGroup();
              ImGui::Spacing();
              ImGui::TextWrapped(tr("%s"), tr(p.description));
              ImGui::Separator();
              setPreview(g, id);
              ImGui::EndChild();
              ImGui::PopStyleColor();
              if (ImGui::Button(tr("Acquire"), {-1, 46}))
                selected = i;
              ImGui::PopID();
            }
            ImGui::EndTable();
            if (selected >= 0)
              g.choose(selected);
          }
          ImGui::BeginDisabled(g.rerolls <= 0 || g.availableChoices() == 0);
          if (ImGui::Button((tr("Reroll / ") + std::to_string(g.rerolls) +
                             tr(" remaining"))
                                .c_str()))
            g.reroll();
          ImGui::EndDisabled();
        }
        ImGui::End();
      }
      if (!started && !selectingCharacter)
        mainMenu(window, artwork, level, selectingCharacter, menuSettings,
                 menuHelp, menuCodex, menuBestiary, g, controller, renderer,
                 sound);
      if ((!started && selectingCharacter) ||
          (started && (paused || finished))) {
        ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize * .5f,
                                ImGuiCond_Always, {.5f, .5f});
        ImGui::SetNextWindowSize(
            {std::min(820.f, ImGui::GetIO().DisplaySize.x - 40), 0});
        ImGui::Begin(tr("Ember Warden"), nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoTitleBar);
        ImGui::PushFont(nullptr, 27);
        ImGui::TextUnformatted(tr("Ember Warden"));
        ImGui::PopFont();
        ImGui::Separator();
        if (!started && selectingCharacter) {
          ImGui::SeparatorText(tr("Character selection"));
          if (ImGui::BeginTable("characters", 2,
                                ImGuiTableFlags_SizingStretchSame |
                                    ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableNextColumn();
            ImGui::PushID(0);
            ImGui::BeginChild("hero", {0, 330}, true);
            heroPortrait(true);
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::PushFont(nullptr, 25);
            ImGui::TextColored({.65f, .9f, .8f, 1}, tr("%s"), tr("Warden"));
            ImGui::PopFont();
            ImGui::TextUnformatted(tr("Selected"));
            ImGui::EndGroup();
            ImGui::TextWrapped(tr("%s"), tr("Balanced fighter. Starts with 100 "
                                            "HP, a knife and no credits."));
            ImGui::EndChild();
            ImGui::PopID();
            ImGui::TableNextColumn();
            ImGui::PushID(1);
            ImGui::BeginChild("hero", {0, 330}, true);
            heroPortrait(false);
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::TextDisabled(tr("%s"), tr("More characters"));
            ImGui::TextDisabled(tr("%s"), tr("Coming soon"));
            ImGui::EndGroup();
            ImGui::TextWrapped(
                tr("%s"),
                tr("More survivors will join the citadel in future updates."));
            ImGui::EndChild();
            ImGui::PopID();
            ImGui::EndTable();
          }
          if (ImGui::Button(tr("Enter battlefield"), {-1, 46}))
            reset();
          if (ImGui::Button(tr("Back")))
            selectingCharacter = false;
        } else if (finished && started) {
          if (g.state == game::State::Won)
            ImGui::TextColored({1, .82f, .3f, 1}, tr("%s"),
                               tr("Victory / all 50 waves cleared"));
          ImGui::Text(tr("RUN ENDED | %d waves cleared | %d kills"),
                      g.completedWaves, g.kills);
          if (ImGui::Button(tr("Select character and play again"), {-1, 46})) {
            started = false;
            selectingCharacter = true;
            shopping = paused = false;
          }
        } else if (paused) {
          if (ImGui::Button(tr("Resume"), {-1, 42}))
            paused = false;
        } else {
          ImGui::TextWrapped(
              tr("%s"),
              tr("Hold the core against a growing siege. Start with a knife, "
                 "earn your arsenal, construct defences and combine elemental "
                 "upgrades. Every fifth wave brings a boss."));
          ImGui::RadioButton(tr("Ruined grasslands"), &level, 0);
          ImGui::RadioButton(tr("Split canyon"), &level, 1);
          if (ImGui::Button(tr("Begin run"), {-1, 46}))
            selectingCharacter = true;
        }
        if (paused && ImGui::CollapsingHeader(tr("Controls"))) {
          ImGui::TextWrapped("%s",
                             tr("Equipment: 1-7 weapons; 8,9,0,-,=,T towers. "
                                "Wheel cycles owned items. Hold Q to click a "
                                "slot. LMB attacks or deploys the held item."));
          ImGui::TextWrapped(
              "%s", tr("TAB shop   LMB attack   R reload   1-7 weapons   E "
                       "build   B preview   U upgrade   P pause"));
          ImGui::TextWrapped("%s", tr("Mouse look   WASD move   SHIFT sprint   "
                                      "F5 new run   ESC pause"));
        }
        displaySettings(window, controller, renderer, sound);
        if (started && ImGui::Button(tr("Back to main menu"), {-1, 40})) {
          started = false;
          selectingCharacter = shopping = paused = false;
          menuSettings = menuHelp = menuCodex = menuBestiary = false;
        }
        ImGui::End();
      }
      if (started) {
        auto *map = ImGui::GetBackgroundDrawList();
        float size = 144, scale = size / game::World::Size;
        float left = ImGui::GetIO().DisplaySize.x - size - 22, top = 22;
        map->AddRectFilled({left - 8, top - 8},
                           {left + size + 8, top + size + 8},
                           IM_COL32(11, 20, 28, 235));
        map->AddRect({left - 8, top - 8}, {left + size + 8, top + size + 8},
                     IM_COL32(105, 146, 151, 255), 0.f, ImDrawFlags_None, 2.f);
        for (const auto &path : g.world.routes)
          for (size_t i = 1; i < path.size(); ++i)
            map->AddLine(
                {left + path[i - 1].x * scale, top + path[i - 1].z * scale},
                {left + path[i].x * scale, top + path[i].z * scale},
                IM_COL32(97, 113, 112, 255), 2);
        for (const auto &point : g.resourcePoints)
          if (point.active && g.resourcePointsAvailable()) {
            float x = left + point.position.x * scale,
                  z = top + point.position.z * scale;
            map->AddQuadFilled({x, z - 3}, {x + 3, z}, {x, z + 3}, {x - 3, z},
                               point.kind == game::PickupKind::Gold
                                   ? IM_COL32(255, 209, 70, 255)
                                   : IM_COL32(90, 223, 236, 255));
          }
        for (const auto &e : g.enemies)
          map->AddRectFilled(
              {left + e.position.x * scale - 2, top + e.position.z * scale - 2},
              {left + e.position.x * scale + 2, top + e.position.z * scale + 2},
              IM_COL32(255, 108, 105, 255));
        for (const auto &t : g.towers)
          map->AddRectFilled(
              {left + t.position.x * scale - 2, top + t.position.z * scale - 2},
              {left + t.position.x * scale + 2, top + t.position.z * scale + 2},
              IM_COL32(255, 215, 116, 255));
        map->AddCircleFilled({left + g.playerPosition.x * scale,
                              top + g.playerPosition.z * scale},
                             3, IM_COL32(115, 255, 212, 255));
        map->AddRectFilled({left + g.world.base.x * scale - 3,
                            top + g.world.base.z * scale - 3},
                           {left + g.world.base.x * scale + 3,
                            top + g.world.base.z * scale + 3},
                           IM_COL32(111, 222, 255, 255));
      }
      // UI buttons can change screens late in the frame. Apply the final state
      // before rendering so menus, shops and drafts always have a visible
      // pointer.
      bool captureMouse = started && !shopping && !paused &&
                          g.state != game::State::Choosing &&
                          g.state != game::State::Lost &&
                          g.state != game::State::Won && !choosingEquipment;
      controller.setMouseCapture(window, captureMouse && !smoke);
      ui.render();
      if (smoke && frames == 1)
        if (const char *capture = std::getenv("CITADEL_CAPTURE")) {
          std::vector<unsigned char> pixels(size_t(width) * height * 4);
          glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                       pixels.data());
          stbi_flip_vertically_on_write(1);
          if (!stbi_write_png(capture, width, height, 4, pixels.data(),
                              width * 4))
            throw std::runtime_error("Frame capture failed");
        }
      window.swapBuffers();
      if (smoke && ++frames >= smokeFrames) {
        std::cout << "smoke_ok perks=" << game::Perks.size()
                  << " frames=" << frames << " towers=" << g.towers.size()
                  << "\n";
        std::cout << "combat_check equipped=" << g.equipped
                  << " ammo=" << g.ammo[g.equipped] << " shots=" << g.shots
                  << " kills=" << g.kills
                  << " numbers=" << g.damageNumbers.size() << "\n";
        if (showcase && (g.shots == 0 || g.kills == 0))
          return 2;
        window.requestClose();
      }
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
  return 0;
}
