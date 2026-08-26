#pragma once
#include "Collectibles.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace gtasa { class SaveParser { public: ParseResult parseFile(const std::string& path) const; ParseResult parseBytes(const std::vector<std::uint8_t>& data, const std::string& name) const; private: std::size_t findPickups(const std::vector<std::uint8_t>& data) const; std::size_t findTags(const std::vector<std::uint8_t>& data) const; std::size_t findStuntJumps(const std::vector<std::uint8_t>& data) const; }; }
