#pragma once
#include "Engine.h"
#include "Localization.h"
#include "game/Game.h"
#include "imgui.h"
#include <algorithm>
#include <cstdint>
#include <glad/gl.h>

namespace citadel::app {
struct MenuArtwork {
  unsigned emblem = 0, catalog = 0;
  MenuArtwork() {
    emblem = graphics::loadTexture(graphics::asset("icons/ember-warden.png"));
    catalog = graphics::loadTexture(graphics::asset("ui/supply-atlas.png"));
  }
  ~MenuArtwork() {
    glDeleteTextures(1, &emblem);
    glDeleteTextures(1, &catalog);
  }
  MenuArtwork(const MenuArtwork &) = delete;
  MenuArtwork &operator=(const MenuArtwork &) = delete;
};
inline void equipmentBar(game::Game &g, const MenuArtwork &art,
                         bool interactive) {
  using locale::tr;
  float slotWidth =
      std::clamp((ImGui::GetIO().DisplaySize.x - 100) / 13.f - 5, 42.f, 80.f);
  constexpr float slotHeight = 70;
  ImGui::SetNextWindowPos(
      {ImGui::GetIO().DisplaySize.x * .5f, ImGui::GetIO().DisplaySize.y - 12},
      ImGuiCond_Always, {.5f, 1});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10, 8});
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {5, 5});
  ImGui::Begin("##Equipment belt", nullptr,
               ImGuiWindowFlags_NoDecoration |
                   ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings |
                   (interactive ? 0 : ImGuiWindowFlags_NoInputs));
  ImGui::TextColored(
      {.65f, .88f, .81f, 1}, "%s",
      tr(g.heldTower >= 0
             ? "LMB / E deploy   |   Wheel switch   |   Hold Q to select"
             : "LMB attack   |   Wheel switch   |   Hold Q to select"));
  const char *keys[] = {"1", "2", "3", "4", "5", "6", "7",
                        "8", "9", "0", "-", "=", "T"};
  for (int row = 0; row < 2; ++row) {
    int count = row == 0 ? 7 : 6;
    for (int column = 0; column < count; ++column) {
      if (row || column)
        ImGui::SameLine();
      int slot = row == 0 ? column : 7 + column;
      int tile = slot;
      bool available = row == 0 ? g.owned[column] : g.blueprints[column] > 0;
      bool selected = row == 0 ? g.heldTower < 0 && g.equipped == column
                               : g.heldTower == column;
      const char *name = tr(row == 0 ? game::Weapons[column].name
                                     : game::TowerSpecs[column].name);
      ImGui::PushID(slot);
      auto origin = ImGui::GetCursorScreenPos();
      bool clicked =
          ImGui::InvisibleButton("equipment", {slotWidth, slotHeight});
      auto *draw = ImGui::GetWindowDrawList();
      draw->AddRectFilled(origin, {origin.x + slotWidth, origin.y + slotHeight},
                          selected ? IM_COL32(29, 65, 67, 250)
                                   : IM_COL32(17, 30, 38, 235));
      draw->AddRect(origin, {origin.x + slotWidth, origin.y + slotHeight},
                    selected ? IM_COL32(125, 231, 195, 255)
                             : IM_COL32(51, 74, 82, 255),
                    0, 0, selected ? 2.f : 1.f);
      float imageSize = 45;
      ImVec2 uv0{float(tile % 4) * .25f + .003f,
                 float(tile / 4) * .25f + .003f};
      draw->AddImage(
          ImTextureRef(ImTextureID(art.catalog)),
          {origin.x + (slotWidth - imageSize) * .5f, origin.y + 2},
          {origin.x + (slotWidth + imageSize) * .5f, origin.y + 2 + imageSize},
          uv0, {uv0.x + .244f, uv0.y + .244f},
          available ? IM_COL32_WHITE : IM_COL32(90, 103, 109, 180));
      ImGui::PushFont(nullptr, 13);
      draw->AddText({origin.x + 5, origin.y + 3}, IM_COL32(163, 190, 193, 255),
                    keys[slot]);
      ImVec2 textSize = ImGui::CalcTextSize(name);
      draw->AddText({origin.x + (slotWidth - textSize.x) * .5f, origin.y + 50},
                    available ? IM_COL32(225, 232, 226, 255)
                              : IM_COL32(111, 128, 133, 255),
                    name);
      std::string quantity = row == 1 ? std::to_string(g.blueprints[column])
                             : game::Weapons[column].magazine && available
                                 ? std::to_string(g.ammo[column])
                                 : "";
      draw->AddText(
          {origin.x + slotWidth - ImGui::CalcTextSize(quantity.c_str()).x - 5,
           origin.y + 3},
          IM_COL32(223, 190, 121, 255), quantity.c_str());
      ImGui::PopFont();
      if (interactive && available && clicked) {
        if (row == 0)
          g.equip(column);
        else
          g.equipTower(column);
      }
      if (interactive && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(name);
        if (!available)
          ImGui::TextUnformatted(tr("Purchase in the TAB shop first."));
        ImGui::EndTooltip();
      }
      ImGui::PopID();
    }
  }
  ImGui::End();
  ImGui::PopStyleVar(2);
}
inline void catalogImage(const MenuArtwork &art, int tile, ImVec2 size) {
  const ImVec2 uv0{float(tile % 4) * .25f + .002f,
                   float(tile / 4) * .25f + .002f};
  const ImVec2 uv1{uv0.x + .246f, uv0.y + .246f};
  ImGui::Image(ImTextureRef(ImTextureID(art.catalog)), size, uv0, uv1);
}
inline bool catalogCard(const MenuArtwork &art, int tile, const char *name,
                        int price, bool owned, bool selected, bool affordable,
                        float height) {
  using locale::tr;
  auto origin = ImGui::GetCursorScreenPos();
  float width = ImGui::GetContentRegionAvail().x;
  bool clicked = ImGui::InvisibleButton("item", {width, height});
  bool hovered = ImGui::IsItemHovered();
  auto *draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled(origin, {origin.x + width, origin.y + height},
                      selected  ? IM_COL32(28, 68, 69, 255)
                      : hovered ? IM_COL32(28, 47, 57, 255)
                                : IM_COL32(16, 28, 36, 255));
  draw->AddRect(origin, {origin.x + width, origin.y + height},
                selected ? IM_COL32(111, 222, 193, 255)
                         : IM_COL32(49, 72, 80, 255),
                0, 0, selected ? 2.f : 1.f);
  bool weapon = tile < 7;
  float imageWidth = std::min(width - 20, (height - 55) * (weapon ? 2.f : 1.f));
  float imageHeight = imageWidth / (weapon ? 2.f : 1.f);
  ImVec2 a{origin.x + (width - imageWidth) * .5f, origin.y + 5};
  ImVec2 uv0{float(tile % 4) * .25f + .002f,
             float(tile / 4) * .25f + (weapon ? .063f : .002f)};
  draw->AddImage(ImTextureRef(ImTextureID(art.catalog)), a,
                 {a.x + imageWidth, a.y + imageHeight}, uv0,
                 {uv0.x + .246f, uv0.y + (weapon ? .123f : .246f)});
  draw->AddText({origin.x + 12, origin.y + height - 47},
                IM_COL32(226, 235, 229, 255), name);
  std::string label =
      owned ? tr("Owned") : std::to_string(price) + "  " + tr("Credits");
  draw->AddText({origin.x + 12, origin.y + height - 24},
                owned        ? IM_COL32(119, 213, 179, 255)
                : affordable ? IM_COL32(227, 188, 109, 255)
                             : IM_COL32(156, 166, 169, 255),
                label.c_str());
  return clicked;
}
inline void weaponCatalog(game::Game &g, const MenuArtwork &art) {
  using locale::tr;
  static int selected = 1;
  const float height = ImGui::GetContentRegionAvail().y;
  const float detailWidth =
      std::clamp(ImGui::GetContentRegionAvail().x * .28f, 235.f, 340.f);
  ImGui::BeginChild("Weapon catalog", {-detailWidth - 12, 0});
  if (ImGui::BeginTable("Weapon classes", 3,
                        ImGuiTableFlags_SizingStretchSame)) {
    const char *labels[] = {"Melee weapons", "Automatic weapons",
                            "Precision & shotguns"};
    const int columns[3][3] = {{0, 5, 6}, {1, 2, -1}, {3, 4, -1}};
    float cardHeight = std::clamp((height - 60) / 3.f - 10, 120.f, 190.f);
    for (int column = 0; column < 3; ++column) {
      ImGui::TableNextColumn();
      ImGui::SeparatorText(tr(labels[column]));
      for (int item : columns[column]) {
        if (item < 0)
          continue;
        ImGui::PushID(item);
        if (catalogCard(art, item, tr(game::Weapons[item].name),
                        g.weaponPrice(item), g.owned[item], selected == item,
                        g.resources >= g.weaponPrice(item), cardHeight))
          selected = item;
        ImGui::PopID();
      }
    }
    ImGui::EndTable();
  }
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("Weapon details", {0, 0}, true);
  const auto &w = game::Weapons[selected];
  ImGui::TextDisabled("%s", tr("ARMORY // INSPECT"));
  ImGui::PushFont(nullptr, 26);
  ImGui::TextWrapped("%s", tr(w.name));
  ImGui::PopFont();
  float imageSize = std::min(180.f, ImGui::GetContentRegionAvail().x);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                       (ImGui::GetContentRegionAvail().x - imageSize) * .5f);
  catalogImage(art, selected, {imageSize, imageSize});
  ImGui::Separator();
  ImGui::Text("%s   %.0f x%d", tr("Damage"), w.damage, w.pellets);
  ImGui::Text("%s   %.1f", tr("Rate / sec"), 1 / w.interval);
  ImGui::Text("%s   %.1fm", tr("Range"), w.range);
  ImGui::Text("%s   %d", tr("Magazine"), g.magazine(selected));
  ImGui::Text("%s   %d", tr("Price"), g.weaponPrice(selected));
  ImGui::Spacing();
  if (g.owned[selected]) {
    bool equipped = g.heldTower < 0 && g.equipped == selected;
    ImGui::BeginDisabled(equipped);
    if (ImGui::Button(tr(equipped ? "Equipped" : "Equip"), {-1, 42}))
      g.equip(selected);
    ImGui::EndDisabled();
  } else {
    ImGui::BeginDisabled(g.resources < g.weaponPrice(selected));
    if (ImGui::Button(tr("Buy weapon"), {-1, 42}))
      g.buyWeapon(selected);
    ImGui::EndDisabled();
  }
  if (w.magazine) {
    ImGui::Text("%s   %d / %d", tr("Ammunition"), g.ammo[selected],
                g.reserve[selected]);
    std::string ammo =
        std::string(tr("Ammo +")) +
        std::to_string(int(g.magazine(selected) * 3 *
                           (1 + g.bonus(game::Bonus::AmmoPack)))) +
        " / " + std::to_string(g.ammoPrice(selected));
    ImGui::BeginDisabled(!g.owned[selected] ||
                         g.resources < g.ammoPrice(selected));
    if (ImGui::Button(ammo.c_str(), {-1, 38}))
      g.buyAmmo(selected);
    ImGui::EndDisabled();
  } else {
    ImGui::TextDisabled("%s", tr("No ammunition required"));
  }
  ImGui::Separator();
  ImGui::TextWrapped("%s",
                     tr("New guns include a loaded magazine and three spare "
                        "magazines. Melee weapons use no ammunition. Bat and "
                        "katana cleave. Sniper deals precise, heavy damage."));
  ImGui::BeginDisabled(g.resources < 35 || g.playerHp >= g.maxPlayerHp);
  if (ImGui::Button(tr("Medkit +50 HP / 35 credits"), {-1, 0})) {
    g.resources -= 35;
    g.playerHp = std::min(g.maxPlayerHp, g.playerHp + 50);
  }
  ImGui::EndDisabled();
  ImGui::EndChild();
}
inline void towerCatalog(game::Game &g, const MenuArtwork &art) {
  using locale::tr;
  static int selected = 0;
  // Relay / extractor / bulwark / tesla follow the artwork atlas order.
  const int tiles[] = {7, 8, 9, 10, 11, 12};
  float detailWidth =
      std::clamp(ImGui::GetContentRegionAvail().x * .28f, 235.f, 340.f);
  float cardHeight = std::clamp(
      (ImGui::GetContentRegionAvail().y - 35) * .5f - 10, 150.f, 250.f);
  ImGui::BeginChild("Tower catalog", {-detailWidth - 12, 0});
  if (ImGui::BeginTable("Tower grid", 3, ImGuiTableFlags_SizingStretchSame)) {
    for (int item = 0; item < int(game::TowerSpecs.size()); ++item) {
      ImGui::TableNextColumn();
      ImGui::PushID(item);
      if (catalogCard(art, tiles[item], tr(game::TowerSpecs[item].name),
                      g.towerPrice(item), g.blueprints[item] > 0,
                      selected == item, g.resources >= g.towerPrice(item),
                      cardHeight))
        selected = item;
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("Tower details", {0, 0}, true);
  const auto &tower = game::TowerSpecs[selected];
  ImGui::TextDisabled("%s", tr("DEFENCE // INSPECT"));
  ImGui::PushFont(nullptr, 26);
  ImGui::TextWrapped("%s", tr(tower.name));
  ImGui::PopFont();
  float imageSize = std::min(180.f, ImGui::GetContentRegionAvail().x);
  catalogImage(art, tiles[selected], {imageSize, imageSize});
  ImGui::TextWrapped("%s", tr(tower.role));
  ImGui::Separator();
  ImGui::Text("%s   %.0f", tr("Health"), tower.hp);
  ImGui::Text("%s   %.0f", tr("Damage"), tower.damage);
  ImGui::Text("%s   %.1fm", tr("Range"), tower.range);
  if (tower.income)
    ImGui::Text(tr("Produces %d credits after every cleared wave"),
                tower.income);
  ImGui::Text("%s   %d", tr("Price"), g.towerPrice(selected));
  ImGui::Text(tr("In inventory %d"), g.blueprints[selected]);
  ImGui::BeginDisabled(g.resources < g.towerPrice(selected));
  if (ImGui::Button(tr("Buy blueprint"), {-1, 42}))
    g.buyTower(selected);
  ImGui::EndDisabled();
  if (ImGui::Button(tr(g.selectedType == selected ? "Selected" : "Select"),
                    {-1, 38}))
    g.equipTower(selected);
  ImGui::Separator();
  ImGui::TextWrapped("%s",
                     tr("Close the shop, aim at grass and press E to deploy."));
  ImGui::TextWrapped(
      tr("Level 2 costs 75%% of base price, level 3 costs 125%%. Each "
         "level adds 50%% base HP, damage or income, 10%% range. Relays "
         "gain +10 percentage points of aura per level."));
  ImGui::TextWrapped("%s", tr("Aim at a tower within 7m and press U, or use "
                              "TAB > Installed towers > Upgrade."));
  ImGui::EndChild();
}
} // namespace citadel::app
