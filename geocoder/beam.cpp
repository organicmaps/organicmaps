#include "geocoder/beam.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace geocoder
{
Beam::Beam(size_t capacity) : m_capacity(capacity) { m_entries.reserve(m_capacity); }

void Beam::Add(Key const & key, Value value)
{
  if (m_capacity == 0)
    return;

  Entry const e(key, value);
  auto it = std::lower_bound(m_entries.begin(), m_entries.end(), e);

  if (it == m_entries.end())
  {
    if (m_entries.size() < m_capacity)
      m_entries.emplace_back(e);
    return;
  }

  if (m_entries.size() == m_capacity)
    m_entries.pop_back();

  m_entries.insert(it, e);
}
}  // namespace geocoder
