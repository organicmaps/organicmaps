#include "compass_arrow.hpp"
#include "framework.hpp"
#include "alfa_animation_task.hpp"

#include "../anim/controller.hpp"

#include "../gui/controller.hpp"

#include "../geometry/transformations.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/screen.hpp"


using namespace graphics;

CompassArrow::Params::Params()
  : m_framework(0)
{}

CompassArrow::CompassArrow(Params const & p)
  : BaseT(p),
    m_pixelSize(-1, -1),
    m_angle(0),
    m_framework(p.m_framework)
{
}

void CompassArrow::AnimateShow()
{
  if (m_animTask == NULL || IsHidingAnim())
  {
    if (!isBaseVisible())
    {
      setIsVisible(true);
      CreateAnim(0.1, 1.0, 0.2, 0.0, true);
    }
    else
      CreateAnim(GetCurrentAlfa(), 1.0, 0.2, 0.0, true);
  }
}

void CompassArrow::AnimateHide()
{
  if (isBaseVisible() && (m_animTask == NULL || !IsHidingAnim()))
    CreateAnim(1.0, 0.0, 0.3, 0.0, false);
}

void CompassArrow::SetAngle(double angle)
{
  m_angle = angle;
}

m2::PointI CompassArrow::GetPixelSize() const
{
  if (m_pixelSize == m2::PointI(-1, -1))
  {
    Resource const * res = GetCompassResource();
    m_pixelSize = m2::PointI(res->m_texRect.SizeX(), res->m_texRect.SizeY());
  }
  return m_pixelSize;
}

void CompassArrow::GetMiniBoundRects(RectsT & rects) const
{
  double const halfW = m_pixelSize.x / 2.0;
  double const halfH = m_pixelSize.y / 2.0;

  rects.push_back(m2::AnyRectD(pivot(), -math::pi / 2 + m_angle,
                               m2::RectD(-halfW, -halfH, halfW, halfH)));
}

void CompassArrow::draw(OverlayRenderer * r,
                        math::Matrix<double, 3, 3> const & m) const
{
  if (isBaseVisible())
  {
    checkDirtyLayout();

    UniformsHolder holder;
    holder.insertValue(ETransparency, GetCurrentAlfa());

    math::Matrix<double, 3, 3> drawM = math::Shift(
                                         math::Rotate(
                                           math::Identity<double, 3>(),
                                           m_angle),
                                         pivot());

    r->drawDisplayList(m_dl.get(), drawM * m, &holder);
  }
}

bool CompassArrow::isVisible() const
{
  if (m_animTask != NULL && IsHidingAnim())
    return false;

  return isBaseVisible();
}

void CompassArrow::AlfaAnimEnded(bool isVisible)
{
  setIsVisible(isVisible);
  m_animTask.reset();
}

bool CompassArrow::IsHidingAnim() const
{
  ASSERT(m_animTask != NULL, ());
  AlfaAnimationTask * a = static_cast<AlfaAnimationTask *>(m_animTask.get());
  return a->IsHiding();
}

float CompassArrow::GetCurrentAlfa() const
{
  if (m_animTask)
  {
    AlfaAnimationTask * a = static_cast<AlfaAnimationTask *>(m_animTask.get());
    return a->GetCurrentAlfa();
  }

  return 1.0;
}

void CompassArrow::CreateAnim(double startAlfa, double endAlfa, double timeInterval, double timeOffset, bool isVisibleAtEnd)
{
  if (m_framework->GetAnimController() == NULL)
    return;

  if (m_animTask)
    m_animTask->Cancel();
  m_animTask.reset(new AlfaAnimationTask(startAlfa, endAlfa, timeInterval, timeOffset, m_framework));
  m_animTask->AddCallback(anim::Task::EEnded, bind(&CompassArrow::AlfaAnimEnded, this, isVisibleAtEnd));
  m_framework->GetAnimController()->AddTask(m_animTask);
}

const Resource * CompassArrow::GetCompassResource() const
{
  Screen * cacheScreen = m_controller->GetCacheScreen();
  Icon::Info icon("compass-image");
  Resource const * res = m_controller->GetCacheScreen()->fromID(cacheScreen->findInfo(icon));
  ASSERT(res, ("Commpass-image not founded"));
  return res;
}

void CompassArrow::cache()
{
  Screen * cacheScreen = m_controller->GetCacheScreen();

  m_dl.reset();
  m_dl.reset(cacheScreen->createDisplayList());

  cacheScreen->beginFrame();
  cacheScreen->setDisplayList(m_dl.get());
  cacheScreen->applyVarAlfaStates();

  Resource const * res = GetCompassResource();
  shared_ptr<gl::BaseTexture> texture = cacheScreen->pipeline(res->m_pipelineID).texture();
  m2::RectU rect = res->m_texRect;
  double halfW = rect.SizeX() / 2.0;
  double halfH = rect.SizeY() / 2.0;

  m2::PointD coords[] =
  {
    m2::PointD(-halfW, -halfH),
    m2::PointD(-halfW, halfH),
    m2::PointD(halfW, -halfH),
    m2::PointD(halfW, halfH),
  };

  m2::PointF normal(0.0, 0.0);
  m2::PointF texCoords[] =
  {
    texture->mapPixel(m2::PointF(rect.minX(), rect.minY())),
    texture->mapPixel(m2::PointF(rect.minX(), rect.maxY())),
    texture->mapPixel(m2::PointF(rect.maxX(), rect.minY())),
    texture->mapPixel(m2::PointF(rect.maxX(), rect.maxY())),
  };

  cacheScreen->addTexturedStripStrided(coords, sizeof(m2::PointD),
                                       &normal, 0,
                                       texCoords, sizeof(m2::PointF),
                                       4, depth(), res->m_pipelineID);


  cacheScreen->setDisplayList(0);
  cacheScreen->endFrame();
}

void CompassArrow::purge()
{
  m_dl.reset();
}

bool CompassArrow::isBaseVisible() const
{
  return BaseT::isVisible();
}

bool CompassArrow::onTapEnded(m2::PointD const & pt)
{
  anim::Controller * animController = m_framework->GetAnimController();
  anim::Controller::Guard guard(animController);

  /// switching off compass follow mode
  m_framework->GetInformationDisplay().locationState()->StopCompassFollowing();

  double startAngle = m_framework->GetNavigator().Screen().GetAngle();
  double endAngle = 0;

  m_framework->GetAnimator().RotateScreen(startAngle, endAngle);

  m_framework->Invalidate();

  return true;
}

bool CompassArrow::roughHitTest(m2::PointD const & pt) const
{
  return hitTest(pt);
}

bool CompassArrow::hitTest(m2::PointD const & pt) const
{
  Resource const * res = GetCompassResource();
  double rad = 1.5 * max(res->m_texRect.SizeX() / 2.0, res->m_texRect.SizeY() / 2.0);
  return pt.Length(pivot()) < rad * visualScale();
}
