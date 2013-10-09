#include "track.hpp"
#include "drawer.hpp"
#include "events.hpp"
#include "navigator.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/depth_constants.hpp"
#include "../graphics/display_list.hpp"

#include "../geometry/distance.hpp"

#include "../base/timer.hpp"
#include "../base/logging.hpp"


Track::~Track()
{
  DeleteDisplayList();
}

Track * Track::CreatePersistent()
{
  Track * p = new Track();
  p->Swap(*this);
  return p;
}

void Track::DeleteDisplayList()
{
  if (HasDisplayList())
  {
    delete m_dList;
    m_dList = 0;
  }
}

void Track::Draw(shared_ptr<PaintEvent> const & e)
{
  if (HasDisplayList())
  {
    graphics::Screen * screen = e->drawer()->screen();
    screen->drawDisplayList(m_dList, math::Identity<double, 3>());
  }
}

void Track::UpdateDisplayList(Navigator & navigator, graphics::Screen * dListScreen)
{
  const bool isIntersect = navigator.Screen().GlobalRect().IsIntersect(m2::AnyRectD(GetLimitRect()));
  if (!(IsVisible() && isIntersect))
  {
    DeleteDisplayList();
    return;
  }

  if (!HasDisplayList() || IsViewportChanged(navigator.Screen()))
  {
    m_screenBase = navigator.Screen();
    m2::AnyRectD const & screenRect = m_screenBase.GlobalRect();

    DeleteDisplayList();
    m_dList = dListScreen->createDisplayList();

    dListScreen->beginFrame();
    dListScreen->setDisplayList(m_dList);

    // Clip and transform points
    /// @todo can we optimize memory allocation?
    vector<m2::PointD> pixPoints(m_polyline.m_points.size());
    int countInRect = 0;
    bool lastSuccessed = false;
    for (int i = 1; i < m_polyline.m_points.size(); ++i)
    {
      m2::PointD & left = m_polyline.m_points[i-1];
      m2::PointD & right = m_polyline.m_points[i];
      m2::RectD segRect(left, right);

      if (m2::AnyRectD(segRect).IsIntersect(screenRect))
      {
        if (!lastSuccessed)
          pixPoints[countInRect++] = navigator.GtoP(m_polyline.m_points[i-1]);
        /// @todo add only visible segments drawing to avoid phanot segments
        pixPoints[countInRect++] = navigator.GtoP(m_polyline.m_points[i]);
        lastSuccessed = true;
      }
      else
        lastSuccessed = false;
    }

    // Draw
    if (countInRect >= 2)
    {
      graphics::Pen::Info info(m_color, m_width);
      uint32_t resId = dListScreen->mapInfo(info);
      /// @todo add simplification
      dListScreen->drawPath(&pixPoints[0], countInRect, 0, resId, graphics::tracksDepth);
    }
    dListScreen->setDisplayList(0);
    dListScreen->endFrame();
  }
}

bool Track::IsViewportChanged(ScreenBase const & sb) const
{
  // check if viewport changed
  return m_screenBase != sb;
}

m2::RectD Track::GetLimitRect() const
{
  return m_polyline.GetLimitRect();
}

size_t Track::Size() const
{
  return m_polyline.m_points.size();
}

double Track::GetShortestSquareDistance(m2::PointD const & point) const
{
  double res = numeric_limits<double>::max();
  m2::DistanceToLineSquare<m2::PointD> d;
  for (size_t i = 0; i + 1 < m_polyline.m_points.size(); ++i)
  {
    d.SetBounds(m_polyline.m_points[i], m_polyline.m_points[i + 1]);
    res = min(res, d(point));
  }
  return res;
}

void Track::Swap(Track & rhs)
{
  swap(m_isVisible, rhs.m_isVisible);
  swap(m_width, rhs.m_width);
  swap(m_color, rhs.m_color);

  m_name.swap(rhs.m_name);
  m_polyline.Swap(rhs.m_polyline);

  DeleteDisplayList();
  rhs.DeleteDisplayList();
}
