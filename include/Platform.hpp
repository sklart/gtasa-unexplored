#pragma once

#include "PoiCategories.hpp"
#include <cstdint>
#include <string>
#include <vector>
#ifdef __SWITCH__
#include <switch.h>
#endif
namespace gtasa {
inline constexpr const char* kAppDir = "sdmc:/switch/gtasa-unexplored";
inline constexpr std::uint64_t kGtaSaTitleId = 0x010065A014024000ULL;
struct AppConfig { std::string language{"ru"}; std::uint64_t uid0{}; std::uint64_t uid1{}; int preferredSlot{1}; bool showPoi{true}; PoiCategoryFilters poiCategoryFilters{true, true, true, true, true}; int collectibleViewMode{}; };
struct SaveEntry { int slot{-1}; std::string path; std::string displayName; bool fromBackup{}; };
struct SaveDiscovery { bool ok{}; std::string error; bool gameRunningOrBusy{}; bool usingBackup{}; std::vector<SaveEntry> saves;
#ifdef __SWITCH__
 AccountUid uid{};
#endif
};
class Platform { public: Platform(); ~Platform(); bool initialize(std::string& error); void shutdown(); AppConfig loadConfig() const; bool saveConfig(const AppConfig& cfg) const; SaveDiscovery discoverSaves(const AppConfig& cfg, bool forceProfilePicker); bool backupSave(const SaveEntry& save, const AppConfig& cfg) const; bool exportDiagnostics(const std::string& text) const; void log(const std::string& line) const;
#ifdef __SWITCH__
 static bool uidValid(const AccountUid& uid); AccountUid requestProfileSelection() const; AccountUid resolveUser(const AppConfig& cfg, bool forceProfilePicker) const; bool userHasTargetSave(AccountUid uid) const; bool mountTargetSave(AccountUid uid, bool readOnly) const; void unmountTargetSave() const;
#endif
 private: bool initialized_{}; static int slotFromName(const std::string& name); static bool plausibleSaveName(const std::string& name); static void collectCandidateFiles(const std::string& root, bool backup, std::vector<SaveEntry>& out, int depth); static bool ensureDir(const std::string& path); static bool copyFile(const std::string& src, const std::string& dst); static std::string uidFolder(std::uint64_t uid0, std::uint64_t uid1); };
}
