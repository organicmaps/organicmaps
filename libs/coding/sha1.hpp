#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace coding
{
class SHA1
{
public:
  static size_t constexpr kHashSizeInBytes = 20;
  using Hash = std::array<uint8_t, kHashSizeInBytes>;

  static Hash Calculate(std::string const & filePath);
  static Hash Calculate(std::vector<uint8_t> const & bytes);
  static std::string CalculateBase64(std::string const & filePath);

  static Hash CalculateForString(std::string_view str);
};
}  // namespace coding
