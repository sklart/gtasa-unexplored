#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace gtasa {

class Md5 {
public:
    Md5();
    void update(const void* data, std::size_t size);
    std::array<std::uint8_t, 16> final();

private:
    void transform(const std::uint8_t block[64]);

    std::array<std::uint32_t, 4> state_{};
    std::array<std::uint8_t, 64> buffer_{};
    std::uint64_t totalBytes_ = 0;
    std::size_t bufferSize_ = 0;
    bool finalized_ = false;
    std::array<std::uint8_t, 16> digest_{};
};

std::string hexDigest(const std::array<std::uint8_t, 16>& digest);

} // namespace gtasa
