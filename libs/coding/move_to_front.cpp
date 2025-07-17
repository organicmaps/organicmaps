#include "coding/move_to_front.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>

namespace coding
{
MoveToFront::MoveToFront()
{
  std::iota(m_order.begin(), m_order.end(), 0);
}

uint8_t MoveToFront::Transform(uint8_t b)
{
  auto const it = std::find(m_order.begin(), m_order.end(), b);
  ASSERT(it != m_order.end(), ());

  size_t const result = std::distance(m_order.begin(), it);
  ASSERT_LESS(result, kNumBytes, ());

  std::rotate(m_order.begin(), it, it + 1);
  ASSERT_EQUAL(m_order[0], b, ());
  return static_cast<uint8_t>(result);
}
}  // namespace coding
