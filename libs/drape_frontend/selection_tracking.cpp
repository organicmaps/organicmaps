#include "drape_frontend/selection_tracking.hpp"

#include <algorithm>

namespace df
{
m2::PointD SelectionTrackInfo::CalcOffset(m2::RectD const & oldViewport, m2::RectD const & newViewport,
                                          m2::RectD const & rect, m2::PointD const & pos)
{
  if (m_snapSides.x == 0)
    m_startPos.x = pos.x;
  if (m_snapSides.y == 0)
    m_startPos.y = pos.y;

  if (newViewport.SizeX() < rect.SizeX() || newViewport.SizeY() < rect.SizeY())
    return m2::PointD::Zero();

  m2::PointD offset(0.0, 0.0);
  bool const fits = newViewport.IsRectInside(rect);
  if ((!fits && oldViewport.IsIntersect(rect)) || (fits && m_snapSides != m2::PointI::Zero()))
  {
    // In case the rect of the selection is [partly] hidden, scroll the map to keep it visible.
    // In case the rect of the selection is visible after the map scrolling,
    // try to rollback part of that scrolling to return the map to its original position.
    if (rect.minX() < newViewport.minX() || m_snapSides.x < 0)
    {
      offset.x = std::max(m_startPos.x - pos.x, newViewport.minX() - rect.minX());
      m_snapSides.x = -1;
    }
    else if (rect.maxX() > newViewport.maxX() || m_snapSides.x > 0)
    {
      offset.x = std::min(m_startPos.x - pos.x, newViewport.maxX() - rect.maxX());
      m_snapSides.x = 1;
    }

    if (rect.minY() < newViewport.minY() || m_snapSides.y < 0)
    {
      offset.y = std::max(m_startPos.y - pos.y, newViewport.minY() - rect.minY());
      m_snapSides.y = -1;
    }
    else if (rect.maxY() > newViewport.maxY() || m_snapSides.y > 0)
    {
      offset.y = std::min(m_startPos.y - pos.y, newViewport.maxY() - rect.maxY());
      m_snapSides.y = 1;
    }
  }
  return offset;
}
}  // namespace df
