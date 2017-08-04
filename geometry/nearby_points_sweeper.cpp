#include "geometry/nearby_points_sweeper.hpp"

namespace m2
{
// NearbyPointsSweeper::Event ----------------------------------------------------------------------
NearbyPointsSweeper::Event::Event(Type type, double y, double x, size_t index)
  : m_type(type), m_y(y), m_x(x), m_index(index)
{
}

bool NearbyPointsSweeper::Event::operator<(Event const & rhs) const
{
  if (m_y != rhs.m_y)
    return m_y < rhs.m_y;

  if (m_type != rhs.m_type)
    return m_type < rhs.m_type;

  if (m_x != rhs.m_x)
    return m_x < rhs.m_x;

  return m_index < rhs.m_index;
}

// NearbyPointsSweeper -----------------------------------------------------------------------------
NearbyPointsSweeper::NearbyPointsSweeper(double eps) : m_eps(eps), m_heps(std::max(eps * 0.5, 0.0))
{
}

void NearbyPointsSweeper::Add(double x, double y, size_t index)
{
  m_events.emplace_back(Event::TYPE_SEGMENT_START, y - m_heps, x, index);
  m_events.emplace_back(Event::TYPE_SEGMENT_END, y + m_heps, x, index);
}
}  // namespace m2
