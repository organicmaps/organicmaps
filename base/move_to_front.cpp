#include "base/move_to_front.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>

using namespace std;

namespace base
{
// static
size_t constexpr MoveToFront::kNumBytes;

MoveToFront::MoveToFront()
{
  iota(m_order.begin(), m_order.end(), 0);
}

uint8_t MoveToFront::Transform(uint8_t b)
{
  auto const it = find(m_order.begin(), m_order.end(), b);
  ASSERT(it != m_order.end(), ());

  size_t const result = distance(m_order.begin(), it);
  ASSERT_LESS(result, kNumBytes, ());

  rotate(m_order.begin(), it, it + 1);
  ASSERT_EQUAL(m_order[0], b, ());
  return static_cast<uint8_t>(result);
}
}  // namespace base
