#pragma once

#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

namespace df
{
// Scrolls the map to keep the selection visible while a panel covers a part of the screen,
// and scrolls it back when the panel goes away.
struct SelectionTrackInfo
{
  // Returns the pixel offset for the selection at pos, with rect around it, when the visible viewport changes.
  m2::PointD CalcOffset(m2::RectD const & oldViewport, m2::RectD const & newViewport, m2::RectD const & rect,
                        m2::PointD const & pos);

  m2::AnyRectD m_startRect;
  // Position to roll back to on a scrolled axis. It is taken at the first correction, not at selection,
  // because the selection can be made before a queued camera move, e.g. a track opened from a list.
  m2::PointD m_startPos;
  m2::PointI m_snapSides = m2::PointI::Zero();
};
}  // namespace df
