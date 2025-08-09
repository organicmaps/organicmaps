#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace kml
{

template <typename Channel>
constexpr uint32_t ToRGBA(Channel red, Channel green, Channel blue, Channel alpha = Channel(255))
{
  return static_cast<uint8_t>(red) << 24 | static_cast<uint8_t>(green) << 16 | static_cast<uint8_t>(blue) << 8 |
         static_cast<uint8_t>(alpha);
}

std::optional<uint32_t> ParseHexColor(std::string_view c);
std::optional<uint32_t> ParseGarminColor(std::string_view c);
std::optional<uint32_t> ParseOSMColor(std::string_view c);

}  // namespace kml
