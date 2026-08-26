#include "Md5.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace gtasa {
namespace {

constexpr std::array<std::uint32_t, 64> kShift = {
    7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
    5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
    4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
    6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
};

constexpr std::array<std::uint32_t, 64> kTable = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};

inline std::uint32_t rotl(std::uint32_t x, std::uint32_t n) {
    return (x << n) | (x >> (32U - n));
}

std::uint32_t loadLe32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

void storeLe32(std::uint8_t* p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8);
    p[2] = static_cast<std::uint8_t>(v >> 16);
    p[3] = static_cast<std::uint8_t>(v >> 24);
}

} // namespace

Md5::Md5() : state_{0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U} {}

void Md5::transform(const std::uint8_t block[64]) {
    std::array<std::uint32_t, 16> m{};
    for (std::size_t i = 0; i < m.size(); ++i) m[i] = loadLe32(block + i * 4);

    std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    for (std::uint32_t i = 0; i < 64; ++i) {
        std::uint32_t f = 0, g = 0;
        if (i < 16) {
            f = (b & c) | ((~b) & d); g = i;
        } else if (i < 32) {
            f = (d & b) | ((~d) & c); g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = b ^ c ^ d; g = (3 * i + 5) % 16;
        } else {
            f = c ^ (b | (~d)); g = (7 * i) % 16;
        }
        const std::uint32_t nextD = c;
        c = b;
        b = b + rotl(a + f + kTable[i] + m[g], kShift[i]);
        a = d;
        d = nextD;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
}

void Md5::update(const void* data, std::size_t size) {
    if (finalized_ || size == 0) return;
    const auto* p = static_cast<const std::uint8_t*>(data);
    totalBytes_ += size;

    if (bufferSize_ != 0) {
        const std::size_t take = std::min(size, 64 - bufferSize_);
        std::memcpy(buffer_.data() + bufferSize_, p, take);
        bufferSize_ += take;
        p += take;
        size -= take;
        if (bufferSize_ == 64) {
            transform(buffer_.data());
            bufferSize_ = 0;
        }
    }
    while (size >= 64) {
        transform(p);
        p += 64;
        size -= 64;
    }
    if (size != 0) {
        std::memcpy(buffer_.data(), p, size);
        bufferSize_ = size;
    }
}

std::array<std::uint8_t, 16> Md5::final() {
    if (finalized_) return digest_;
    const std::uint64_t bitLength = totalBytes_ * 8U;
    const std::uint8_t one = 0x80;
    update(&one, 1);
    const std::uint8_t zero = 0;
    while (bufferSize_ != 56) update(&zero, 1);
    std::uint8_t length[8]{};
    for (int i = 0; i < 8; ++i) length[i] = static_cast<std::uint8_t>(bitLength >> (8 * i));
    update(length, sizeof(length));

    for (std::size_t i = 0; i < state_.size(); ++i) storeLe32(digest_.data() + i * 4, state_[i]);
    finalized_ = true;
    return digest_;
}

std::string hexDigest(const std::array<std::uint8_t, 16>& digest) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto b : digest) ss << std::setw(2) << static_cast<unsigned>(b);
    return ss.str();
}

} // namespace gtasa
