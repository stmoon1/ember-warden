#include "Localization.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
namespace citadel::locale {
namespace {
Language current = Language::Chinese;
std::unordered_map<std::string, std::array<std::string, 2>> entries;
std::vector<std::string> fragments;
} // namespace
void load(const std::filesystem::path &path) {
  std::ifstream input(path);
  if (!input)
    throw std::runtime_error("Translation file missing");
  entries.clear();
  fragments.clear();
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line.empty() || line[0] == '#')
      continue;
    auto first = line.find('\t'), second = line.find('\t', first + 1);
    if (first == std::string::npos || second == std::string::npos)
      throw std::runtime_error("Invalid translation row");
    auto key = line.substr(0, first);
    entries[key] = {line.substr(first + 1, second - first - 1),
                    line.substr(second + 1)};
  }
  for (const auto &entry : entries)
    if (entry.first.size() >= 5)
      fragments.push_back(entry.first);
  std::sort(fragments.begin(), fragments.end(),
            [](const auto &a, const auto &b) { return a.size() > b.size(); });
}
Language language() { return current; }
void setLanguage(Language value) { current = value; }
const char *tr(const char *english) {
  if (current == Language::English)
    return english;
  auto found = entries.find(english);
  if (found == entries.end())
    return english;
  return found->second[current == Language::Chinese ? 0 : 1].c_str();
}
std::string message(const std::string &english) {
  if (current == Language::English)
    return english;
  auto exact = entries.find(english);
  if (exact != entries.end())
    return exact->second[current == Language::Chinese ? 0 : 1];
  std::string result = english;
  for (const auto &key : fragments) {
    auto position = result.find(key);
    if (position != std::string::npos)
      result.replace(position, key.size(),
                     entries.at(key)[current == Language::Chinese ? 0 : 1]);
  }
  return result;
}
} // namespace citadel::locale
