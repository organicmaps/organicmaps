#include "track.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/pen.hpp"
#include "../graphics/depth_constants.hpp"
#include "drawer.hpp"
#include "../base/timer.hpp"

Track::~Track()
{
  DeleteDisplayList();
}

void Track::DeleteDisplayList()
{
  if (HasDisplayList())
  {
    delete m_dList;
    m_dList = 0;
    LOG(LDEBUG, ("DisplayList deleted for track", m_name));
  }
}

void Track::Draw(shared_ptr<PaintEvent> const & e)
{
  if (HasDisplayList())
  {
    graphics::Screen * screen = e->drawer()->screen();
    screen->drawDisplayList(m_dList, math::Identity<double, 3>());
    LOG(LDEBUG, ("Drawing track:", GetName()));
  }
}

void Track::UpdateDisplayList(Navigator & navigator, graphics::Screen * dListScreen)
{
  const bool isIntersect = navigator.Screen().GlobalRect().IsIntersect(m2::AnyRectD(GetLimitRect()));
  if ( !(IsVisible() && isIntersect) )
  {
    LOG(LDEBUG, ("No intresection, deleting dlist", GetName()));
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
    LOG(LDEBUG, ("Number of points in rect = ", countInRect));
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

bool Track::IsViewportChanged(ScreenBase const & sb)
{
  // check if viewport changed
  return m_screenBase != sb;
}

m2::RectD Track::GetLimitRect() const { return m_polyline.GetLimitRect(); }
size_t Track::Size() const { return m_polyline.m_points.size(); }
