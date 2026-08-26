#include "Platform.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace gtasa {
namespace {

std::string baseName(const std::string& path) {
    const auto p = path.find_last_of("/\\");
    return p == std::string::npos ? path : path.substr(p + 1);
}

std::string parentDir(const std::string& path) {
    const auto p = path.find_last_of("/\\");
    return p == std::string::npos ? std::string{} : path.substr(0, p);
}

bool isRegularFile(const std::string& path, const dirent* ent) {
#ifdef DT_REG
    if (ent->d_type == DT_REG) return true;
    if (ent->d_type != DT_UNKNOWN) return false;
#endif
    struct stat st{};
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool isDirectory(const std::string& path, const dirent* ent) {
#ifdef DT_DIR
    if (ent->d_type == DT_DIR) return true;
    if (ent->d_type != DT_UNKNOWN) return false;
#endif
    struct stat st{};
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

} // namespace

Platform::Platform() = default;
Platform::~Platform() { shutdown(); }

bool Platform::initialize(std::string& error) {
    if (initialized_) return true;
#ifndef __SWITCH__
    (void)error;
#endif
    ensureDir(kAppDir);
    ensureDir(std::string(kAppDir) + "/saves");
#ifdef __SWITCH__
    Result rc = accountInitialize(AccountServiceType_Administrator);
    if (R_FAILED(rc)) {
        std::ostringstream ss;
        ss << "accountInitialize failed: 0x" << std::hex << rc;
        error = ss.str();
        return false;
    }
#endif
    initialized_ = true;
    log("Platform initialized");
    return true;
}

void Platform::shutdown() {
    if (!initialized_) return;
#ifdef __SWITCH__
    accountExit();
#endif
    initialized_ = false;
}

AppConfig Platform::loadConfig() const {
    AppConfig cfg;
    std::ifstream f(std::string(kAppDir) + "/config.ini");
    std::string line;
    while (std::getline(f, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const auto key = trim(line.substr(0, eq));
        const auto value = trim(line.substr(eq + 1));
        try {
            if (key == "language" && (value == "ru" || value == "en")) cfg.language = value;
            else if (key == "uid0") cfg.uid0 = std::stoull(value, nullptr, 16);
            else if (key == "uid1") cfg.uid1 = std::stoull(value, nullptr, 16);
            else if (key == "slot") cfg.preferredSlot = std::max(1, std::min(10, std::stoi(value)));
        } catch (...) {
            // Ignore malformed config values. This file is not game data.
        }
    }
    return cfg;
}

bool Platform::saveConfig(const AppConfig& cfg) const {
    ensureDir(kAppDir);
    std::ofstream f(std::string(kAppDir) + "/config.ini", std::ios::trunc);
    if (!f) return false;
    f << "language=" << cfg.language << "\n";
    f << "uid0=" << std::hex << cfg.uid0 << "\n";
    f << "uid1=" << std::hex << cfg.uid1 << "\n";
    f << std::dec << "slot=" << cfg.preferredSlot << "\n";
    return static_cast<bool>(f);
}

#ifdef __SWITCH__
bool Platform::uidValid(const AccountUid& uid) {
    return accountUidIsValid(&uid);
}

AccountUid Platform::requestProfileSelection() const {
    struct UserReturnData {
        u64 result;
        AccountUid uid;
    } __attribute__((packed));

    UserReturnData out{};
    AppletHolder holder{};
    AppletStorage inputStorage{};
    AppletStorage outputStorage{};
    LibAppletArgs args{};
    u8 indata[0xA0]{};
    indata[0x96] = 1;

    Result rc = appletCreateLibraryApplet(&holder, AppletId_LibraryAppletPlayerSelect,
                                          LibAppletMode_AllForeground);
    if (R_FAILED(rc)) return {};

    libappletArgsCreate(&args, 0);
    rc = libappletArgsPush(&args, &holder);
    if (R_FAILED(rc)) {
        appletHolderClose(&holder);
        return {};
    }

    rc = appletCreateStorage(&inputStorage, sizeof(indata));
    if (R_SUCCEEDED(rc)) rc = appletStorageWrite(&inputStorage, 0, indata, sizeof(indata));
    if (R_SUCCEEDED(rc)) rc = appletHolderPushInData(&holder, &inputStorage);
    if (R_SUCCEEDED(rc)) rc = appletHolderStart(&holder);
    if (R_FAILED(rc)) {
        appletStorageClose(&inputStorage);
        appletHolderClose(&holder);
        return {};
    }

    while (appletHolderWaitInteractiveOut(&holder)) {}
    appletHolderJoin(&holder);
    rc = appletHolderPopOutData(&holder, &outputStorage);
    if (R_SUCCEEDED(rc)) rc = appletStorageRead(&outputStorage, 0, &out, sizeof(out));

    appletStorageClose(&outputStorage);
    appletStorageClose(&inputStorage);
    appletHolderClose(&holder);

    if (R_FAILED(rc) || out.result != 0 || !uidValid(out.uid)) return {};
    return out.uid;
}

AccountUid Platform::resolveUser(const AppConfig& cfg, bool forceProfilePicker) const {
    if (forceProfilePicker) return requestProfileSelection();

    AccountUid cached{};
    cached.uid[0] = cfg.uid0;
    cached.uid[1] = cfg.uid1;
    if (uidValid(cached)) return cached;

    AccountUid uid{};
    if (R_SUCCEEDED(accountGetLastOpenedUser(&uid)) && uidValid(uid)) return uid;
    uid = {};
    if (R_SUCCEEDED(accountGetPreselectedUser(&uid)) && uidValid(uid)) return uid;
    return requestProfileSelection();
}

bool Platform::userHasTargetSave(AccountUid uid) const {
    FsSaveDataInfoReader reader{};
    FsSaveDataInfo info{};
    s64 entries = 0;
    Result rc = fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);
    if (R_FAILED(rc)) return false;

    bool found = false;
    while (true) {
        rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &entries);
        if (R_FAILED(rc) || entries == 0) break;
        if (info.save_data_type == FsSaveDataType_Account &&
            info.application_id == kGtaSaTitleId &&
            info.uid.uid[0] == uid.uid[0] && info.uid.uid[1] == uid.uid[1]) {
            found = true;
            break;
        }
    }
    fsSaveDataInfoReaderClose(&reader);
    return found;
}

bool Platform::mountTargetSave(AccountUid uid, bool readOnly) const {
    const Result rc = readOnly
        ? fsdevMountSaveDataReadOnly("gtasa", kGtaSaTitleId, uid)
        : fsdevMountSaveData("gtasa", kGtaSaTitleId, uid);
    return R_SUCCEEDED(rc);
}

void Platform::unmountTargetSave() const {
    fsdevUnmountDevice("gtasa");
}
#endif

SaveDiscovery Platform::discoverSaves(const AppConfig& cfg, bool forceProfilePicker) {
    SaveDiscovery out;
#ifdef __SWITCH__
    const AccountUid uid = resolveUser(cfg, forceProfilePicker);
    out.uid = uid;
    if (!uidValid(uid)) {
        out.error = "Профиль не выбран / No profile selected";
        return out;
    }

    const bool hasSave = userHasTargetSave(uid);
    if (!hasSave) {
        out.error = "Для выбранного профиля нет сохранения GTA San Andreas DE";
        return out;
    }

    if (mountTargetSave(uid, true)) {
        collectCandidateFiles("gtasa:/", false, out.saves, 0);
        const auto uidDir = uidFolder(uid.uid[0], uid.uid[1]);
        ensureDir(std::string(kAppDir) + "/saves/" + uidDir);
        for (const auto& save : out.saves) {
            const auto dst = std::string(kAppDir) + "/saves/" + uidDir + "/" + baseName(save.path);
            copyFile(save.path, dst);
        }
        unmountTargetSave();
    } else {
        out.gameRunningOrBusy = true;
        out.usingBackup = true;
        const auto uidDir = uidFolder(uid.uid[0], uid.uid[1]);
        collectCandidateFiles(std::string(kAppDir) + "/saves/" + uidDir, true, out.saves, 0);
    }
#else
    (void)cfg;
    (void)forceProfilePicker;
    collectCandidateFiles(".", false, out.saves, 0);
#endif

    // De-duplicate and prefer files whose names encode slots.
    std::sort(out.saves.begin(), out.saves.end(), [](const SaveEntry& a, const SaveEntry& b) {
        if ((a.slot >= 0) != (b.slot >= 0)) return a.slot >= 0;
        if (a.slot != b.slot) return a.slot < b.slot;
        return a.path < b.path;
    });
    out.saves.erase(std::unique(out.saves.begin(), out.saves.end(), [](const SaveEntry& a, const SaveEntry& b) {
        return a.path == b.path;
    }), out.saves.end());

    if (out.saves.empty()) {
        out.error = out.usingBackup
            ? "Игра запущена, а резервной копии сохранения ещё нет. Закройте игру и один раз запустите GTASA Unexplored."
            : "Не удалось найти файлы сохранения GTA San Andreas DE";
        return out;
    }
    out.ok = true;
    return out;
}

bool Platform::backupSave(const SaveEntry& save, const AppConfig& cfg) const {
    const auto uidDir = uidFolder(cfg.uid0, cfg.uid1);
    const auto dir = std::string(kAppDir) + "/saves/" + uidDir;
    if (!ensureDir(dir)) return false;
    return copyFile(save.path, dir + "/" + baseName(save.path));
}

bool Platform::exportDiagnostics(const std::string& text) const {
    ensureDir(kAppDir);
    std::ofstream f(std::string(kAppDir) + "/diagnostics.txt", std::ios::trunc);
    if (!f) return false;
    f << text;
    return static_cast<bool>(f);
}

void Platform::log(const std::string& line) const {
    ensureDir(kAppDir);
    std::ofstream f(std::string(kAppDir) + "/log.txt", std::ios::app);
    if (f) f << line << "\n";
}

int Platform::slotFromName(const std::string& name) {
    std::string lower;
    lower.reserve(name.size());
    for (const char c : name) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    const auto pos = lower.find("gtasasf");
    if (pos == std::string::npos) return -1;
    std::size_t i = pos + 7;
    int value = 0;
    bool any = false;
    while (i < lower.size() && std::isdigit(static_cast<unsigned char>(lower[i]))) {
        any = true;
        value = value * 10 + (lower[i] - '0');
        ++i;
    }
    return any && value >= 1 && value <= 10 ? value : -1;
}

bool Platform::plausibleSaveName(const std::string& name) {
    std::string lower;
    lower.reserve(name.size());
    for (const char c : name) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower.find("gtasasf") != std::string::npos) return true;
    if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".sav") return true;
    return false;
}

void Platform::collectCandidateFiles(const std::string& root, bool backup,
                                     std::vector<SaveEntry>& out, int depth) {
    if (depth > 4) return;
    DIR* dir = opendir(root.c_str());
    if (!dir) return;
    while (auto* ent = readdir(dir)) {
        if (!std::strcmp(ent->d_name, ".") || !std::strcmp(ent->d_name, "..")) continue;
        std::string path = root;
        if (!path.empty() && path.back() != '/') path += '/';
        path += ent->d_name;
        if (isDirectory(path, ent)) {
            collectCandidateFiles(path, backup, out, depth + 1);
            continue;
        }
        if (!isRegularFile(path, ent)) continue;
        struct stat st{};
        if (stat(path.c_str(), &st) != 0) continue;
        if (st.st_size < 64 * 1024 || st.st_size > 4 * 1024 * 1024) continue;
        if (!plausibleSaveName(ent->d_name)) continue;
        out.push_back({slotFromName(ent->d_name), path, ent->d_name, backup});
    }
    closedir(dir);
}

bool Platform::ensureDir(const std::string& path) {
    if (path.empty()) return false;
    struct stat st{};
    if (stat(path.c_str(), &st) == 0) return S_ISDIR(st.st_mode);
    const auto parent = parentDir(path);
    if (!parent.empty() && parent != path) ensureDir(parent);
    if (mkdir(path.c_str(), 0777) == 0) return true;
    return errno == EEXIST;
}

bool Platform::copyFile(const std::string& src, const std::string& dst) {
    ensureDir(parentDir(dst));
    std::ifstream in(src, std::ios::binary);
    std::ofstream out(dst, std::ios::binary | std::ios::trunc);
    if (!in || !out) return false;
    out << in.rdbuf();
    return static_cast<bool>(out);
}

std::string Platform::uidFolder(std::uint64_t uid0, std::uint64_t uid1) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(16) << uid1 << std::setw(16) << uid0;
    return ss.str();
}

} // namespace gtasa
