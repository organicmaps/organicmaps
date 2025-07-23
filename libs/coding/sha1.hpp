#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace coding
{
class SHA1
{
public:
  static size_t constexpr kHashSizeInBytes = 20;
  using Hash = std::array<uint8_t, kHashSizeInBytes>;

  static Hash Calculate(std::string const & filePath);
  static std::string CalculateBase64(std::string const & filePath);

  static Hash CalculateForString(std::string_view str);
};
}  // namespace coding
