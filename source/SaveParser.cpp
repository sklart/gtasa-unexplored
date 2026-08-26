#include "SaveParser.hpp"
#include "TagData.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>

namespace gtasa {
namespace {

constexpr std::size_t kNotFound = std::numeric_limits<std::size_t>::max();
constexpr std::size_t kPickupCount = 620;
constexpr std::size_t kPickupSize = 0x20;
constexpr std::size_t kStuntJumpSize = 0x44;

constexpr std::uint8_t kPickupOnce = 3;
constexpr std::uint8_t kPickupSnapshot = 20;
constexpr std::uint16_t kModelOyster = 953;
constexpr std::uint16_t kModelHorseshoe = 954;
constexpr std::uint16_t kModelSnapshot = 1253;
constexpr std::uint8_t kTagCompletedThreshold = 228;

bool inRange(const std::vector<std::uint8_t>& d, std::size_t off, std::size_t len) {
    return off <= d.size() && len <= d.size() - off;
}

std::uint16_t readU16(const std::vector<std::uint8_t>& d, std::size_t off) {
    return static_cast<std::uint16_t>(d[off]) |
           static_cast<std::uint16_t>(d[off + 1] << 8);
}

std::int16_t readI16(const std::vector<std::uint8_t>& d, std::size_t off) {
    return static_cast<std::int16_t>(readU16(d, off));
}

std::uint32_t readU32(const std::vector<std::uint8_t>& d, std::size_t off) {
    return static_cast<std::uint32_t>(d[off]) |
           (static_cast<std::uint32_t>(d[off + 1]) << 8) |
           (static_cast<std::uint32_t>(d[off + 2]) << 16) |
           (static_cast<std::uint32_t>(d[off + 3]) << 24);
}

float readF32(const std::vector<std::uint8_t>& d, std::size_t off) {
    std::uint32_t raw = readU32(d, off);
    float value = 0.0f;
    static_assert(sizeof(value) == sizeof(raw), "float32 required");
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

std::vector<std::size_t> findOccurrences(const std::vector<std::uint8_t>& d, const char* name) {
    const auto n = std::strlen(name);
    std::vector<std::size_t> out;
    if (n == 0 || d.size() < n + 1) return out;

    for (std::size_t i = 0; i + n < d.size(); ++i) {
        if (std::memcmp(d.data() + i, name, n) == 0 && d[i + n] == 0) {
            out.push_back(i);
        }
    }
    return out;
}

template <class Validator>
std::size_t findNamedPayload(const std::vector<std::uint8_t>& data, const char* name, Validator validator) {
    // DE stores named blocks. Older research shows a length-prefixed NUL-terminated
    // block name. 1.112 changed the container substantially, so deliberately try a
    // few small wrapper sizes after the name instead of baking an absolute offset.
    static constexpr std::array<std::size_t, 8> kPayloadSkips = {0, 4, 8, 12, 16, 20, 24, 32};
    const auto nameLen = std::strlen(name) + 1;

    for (const auto nameOff : findOccurrences(data, name)) {
        const auto afterName = nameOff + nameLen;
        for (const auto skip : kPayloadSkips) {
            const auto candidate = afterName + skip;
            if (candidate < data.size() && validator(candidate)) {
                return candidate;
            }
        }
    }
    return kNotFound;
}

bool plausibleCoordinate(float v) {
    return std::isfinite(v) && v > -10000.0f && v < 10000.0f;
}

} // namespace

const char* typeKey(CollectibleType type) {
    switch (type) {
        case CollectibleType::Tag: return "tag";
        case CollectibleType::Snapshot: return "snapshot";
        case CollectibleType::Horseshoe: return "horseshoe";
        case CollectibleType::Oyster: return "oyster";
        case CollectibleType::StuntJump: return "stunt_jump";
        default: return "unknown";
    }
}

ParseResult SaveParser::parseFile(const std::string& path) const {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        ParseResult r;
        r.error = "Cannot open save file";
        r.saveName = path;
        return r;
    }
    f.seekg(0, std::ios::end);
    const auto size = f.tellg();
    if (size <= 0 || size > 4 * 1024 * 1024) {
        ParseResult r;
        r.error = "Unexpected save file size";
        r.saveName = path;
        return r;
    }
    f.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!f) {
        ParseResult r;
        r.error = "Failed to read save file";
        r.saveName = path;
        return r;
    }
    return parseBytes(bytes, path);
}

std::size_t SaveParser::findPickups(const std::vector<std::uint8_t>& data) const {
    auto validator = [&](std::size_t off) {
        if (!inRange(data, off, kPickupCount * kPickupSize)) return false;
        int plausibleTypes = 0;
        int oysters = 0, horseshoes = 0, snapshots = 0;
        int plausibleCoords = 0;
        for (std::size_t i = 0; i < kPickupCount; ++i) {
            const auto p = off + i * kPickupSize;
            const auto type = data[p + 0x1C];
            const auto model = readU16(data, p + 0x18);
            if (type <= 24) ++plausibleTypes;
            if (model == kModelOyster && type == kPickupOnce) ++oysters;
            if (model == kModelHorseshoe && type == kPickupOnce) ++horseshoes;
            if (model == kModelSnapshot && type == kPickupSnapshot) ++snapshots;
            const auto x = static_cast<float>(readI16(data, p + 0x10)) / 8.0f;
            const auto y = static_cast<float>(readI16(data, p + 0x12)) / 8.0f;
            if (std::abs(x) < 5000.0f && std::abs(y) < 5000.0f) ++plausibleCoords;
        }
        return plausibleTypes > 560 && plausibleCoords > 580 &&
               oysters <= 50 && horseshoes <= 50 && snapshots <= 50;
    };
    return findNamedPayload(data, "PICKUPS", validator);
}

std::size_t SaveParser::findTags(const std::vector<std::uint8_t>& data) const {
    auto validator = [&](std::size_t off) {
        if (!inRange(data, off, 4)) return false;
        const auto count = readU32(data, off);
        if (count != 100) return false;
        return inRange(data, off + 4, count);
    };
    return findNamedPayload(data, "TAGS", validator);
}

std::size_t SaveParser::findStuntJumps(const std::vector<std::uint8_t>& data) const {
    auto validator = [&](std::size_t off) {
        if (!inRange(data, off, 4)) return false;
        const auto count = readU32(data, off);
        if (count < 1 || count > 256) return false;
        if (!inRange(data, off + 4, static_cast<std::size_t>(count) * kStuntJumpSize)) return false;
        int bools = 0;
        int coords = 0;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto j = off + 4 + static_cast<std::size_t>(i) * kStuntJumpSize;
            if ((data[j + 0x40] <= 1) && (data[j + 0x41] <= 1)) ++bools;
            const auto x = readF32(data, j + 0x00);
            const auto y = readF32(data, j + 0x04);
            if (plausibleCoordinate(x) && plausibleCoordinate(y)) ++coords;
        }
        return bools == static_cast<int>(count) && coords == static_cast<int>(count);
    };
    return findNamedPayload(data, "STUNTJUMPS", validator);
}

ParseResult SaveParser::parseBytes(const std::vector<std::uint8_t>& data, const std::string& name) const {
    ParseResult r;
    r.saveName = name;
    if (data.size() < 64 * 1024 || data.size() > 4 * 1024 * 1024) {
        r.error = "Not a plausible GTA SA DE save size";
        return r;
    }

    r.pickupsOffset = findPickups(data);
    r.tagsOffset = findTags(data);
    r.stuntJumpsOffset = findStuntJumps(data);

    if (r.pickupsOffset == kNotFound || r.tagsOffset == kNotFound || r.stuntJumpsOffset == kNotFound) {
        std::ostringstream ss;
        ss << "Unsupported/corrupt save: ";
        if (r.pickupsOffset == kNotFound) ss << "PICKUPS ";
        if (r.tagsOffset == kNotFound) ss << "TAGS ";
        if (r.stuntJumpsOffset == kNotFound) ss << "STUNTJUMPS ";
        ss << "not found";
        r.error = ss.str();
        return r;
    }

    // --- Tags ---
    const auto tagCount = readU32(data, r.tagsOffset);
    r.summary.tagsTotal = static_cast<int>(tagCount);
    for (std::uint32_t i = 0; i < tagCount; ++i) {
        const auto alpha = data[r.tagsOffset + 4 + i];
        const bool done = alpha > kTagCompletedThreshold;
        if (done) {
            ++r.summary.tagsCompleted;
        } else if (i < kTagPositions.size()) {
            r.missing.push_back({CollectibleType::Tag,
                                 static_cast<int>(i + 1),
                                 kTagPositions[i].x,
                                 kTagPositions[i].y,
                                 0.0f,
                                 false,
                                 false,
                                 static_cast<int>(i)});
        }
    }

    // --- Oysters / horseshoes / snapshots ---
    int missingOysters = 0;
    int missingHorseshoes = 0;
    int missingSnapshots = 0;
    for (std::size_t i = 0; i < kPickupCount; ++i) {
        const auto p = r.pickupsOffset + i * kPickupSize;
        const auto model = readU16(data, p + 0x18);
        const auto type = data[p + 0x1C];

        CollectibleType collectibleType;
        bool isCollectible = false;
        int* missingCounter = nullptr;
        if (model == kModelOyster && type == kPickupOnce) {
            collectibleType = CollectibleType::Oyster;
            isCollectible = true;
            missingCounter = &missingOysters;
        } else if (model == kModelHorseshoe && type == kPickupOnce) {
            collectibleType = CollectibleType::Horseshoe;
            isCollectible = true;
            missingCounter = &missingHorseshoes;
        } else if (model == kModelSnapshot && type == kPickupSnapshot) {
            collectibleType = CollectibleType::Snapshot;
            isCollectible = true;
            missingCounter = &missingSnapshots;
        }
        if (!isCollectible) continue;

        ++(*missingCounter);
        const auto x = static_cast<float>(readI16(data, p + 0x10)) / 8.0f;
        const auto y = static_cast<float>(readI16(data, p + 0x12)) / 8.0f;
        const auto z = static_cast<float>(readI16(data, p + 0x14)) / 8.0f;
        r.missing.push_back({collectibleType,
                             *missingCounter,
                             x, y, z,
                             false, false,
                             static_cast<int>(i)});
    }
    r.summary.oystersCompleted = std::max(0, r.summary.oystersTotal - missingOysters);
    r.summary.horseshoesCompleted = std::max(0, r.summary.horseshoesTotal - missingHorseshoes);
    r.summary.snapshotsCompleted = std::max(0, r.summary.snapshotsTotal - missingSnapshots);

    // --- Unique stunt jumps ---
    const auto jumpCount = readU32(data, r.stuntJumpsOffset);
    r.summary.stuntJumpsTotal = static_cast<int>(jumpCount);
    for (std::uint32_t i = 0; i < jumpCount; ++i) {
        const auto j = r.stuntJumpsOffset + 4 + static_cast<std::size_t>(i) * kStuntJumpSize;
        const bool done = data[j + 0x40] != 0;
        const bool found = data[j + 0x41] != 0;
        if (done) {
            ++r.summary.stuntJumpsCompleted;
            continue;
        }
        const auto x1 = readF32(data, j + 0x00);
        const auto y1 = readF32(data, j + 0x04);
        const auto z1 = readF32(data, j + 0x08);
        const auto x2 = readF32(data, j + 0x0C);
        const auto y2 = readF32(data, j + 0x10);
        r.missing.push_back({CollectibleType::StuntJump,
                             static_cast<int>(i + 1),
                             (x1 + x2) * 0.5f,
                             (y1 + y2) * 0.5f,
                             z1,
                             false,
                             found,
                             static_cast<int>(i)});
    }

    r.ok = true;
    return r;
}

} // namespace gtasa
