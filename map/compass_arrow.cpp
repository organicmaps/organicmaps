#include "compass_arrow.hpp"

#include "framework.hpp"

#include "../anim/controller.hpp"

#include "../gui/controller.hpp"

#include "../geometry/any_rect2d.hpp"
#include "../geometry/transformations.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/skin.hpp"

CompassArrow::CompassArrow(Params const & p)
  : base_t(p),
    m_arrowWidth(p.m_arrowWidth),
    m_arrowHeight(p.m_arrowHeight),
    m_northLeftColor(0xcc, 0x33, 0x00, 0xcc),
    m_northRightColor(0xff, 0x33, 0x00, 0xcc),
    m_southLeftColor(0xcc, 0xcc, 0xcc, 0xcc),
    m_southRightColor(0xff, 0xff, 0xff, 0xcc),
    m_angle(0),
    m_boundRects(1),
    m_framework(p.m_framework)
{
}

void CompassArrow::SetAngle(double angle)
{
  m_angle = angle;
  setIsDirtyRect(true);
}

vector<m2::AnyRectD> const & CompassArrow::boundRects() const
{
  if (isDirtyRect())
  {
    double k = visualScale();

    double halfW = m_arrowWidth / 2.0 * k;
    double halfH = m_arrowHeight / 2.0 * k;

    m_boundRects[0] = m2::AnyRectD(pivot(),
                                   -math::pi / 2 + m_angle,
                                   m2::RectD(-halfW, -halfH, halfW, halfH));

    setIsDirtyRect(false);
  }

  return m_boundRects;
}

void CompassArrow::draw(graphics::gl::OverlayRenderer * r,
                        math::Matrix<double, 3, 3> const & m) const
{
  if (isVisible())
  {
    checkDirtyDrawing();

    math::Matrix<double, 3, 3> drawM = math::Shift(
                                         math::Rotate(
                                           math::Identity<double, 3>(),
                                           m_angle),
                                         pivot());

    r->drawDisplayList(m_displayList.get(), drawM * m);
  }
}

void CompassArrow::cache()
{
  graphics::gl::Screen * cacheScreen = m_controller->GetCacheScreen();

  m_displayList.reset();
  m_displayList.reset(cacheScreen->createDisplayList());

  cacheScreen->beginFrame();
  cacheScreen->setDisplayList(m_displayList.get());

  double const k = visualScale();

  double const halfW = m_arrowWidth / 2.0 * k;
  double const halfH = m_arrowHeight / 2.0 * k;

  m2::PointF const northLeftPts[3] = {
    m2::PointF(-halfW, 0),
    m2::PointF(0, -halfH),
    m2::PointF(0, 0)
  };
  cacheScreen->drawConvexPolygon(northLeftPts, 3, m_northLeftColor, depth());

  m2::PointF const northRightPts[3] = {
    m2::PointF(0, 0),
    m2::PointF(0, -halfH),
    m2::PointF(halfW, 0)
  };
  cacheScreen->drawConvexPolygon(northRightPts, 3, m_northRightColor, depth());

  m2::PointF const southLeftPts[3] = {
    m2::PointF(-halfW, 0),
    m2::PointF(0, 0),
    m2::PointF(0, halfH),
  };
  cacheScreen->drawConvexPolygon(southLeftPts, 3, m_southLeftColor, depth());

  m2::PointF const southRightPts[3] = {
    m2::PointF(0, 0),
    m2::PointF(halfW, 0),
    m2::PointF(0, halfH),
  };
  cacheScreen->drawConvexPolygon(southRightPts, 3, m_southRightColor, depth());

  m2::PointD outlinePts[6] = {
    m2::PointD(-halfW, 0),
    m2::PointD(0, -halfH),
    m2::PointD(halfW, 0),
    m2::PointD(0, halfH),
    m2::PointD(-halfW, 0),
    m2::PointD(halfW, 0)
  };

  graphics::PenInfo const outlinePenInfo(graphics::Color(0x66, 0x66, 0x66, 0xcc), 1, 0, 0, 0);

  cacheScreen->drawPath(outlinePts, sizeof(outlinePts) / sizeof(m2::PointD), 0, cacheScreen->skin()->mapPenInfo(outlinePenInfo), depth());

  cacheScreen->setDisplayList(0);
  cacheScreen->endFrame();

  // we should not call cacheScreen->completeCommands
  // as the gui::Element::cache is called on the GUI thread.
}

void CompassArrow::purge()
{
  m_displayList.reset();
}

bool CompassArrow::onTapEnded(m2::PointD const & pt)
{
  anim::Controller * animController = m_framework->GetAnimController();
  animController->Lock();

  /// switching off compass follow mode
  m_framework->GetInformationDisplay().locationState()->StopCompassFollowing();

  double startAngle = m_framework->GetNavigator().Screen().GetAngle();
  double endAngle = 0;

  m_framework->GetAnimator().RotateScreen(startAngle, endAngle);

  animController->Unlock();

  m_framework->Invalidate();

  return true;
}

unsigned CompassArrow::GetArrowHeight() const
{
  return m_arrowHeight;
}

bool CompassArrow::hitTest(m2::PointD const & pt) const
{
  double rad = max(m_arrowWidth / 2, m_arrowHeight / 2);
  return pt.Length(pivot()) < rad * visualScale();
}
