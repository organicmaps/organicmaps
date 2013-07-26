#include "track.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/depth_constants.hpp"
#include "drawer.hpp"
#include "../geometry/screenbase.hpp"

void Track::Draw(shared_ptr<PaintEvent> const & e)
{
  // do not draw degenerated points
  if (Size() < 2)
    return;

  graphics::Screen * screen = e->drawer()->screen();
  ScreenBase sb;
  // pen
  graphics::Pen::Info info(m_color, m_width);
  uint32_t resId = screen->mapInfo(info);

  // depth
  const double depth = graphics::debugDepth;

  // offset
  const size_t offset = 0;

  // points
  vector<m2::PointD> pixPoints(m_polyline.m_points.size());
  for (int i=0; i < pixPoints.size(); ++i)
    pixPoints[i] = sb.GtoP(m_polyline.m_points[i]);
  // add matrix?

  screen->drawPath(&pixPoints[0], pixPoints.size(), offset, resId, depth);
}

m2::RectD Track::GetLimitRect() { return m_polyline.GetLimitRect(); }
size_t Track::Size() { return m_polyline.m_points.size(); }
