#pragma once
#include <filesystem>
#include <string>
namespace citadel::locale {
enum class Language { Chinese, English, Japanese };
void load(const std::filesystem::path &path);
Language language();
void setLanguage(Language value);
const char *tr(const char *english);
std::string message(const std::string &english);
} // namespace citadel::locale
