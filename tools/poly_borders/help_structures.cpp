#include "poly_borders/help_structures.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace poly_borders
{
// Link --------------------------------------------------------------------------------------------
bool Link::operator<(Link const & rhs) const
{
  return std::tie(m_borderId, m_pointId) < std::tie(rhs.m_borderId, rhs.m_pointId);
}

// ReplaceData -------------------------------------------------------------------------------------
bool ReplaceData::operator<(ReplaceData const & rhs) const
{
  return std::tie(m_dstFrom, m_dstTo) < std::tie(rhs.m_dstFrom, rhs.m_dstTo);
}

// MarkedPoint -------------------------------------------------------------------------------------
void MarkedPoint::AddLink(size_t borderId, size_t pointId)
{
  std::lock_guard<std::mutex> lock(*m_mutex);
  m_links.emplace(borderId, pointId);
}

std::optional<Link> MarkedPoint::GetLink(size_t curBorderId) const
{
  if (m_links.size() != 1)
    return std::nullopt;

  size_t anotherBorderId = m_links.begin()->m_borderId;
  if (anotherBorderId == curBorderId)
    return std::nullopt;

  return *m_links.begin();
}

// Polygon -----------------------------------------------------------------------------------------
void Polygon::MakeFrozen(size_t a, size_t b)
{
  CHECK_LESS(a, b, ());

  // Ends of intervals shouldn't be frozen, we freeze only inner points: (a, b).
  // This condition is needed to drop such cases: a = x, b = x + 1, when
  // a + 1 will be greater than b - 1.
  if (b - a + 1 > 2)
    m_replaced.AddInterval(a + 1, b - 1);
}

bool Polygon::IsFrozen(size_t a, size_t b) const
{
  // We use LESS_OR_EQUAL because we want sometimes to check if
  // point i (associated with interval: [i, i]) is frozen.
  CHECK_LESS_OR_EQUAL(a, b, ());

  return m_replaced.Intersects(a, b);
}

void Polygon::AddReplaceInfo(size_t dstFrom, size_t dstTo,
                             size_t srcFrom, size_t srcTo, size_t srcBorderId,
                             bool reversed)
{
  CHECK_LESS_OR_EQUAL(dstFrom, dstTo, ());
  CHECK_LESS(srcFrom, srcTo, ());

  CHECK(!IsFrozen(dstFrom, dstTo), ());
  MakeFrozen(dstFrom, dstTo);

  m_replaceData.emplace(dstFrom, dstTo, srcFrom, srcTo, srcBorderId, reversed);
}

std::set<ReplaceData>::const_iterator Polygon::FindReplaceData(size_t index)
{
  for (auto it = m_replaceData.cbegin(); it != m_replaceData.cend(); ++it)
  {
    if (it->m_dstFrom <= index && index <= it->m_dstTo)
      return it;
  }

  return m_replaceData.cend();
}
}  // namespace poly_borders
