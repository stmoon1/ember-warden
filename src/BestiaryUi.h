#pragma once
#include "Localization.h"
#include "game/Game.h"
#include "imgui.h"
#include <algorithm>
#include <cstdlib>
#include <string>
namespace citadel::app {
inline void monsterPortrait(int kind, ImVec2 size) {
  auto origin = ImGui::GetCursorScreenPos();
  ImGui::Dummy(size);
  float scale = std::min(size.x / 36, size.y / 42);
  origin.x += (size.x - 36 * scale) * .5f;
  auto *draw = ImGui::GetWindowDrawList();
  auto block = [&](float x, float y, float w, float h, ImU32 color) {
    draw->AddRectFilled(
        {origin.x + x * scale, origin.y + y * scale},
        {origin.x + (x + w) * scale, origin.y + (y + h) * scale}, color);
  };
  const auto &spec = game::enemySpec(kind);
  ImU32 accent = ImGui::ColorConvertFloat4ToU32(
      {spec.color[0], spec.color[1], spec.color[2], 1});
  ImU32 dark = IM_COL32(33, 49, 58, 255), armor = IM_COL32(83, 111, 121, 255),
        edge = IM_COL32(134, 154, 154, 255), gold = IM_COL32(198, 158, 77, 255);
  block(4, 39, 28, 1, dark);
  if (kind == 1 || kind == 6) {
    block(5, 22, 23, 9, dark);
    block(8, 20, 16, 4, armor);
    block(25, 23, 7, 6, armor);
    block(29, 23, 2, 2, accent);
    for (int x : {7, 14, 24}) {
      block(x, 30, 3, 8, armor);
      block(x - 1, 37, 5, 2, edge);
    }
    if (kind == 6)
      for (int x : {8, 16, 24}) {
        block(x, 16, 4, 6, accent);
        block(x - 5, 28, 3, 9, edge);
      }
  } else {
    if (kind == 3)
      for (int side : {-1, 1})
        for (int step = 0; step < 4; ++step)
          block(17 + side * (8 + step * 2), 14 + step * 3, 5, 4, armor);
    block(11, 6, 14, 11, dark);
    block(12, 4, 12, 5, armor);
    block(14, 10, 3, 2, accent);
    block(20, 10, 3, 2, accent);
    block(10, 17, 16, 15, dark);
    block(12, 18, 12, 9, armor);
    block(17, 20, 3, 4, accent);
    block(7, 18, 4, 14, armor);
    block(26, 18, 4, 14, armor);
    block(11, 31, 6, 8, armor);
    block(20, 31, 6, 8, armor);
    block(10, 37, 8, 2, dark);
    block(20, 37, 8, 2, dark);
    if (kind == 2 || kind == 10) {
      block(4, 21, 8, 12, edge);
      block(7, 23, 2, 8, accent);
      block(8, 16, 7, 4, edge);
      block(23, 16, 7, 4, edge);
    }
    if (kind == 4 || kind == 5) {
      block(10, 3, 16, 5, dark);
      block(9, 28, 18, 9, dark);
      block(30, 13, 2, 25, gold);
      block(28, 7, 6, 7, accent);
    }
    if (kind == 5) {
      block(10, 19, 3, 13, accent);
      block(23, 19, 3, 13, accent);
    }
    if (kind == 7) {
      block(27, 20, 7, 13, gold);
      block(29, 16, 2, 5, accent);
    }
    if (kind == 8) {
      block(8, 2, 4, 9, gold);
      block(24, 2, 4, 9, gold);
      block(5, 17, 7, 7, edge);
      block(25, 17, 7, 7, edge);
      block(10, 29, 16, 7, IM_COL32(116, 45, 55, 255));
    }
    if (kind == 9) {
      block(17, 0, 4, 7, accent);
      block(8, 15, 3, 13, accent);
      block(27, 15, 3, 13, accent);
    }
    if (kind == 10) {
      block(30, 13, 2, 25, gold);
      block(26, 10, 9, 6, edge);
      block(13, 25, 10, 3, accent);
    }
    if (kind == 11) {
      block(9, 3, 18, 6, IM_COL32(64, 82, 55, 255));
      block(11, 13, 14, 4, dark);
      block(27, 25, 8, 11, gold);
      block(29, 23, 4, 3, accent);
    }
  }
}
inline void monsterCodex(bool &open) {
  using locale::tr;
  static int category = 0, selected = 0, previewWave = 5;
  static ImGuiTextFilter search;
  if (const char *check = std::getenv("CITADEL_BESTIARY_ENTRY"))
    selected =
        std::clamp(std::atoi(check), 0, int(game::EnemySpecs.size()) - 1);
  auto display = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos({display.x * .5f, display.y * .5f}, ImGuiCond_Always,
                          {.5f, .5f});
  ImGui::SetNextWindowSize({std::min(1400.f, display.x - 40), display.y - 60},
                           ImGuiCond_Always);
  ImGui::Begin("##Bestiary", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoCollapse);
  if (ImGui::Button(tr("Back to main menu")))
    open = false;
  ImGui::SameLine();
  ImGui::PushFont(nullptr, 27);
  ImGui::Text("%s / %zu", tr("Monster codex"), game::EnemySpecs.size());
  ImGui::PopFont();
  if (ImGui::BeginTabBar("Bestiary pages")) {
    if (ImGui::BeginTabItem(tr("Monsters"))) {
      ImGui::TextWrapped(
          "%s", tr("Target preferences are strict: crystal runners ignore "
                   "taunts; siege breakers never attack players or the "
                   "crystal; thieves only attack extractors."));
      ImGui::BeginChild("Enemy groups", {180, 0}, true);
      const char *categories[] = {"All monsters",    "Mixed targets",
                                  "Crystal only",    "Towers only",
                                  "Extractors only", "Bosses"};
      for (int i = 0; i < 6; ++i)
        if (ImGui::Selectable(tr(categories[i]), category == i, 0, {0, 34}))
          category = i;
      ImGui::Separator();
      ImGui::TextWrapped("%s",
                         tr("Base statistics scale with the preview wave. "
                            "Spawn requirements and boss timing still apply."));
      ImGui::EndChild();
      ImGui::SameLine();
      ImGui::BeginChild("Enemy content", {0, 0});
      search.Draw(tr("Search"), 220);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(240);
      ImGui::SliderInt(tr("Preview wave"), &previewWave, 1, 50);
      std::vector<int> visible;
      for (int kind = 0; kind < int(game::EnemySpecs.size()); ++kind) {
        const auto &s = game::EnemySpecs[kind];
        bool match =
            category == 0 ||
            (category == 1 && s.goal == game::EnemyGoal::Mixed && kind != 8) ||
            (category == 2 && s.goal == game::EnemyGoal::CrystalOnly) ||
            (category == 3 && s.goal == game::EnemyGoal::TowersOnly) ||
            (category == 4 && s.goal == game::EnemyGoal::ExtractorsOnly) ||
            (category == 5 && kind == 8);
        std::string content = std::string(tr(s.name)) + " " + tr(s.mechanic);
        if (match && search.PassFilter(content.c_str()))
          visible.push_back(kind);
      }
      if (!visible.empty() &&
          std::find(visible.begin(), visible.end(), selected) == visible.end())
        selected = visible.front();
      float detailWidth =
          std::clamp(ImGui::GetContentRegionAvail().x * .38f, 300.f, 420.f);
      ImGui::BeginChild("Enemy list", {-detailWidth - 12, 0});
      if (ImGui::BeginTable("Enemy cards", 2,
                            ImGuiTableFlags_SizingStretchSame)) {
        for (int kind : visible) {
          const auto &s = game::EnemySpecs[kind];
          ImGui::TableNextColumn();
          ImGui::PushID(kind);
          ImGui::BeginChild("Enemy card", {0, 190}, true);
          monsterPortrait(kind, {65, 100});
          ImGui::SameLine();
          ImGui::BeginGroup();
          ImGui::TextWrapped("%s", tr(s.name));
          ImGui::TextWrapped("%s", tr(game::goalName(s.goal)));
          ImGui::Text(tr("First wave: %d"), s.firstWave);
          ImGui::EndGroup();
          if (ImGui::Button(tr("View details"), {-1, 32}))
            selected = kind;
          ImGui::EndChild();
          ImGui::PopID();
        }
        ImGui::EndTable();
      }
      ImGui::EndChild();
      ImGui::SameLine();
      ImGui::BeginChild("Enemy details", {0, 0}, true);
      if (!visible.empty()) {
        const auto &s = game::EnemySpecs[selected];
        monsterPortrait(selected, {0 + ImGui::GetContentRegionAvail().x, 145});
        ImGui::PushFont(nullptr, 25);
        ImGui::TextWrapped("%s", tr(s.name));
        ImGui::PopFont();
        ImGui::TextColored({1, .77f, .4f, 1}, "%s", tr(game::goalName(s.goal)));
        ImGui::Text(tr("First wave: %d"), s.firstWave);
        ImGui::Text("%s %.0f | %s %.2f", tr("Health"),
                    game::waveEnemyHealth(previewWave) * s.healthScale,
                    tr("Movement speed"),
                    game::waveEnemySpeed(previewWave) * s.speedScale);
        ImGui::Text("%s %.0f | %s %d", tr("Damage"),
                    s.damage * (1 + .10f * (previewWave - 1)),
                    tr("Core breach"), s.coreDamage);
        ImGui::SeparatorText(tr("Enemy mechanics"));
        ImGui::TextWrapped("%s", tr(s.mechanic));
        ImGui::SeparatorText(tr("Counterplay"));
        ImGui::TextWrapped("%s", tr(s.counter));
        if (selected == 8)
          ImGui::TextWrapped("%s",
                             tr("Bosses appear on waves 5, 10, 15 and so on. "
                                "Siege pulse hits within 8m every 7s; other "
                                "variants summon hounds or regenerate 4% HP."));
      }
      ImGui::EndChild();
      ImGui::EndChild();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(tr("Field guide"))) {
      ImGui::SeparatorText(tr("Dismantle towers"));
      ImGui::TextWrapped(
          "%s",
          tr("Aim at a tower within 7m and press X, or open TAB > Installed "
             "towers > Dismantle. Refunds return 60% of actual construction "
             "and upgrade spending, rounded down. Repair spending is excluded. "
             "Destroyed towers cannot be dismantled."));
      ImGui::SeparatorText(tr("Resource points"));
      ImGui::TextWrapped(
          "%s",
          tr("Resource points only appear while living enemies remain. "
             "Preparation, empty intervals and wave-clear screens have no "
             "points. Collected points never respawn in the same wave."));
      ImGui::TextWrapped(
          "%s",
          tr("Each wave activates two gold caches and one blessing point. Walk "
             "within 2.2m and press F to collect. Each point can be collected "
             "once per wave. Active points are marked on the minimap."));
      ImGui::TextWrapped(
          "%s", tr("Gold caches grant 20 credits, increasing by 2 per wave up "
                   "to 40. Battle: weapon hit damage +25% for 20s. Wind: move "
                   "speed +20% for 20s. Guardian: incoming damage -20% for "
                   "20s. Healing: restore 30 HP. Recollecting a blessing "
                   "refreshes its duration without stacking its strength."));
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  ImGui::End();
}
} // namespace citadel::app
