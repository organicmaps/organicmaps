#include "map/compass_arrow.hpp"
#include "map/framework.hpp"
#include "map/alfa_animation_task.hpp"

#include "anim/controller.hpp"

#include "gui/controller.hpp"

#include "geometry/transformations.hpp"

#include "graphics/display_list.hpp"
#include "graphics/screen.hpp"


using namespace graphics;

CompassArrow::Params::Params()
  : m_framework(0)
{}

CompassArrow::CompassArrow(Params const & p)
  : m_angle(0),
    m_framework(p.m_framework)
{
}

void CompassArrow::AnimateShow()
{
  ///@TODO UVR
//  if (!isVisible())
//  {
//    setIsVisible(true);
//    double startValue = m_animTask == nullptr ? 0.1 : GetCurrentAlfa();
//    CreateAnim(startValue, 1.0, 0.2, 0.0, true);
//  }
}

void CompassArrow::AnimateHide()
{
  ///@TODO UVR
//  if (isBaseVisible() && (m_animTask == NULL || !IsHidingAnim()))
//    CreateAnim(1.0, 0.0, 0.3, 0.0, false);
}

void CompassArrow::SetAngle(double angle)
{
  m_angle = angle;
}

///@TODO UVR
//void CompassArrow::draw(OverlayRenderer * r,
//                        math::Matrix<double, 3, 3> const & m) const
//{
//  if (isBaseVisible())
//  {
//    checkDirtyLayout();

//    UniformsHolder holder;
//    holder.insertValue(ETransparency, GetCurrentAlfa());

//    math::Matrix<double, 3, 3> drawM = math::Shift(
//                                         math::Rotate(
//                                           math::Identity<double, 3>(),
//                                           m_angle),
//                                         pivot());

//    r->drawDisplayList(m_dl.get(), drawM * m, &holder);
//  }
//}

bool CompassArrow::isVisible() const
{
  if (m_animTask != NULL && IsHidingAnim())
    return false;

  return isBaseVisible();
}

void CompassArrow::AlfaAnimEnded(bool isVisible)
{
  ///@TODO UVR
  //setIsVisible(isVisible);
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

void CompassArrow::cache()
{
  ///@TODO UVR
//  Screen * cacheScreen = m_controller->GetCacheScreen();

//  m_dl.reset();
//  m_dl.reset(cacheScreen->createDisplayList());

//  cacheScreen->beginFrame();
//  cacheScreen->setDisplayList(m_dl.get());
//  cacheScreen->applyVarAlfaStates();

//  Resource const * res = GetCompassResource();
//  shared_ptr<gl::BaseTexture> texture = cacheScreen->pipeline(res->m_pipelineID).texture();
//  m2::RectU rect = res->m_texRect;
//  double halfW = rect.SizeX() / 2.0;
//  double halfH = rect.SizeY() / 2.0;

//  m2::PointD coords[] =
//  {
//    m2::PointD(-halfW, -halfH),
//    m2::PointD(-halfW, halfH),
//    m2::PointD(halfW, -halfH),
//    m2::PointD(halfW, halfH),
//  };

//  m2::PointF normal(0.0, 0.0);
//  m2::PointF texCoords[] =
//  {
//    texture->mapPixel(m2::PointF(rect.minX(), rect.minY())),
//    texture->mapPixel(m2::PointF(rect.minX(), rect.maxY())),
//    texture->mapPixel(m2::PointF(rect.maxX(), rect.minY())),
//    texture->mapPixel(m2::PointF(rect.maxX(), rect.maxY())),
//  };

//  cacheScreen->addTexturedStripStrided(coords, sizeof(m2::PointD),
//                                       &normal, 0,
//                                       texCoords, sizeof(m2::PointF),
//                                       4, depth(), res->m_pipelineID);


//  cacheScreen->setDisplayList(0);
//  cacheScreen->endFrame();
}

void CompassArrow::purge()
{
}

bool CompassArrow::isBaseVisible() const
{
  ///@TODO UVR
  return false;
}
