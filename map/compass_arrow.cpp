#include "compass_arrow.hpp"
#include "framework.hpp"
#include "rotate_screen_task.hpp"

#include "../anim/controller.hpp"

#include "../gui/controller.hpp"

#include "../geometry/any_rect2d.hpp"
#include "../geometry/transformations.hpp"

#include "../yg/display_list.hpp"
#include "../yg/screen.hpp"
#include "../yg/skin.hpp"

CompassArrow::CompassArrow(Params const & p)
  : base_t(p),
    m_boundRects(1)
{
  m_arrowWidth = p.m_arrowWidth;
  m_arrowHeight = p.m_arrowHeight;
  m_northColor = p.m_northColor;
  m_southColor = p.m_southColor;
  m_framework = p.m_framework;
  m_angle = 0;
}

void CompassArrow::setAngle(double angle)
{
  m_angle = angle;
  m_drawM = math::Shift(math::Rotate(math::Identity<double, 3>(), m_angle), pivot() * visualScale());

  setIsDirtyRect(true);
}

vector<m2::AnyRectD> const & CompassArrow::boundRects() const
{
  if (isDirtyRect())
  {
    double k = visualScale();

    m2::PointD pv = pivot() * k;
    double halfW = m_arrowWidth / 2.0 * k;
    double halfH = m_arrowHeight / 2.0 * k;

    m_boundRects[0] = m2::AnyRectD(pv,
                                   -math::pi / 2 + m_angle,
                                   m2::RectD(-halfW, -halfH, halfW, halfH));

    setIsDirtyRect(false);
  }

  return m_boundRects;
}

void CompassArrow::draw(yg::gl::OverlayRenderer * r,
                        math::Matrix<double, 3, 3> const & m) const
{
  if (isVisible())
  {
    checkDirtyDrawing();
    m_displayList->draw(m_drawM * m);
  }
}

void CompassArrow::cache()
{
  yg::gl::Screen * cacheScreen = m_controller->GetCacheScreen();

  m_displayList.reset();
  m_displayList.reset(cacheScreen->createDisplayList());

  cacheScreen->beginFrame();
  cacheScreen->setDisplayList(m_displayList.get());

  double k = visualScale();

  double halfW = m_arrowWidth / 2.0 * k;
  double halfH = m_arrowHeight / 2.0 * k;

  m2::PointF northPts[3] = {
    m2::PointF(-halfW, 0),
    m2::PointF(0, -halfH),
    m2::PointF(halfW, 0)
  };

  cacheScreen->drawConvexPolygon(northPts, 3, m_northColor, depth());

  m2::PointF southPts[3] = {
    m2::PointF(-halfW, 0),
    m2::PointF(halfW, 0),
    m2::PointF(0, halfH),
  };

  cacheScreen->drawConvexPolygon(southPts, 3, m_southColor, depth());

  m2::PointD outlinePts[6] = {
    m2::PointD(-halfW, 0),
    m2::PointD(0, -halfH),
    m2::PointD(halfW, 0),
    m2::PointD(0, halfH),
    m2::PointD(-halfW, 0),
    m2::PointD(halfW, 0)
  };

  yg::PenInfo outlinePenInfo(yg::Color(32, 32, 32, 255), 1, 0, 0, 0);

  cacheScreen->drawPath(outlinePts, sizeof(outlinePts) / sizeof(m2::PointD), 0, cacheScreen->skin()->mapPenInfo(outlinePenInfo), depth());

  m_drawM = math::Shift(math::Rotate(math::Identity<double, 3>(), m_angle), pivot() * k);

  cacheScreen->setDisplayList(0);
  cacheScreen->endFrame();

  // we should not call cacheScreen->completeCommands
  // as the gui::Element::cache is called on the GUI thread.
}

void CompassArrow::purge()
{
  m_displayList.reset();
}

void CompassArrow::StopAnimation()
{
  if (m_rotateScreenTask
  && !m_rotateScreenTask->IsEnded()
  && !m_rotateScreenTask->IsCancelled())
    m_rotateScreenTask->Cancel();
  m_rotateScreenTask.reset();
}

bool CompassArrow::onTapEnded(m2::PointD const & pt)
{
  shared_ptr<anim::Controller> animController = m_framework->GetRenderPolicy()->GetAnimController();
  animController->Lock();

  /// switching off compass follow mode
  m_framework->GetInformationDisplay().locationState()->StopCompassFollowing();

  if (m_rotateScreenTask
  && !m_rotateScreenTask->IsEnded()
  && !m_rotateScreenTask->IsCancelled())
    m_rotateScreenTask->Cancel();
  m_rotateScreenTask.reset();

  double startAngle = m_framework->GetNavigator().Screen().GetAngle();
  double endAngle = 0;

  double period = 2 * math::pi;

  startAngle -= floor(startAngle / period) * period;
  endAngle -= floor(endAngle / period) * period;

  if (fabs(startAngle - endAngle) > math::pi)
    startAngle -= 2 * math::pi;

  m_rotateScreenTask.reset(new RotateScreenTask(m_framework,
                                                startAngle,
                                                endAngle,
                                                2));
  animController->AddTask(m_rotateScreenTask);

  animController->Unlock();

  m_framework->Invalidate();

  return true;
}

bool CompassArrow::hitTest(m2::PointD const & pt) const
{
  double rad = max(m_arrowWidth / 2, m_arrowHeight / 2);
  return pt.Length(pivot() * visualScale()) < rad * visualScale();
}
