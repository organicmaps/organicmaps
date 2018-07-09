#include "geometry/nearby_points_sweeper.hpp"

namespace m2
{
// NearbyPointsSweeper::Event ----------------------------------------------------------------------
NearbyPointsSweeper::Event::Event(Type type, double y, double x, size_t index, uint8_t priority)
  : m_type(type), m_y(y), m_x(x), m_index(index), m_priority(priority)
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

  if (m_index != rhs.m_index)
    return m_index < rhs.m_index;

  return m_priority < rhs.m_priority;
}

// NearbyPointsSweeper -----------------------------------------------------------------------------
NearbyPointsSweeper::NearbyPointsSweeper(double eps) : m_eps(eps), m_heps(std::max(eps * 0.5, 0.0))
{
}

void NearbyPointsSweeper::Add(double x, double y, size_t index, uint8_t priority)
{
  m_events.emplace_back(Event::TYPE_SEGMENT_START, y - m_heps, x, index, priority);
  m_events.emplace_back(Event::TYPE_SEGMENT_END, y + m_heps, x, index, priority);
}
}  // namespace m2
