#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace coding
{
class MoveToFront
{
public:
  static size_t constexpr kNumBytes = static_cast<size_t>(std::numeric_limits<uint8_t>::max()) + 1;

  MoveToFront();

  // Returns index of the byte |b| in the current sequence of bytes,
  // then moves |b| to the first position.
  uint8_t Transform(uint8_t b);

  uint8_t operator[](uint8_t i) const { return m_order[i]; }

private:
  std::array<uint8_t, kNumBytes> m_order;
};
}  // namespace coding
