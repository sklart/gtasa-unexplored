#include "SaveParser.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

using namespace gtasa;

static void putU16(std::vector<std::uint8_t>& d, std::size_t o, std::uint16_t v) {
    d[o] = static_cast<std::uint8_t>(v);
    d[o + 1] = static_cast<std::uint8_t>(v >> 8);
}
static void putU32(std::vector<std::uint8_t>& d, std::size_t o, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) d[o + i] = static_cast<std::uint8_t>(v >> (i * 8));
}
static void putF32(std::vector<std::uint8_t>& d, std::size_t o, float v) {
    std::uint32_t raw;
    std::memcpy(&raw, &v, 4);
    putU32(d, o, raw);
}
static void appendName(std::vector<std::uint8_t>& d, const char* s) {
    const auto n = std::strlen(s) + 1;
    const auto old = d.size();
    d.resize(old + 4 + n);
    putU32(d, old, static_cast<std::uint32_t>(n));
    std::memcpy(d.data() + old + 4, s, n);
}

int main() {
    std::vector<std::uint8_t> d(70 * 1024, 0xA5);

    appendName(d, "PICKUPS");
    const std::size_t pOff = d.size();
    d.resize(d.size() + 620 * 0x20, 0);

    // oyster at slot 5
    putU16(d, pOff + 5 * 0x20 + 0x10, static_cast<std::uint16_t>(100 * 8));
    putU16(d, pOff + 5 * 0x20 + 0x12, static_cast<std::uint16_t>(-200 * 8));
    putU16(d, pOff + 5 * 0x20 + 0x14, static_cast<std::uint16_t>(5 * 8));
    putU16(d, pOff + 5 * 0x20 + 0x18, 953);
    d[pOff + 5 * 0x20 + 0x1C] = 3;

    // horseshoe at slot 6
    putU16(d, pOff + 6 * 0x20 + 0x18, 954);
    d[pOff + 6 * 0x20 + 0x1C] = 3;

    // snapshot at slot 7
    putU16(d, pOff + 7 * 0x20 + 0x18, 1253);
    d[pOff + 7 * 0x20 + 0x1C] = 20;

    appendName(d, "TAGS");
    const auto tOff = d.size();
    d.resize(d.size() + 4 + 100, 0);
    putU32(d, tOff, 100);
    d[tOff + 4] = 255; // tag #1 completed

    appendName(d, "STUNTJUMPS");
    const auto jOff = d.size();
    d.resize(d.size() + 4 + 70 * 0x44, 0);
    putU32(d, jOff, 70);
    for (int i = 0; i < 70; ++i) {
        const auto j = jOff + 4 + i * 0x44;
        putF32(d, j + 0x00, static_cast<float>(i));
        putF32(d, j + 0x04, static_cast<float>(i * 2));
        putF32(d, j + 0x08, 10.0f);
        putF32(d, j + 0x0C, static_cast<float>(i + 2));
        putF32(d, j + 0x10, static_cast<float>(i * 2 + 2));
        d[j + 0x40] = i == 0 ? 1 : 0;
        d[j + 0x41] = i < 4 ? 1 : 0;
    }

    SaveParser parser;
    auto r = parser.parseBytes(d, "synthetic");
    if (!r.ok) {
        std::cerr << r.error << "\n";
        return 1;
    }
    assert(r.summary.tagsCompleted == 1);
    assert(r.summary.snapshotsCompleted == 49);
    assert(r.summary.horseshoesCompleted == 49);
    assert(r.summary.oystersCompleted == 49);
    assert(r.summary.stuntJumpsTotal == 70);
    assert(r.summary.stuntJumpsCompleted == 1);

    int tagMissing = 0, snapshotMissing = 0, horseMissing = 0, oysterMissing = 0, jumpMissing = 0;
    for (const auto& c : r.missing) {
        switch (c.type) {
            case CollectibleType::Tag: ++tagMissing; break;
            case CollectibleType::Snapshot: ++snapshotMissing; break;
            case CollectibleType::Horseshoe: ++horseMissing; break;
            case CollectibleType::Oyster: ++oysterMissing; break;
            case CollectibleType::StuntJump: ++jumpMissing; break;
            default: break;
        }
    }
    assert(tagMissing == 99);
    assert(snapshotMissing == 1);
    assert(horseMissing == 1);
    assert(oysterMissing == 1);
    assert(jumpMissing == 69);

    std::cout << "parser synthetic test: OK\n";
    return 0;
}
