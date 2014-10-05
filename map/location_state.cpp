#include "location_state.hpp"
#include "navigator.hpp"
#include "framework.hpp"
#include "move_screen_task.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/icon.hpp"
#include "../graphics/depth_constants.hpp"

#include "../anim/controller.hpp"
#include "../anim/task.hpp"
#include "../anim/angle_interpolation.hpp"
#include "../anim/segment_interpolation.hpp"

#include "../gui/controller.hpp"

#include "../indexer/mercator.hpp"

#include "../platform/platform.hpp"
#include "../platform/location.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/transformations.hpp"

namespace location
{

namespace
{

static const int POSITION_Y_OFFSET = 120;
static const double POSITION_TOLERANCE = 1.0E-6;  // much less than coordinates coding error
static const double ANGLE_TOLERANCE = my::DegToRad(3.0);


uint16_t IncludeModeBit(uint16_t mode, uint16_t bit)
{
  return mode | bit;
}

uint16_t ExcludeModeBit(uint16_t mode, uint16_t bit)
{
  return mode & (~bit);
}

State::Mode ExcludeAllBits(uint16_t mode)
{
  return (State::Mode)(mode & 0xF);
}

uint16_t ChangeMode(uint16_t mode, State::Mode newMode)
{
  return (mode & 0xF0) | newMode;
}

bool TestModeBit(uint16_t mode, uint16_t bit)
{
  return (mode & bit) != 0;
}

class RotateAndFollowAnim : public anim::Task
{
public:
  RotateAndFollowAnim(Framework * fw, m2::PointD const & srcPos,
                      double srcAngle,
                      m2::PointD const & srcPixelBinding,
                      m2::PointD const & dstPixelbinding)
    : m_fw(fw)
  {
    m_angleAnim.reset(new anim::SafeAngleInterpolation(srcAngle, srcAngle, 1.0));
    m_posAnim.reset(new anim::SafeSegmentInterpolation(srcPos, srcPos, 1.0));
    m_pxBindingAnim.reset(new anim::SafeSegmentInterpolation(InvertPxBinding(srcPixelBinding),
                                                             InvertPxBinding(dstPixelbinding), 1.0));
  }

  void SetDestinationParams(m2::PointD const & dstPos, double dstAngle)
  {
    ASSERT(m_angleAnim != nullptr, ());
    ASSERT(m_posAnim != nullptr, ());

    if (dstPos.EqualDxDy(m_posAnim->GetCurrentValue(), POSITION_TOLERANCE) &&
        fabs(ang::GetShortestDistance(m_angleAnim->GetCurrentValue(), dstAngle)) < ANGLE_TOLERANCE)
      return;

    double const posSpeed = m_fw->GetNavigator().ComputeMoveSpeed(m_posAnim->GetCurrentValue(), dstPos);
    double const angleSpeed = 1.5;//m_fw->GetAnimator().GetRotationSpeed();

    m_angleAnim->ResetDestParams(dstAngle, angleSpeed);
    m_posAnim->ResetDestParams(dstPos, posSpeed);
  }

  virtual void OnStep(double ts)
  {
    ASSERT(m_angleAnim != nullptr, ());
    ASSERT(m_posAnim != nullptr, ());
    ASSERT(m_pxBindingAnim != nullptr, ());

    bool updateViewPort = false;
    updateViewPort |= OnStep(m_angleAnim.get(), ts);
    updateViewPort |= OnStep(m_posAnim.get(), ts);
    updateViewPort |= OnStep(m_pxBindingAnim.get(), ts);

    if (updateViewPort)
      UpdateViewport();
  }

  virtual bool IsVisual() const
  {
    ASSERT(m_posAnim != nullptr, ());
    ASSERT(m_angleAnim != nullptr, ());
    ASSERT(m_pxBindingAnim != nullptr, ());

    return m_posAnim->IsRunning() ||
           m_angleAnim->IsRunning() ||
           m_pxBindingAnim->IsRunning();
  }

private:
  void UpdateViewport()
  {
    ASSERT(m_posAnim != nullptr, ());
    ASSERT(m_angleAnim != nullptr, ());
    ASSERT(m_pxBindingAnim != nullptr, ());

    m2::PointD const & pxBinding = m_pxBindingAnim->GetCurrentValue();
    m2::PointD const & currentPosition = m_posAnim->GetCurrentValue();
    double currentAngle = m_angleAnim->GetCurrentValue();

    //@{ pixel coord system
    m2::PointD const pxCenter = GetPixelRect().Center();
    m2::PointD vectorToCenter = pxCenter - pxBinding;
    if (!vectorToCenter.IsAlmostZero())
      vectorToCenter = vectorToCenter.Normalize();
    m2::PointD const vectorToTop = m2::PointD(0.0, 1.0);
    double sign = m2::CrossProduct(vectorToTop, vectorToCenter) > 0 ? 1 : -1;
    double angle = sign * acos(m2::DotProduct(vectorToTop, vectorToCenter));
    //@}

    //@{ global coord system
    double offset = (m_fw->PtoG(pxCenter) - m_fw->PtoG(pxBinding)).Length();
    m2::PointD const viewPoint = currentPosition.Move(1.0, currentAngle + my::DegToRad(90.0));
    m2::PointD const viewVector = viewPoint - currentPosition;
    m2::PointD rotateVector = viewVector;
    rotateVector.Rotate(angle);
    rotateVector.Normalize();
    rotateVector *= offset;
    //@}

    m_fw->SetViewportCenter(currentPosition + rotateVector);
    m_fw->GetNavigator().SetAngle(currentAngle);
    m_fw->Invalidate();
  }

  bool OnStep(anim::Task * task, double ts)
  {
    if (task->IsReady())
    {
      task->Start();
      task->OnStart(ts);
    }

    if (task->IsRunning())
    {
      task->OnStep(ts);
      return true;
    }

    return false;
  }

private:
  m2::PointD InvertPxBinding(m2::PointD const & px) const
  {
    return m2::PointD(px.x, GetPixelRect().maxY() - px.y);
  }

  m2::RectD const & GetPixelRect() const
  {
    return m_fw->GetNavigator().Screen().PixelRect();
  }

private:
  Framework * m_fw;

  unique_ptr<anim::SafeAngleInterpolation> m_angleAnim;
  unique_ptr<anim::SafeSegmentInterpolation> m_posAnim;
  unique_ptr<anim::SafeSegmentInterpolation> m_pxBindingAnim;
};

}

State::Params::Params()
  : m_locationAreaColor(0, 0, 0, 0),
    m_framework(0)
{}

State::State(Params const & p)
  : TBase(p),
    m_modeInfo(Follow),
    m_errorRadius(0),
    m_position(0, 0),
    m_drawDirection(0.0),
    m_afterPendingMode(Follow),
    m_currentSlotID(0)
{
  m_locationAreaColor = p.m_locationAreaColor;
  m_framework = p.m_framework;

  setIsVisible(false);
}

m2::PointD const & State::Position() const
{
  return m_position;
}

double State::GetErrorRadius() const
{
  return m_errorRadius;
}

State::Mode State::GetMode() const
{
  return ExcludeAllBits(m_modeInfo);
}

bool State::IsModeChangeViewport() const
{
  return GetMode() >= Follow;
}

bool State::IsModeHasPosition() const
{
  return GetMode() >= NotFollow;
}

void State::SwitchToNextMode()
{
  Mode currentMode = GetMode();
  Mode newMode = currentMode;
  if (!IsInRouting())
  {
    switch (currentMode)
    {
    case UnknownPosition:
      newMode = PendingPosition;
      break;
    case PendingPosition:
      newMode = UnknownPosition;
      m_afterPendingMode = Follow;
      break;
    case NotFollow:
      newMode = Follow;
      break;
    case Follow:
      if (IsRotationActive())
        newMode = RotateAndFollow;
      else
      {
        newMode = UnknownPosition;
        m_afterPendingMode = Follow;
      }
      break;
    case RotateAndFollow:
      {
        newMode = UnknownPosition;
        m_afterPendingMode = Follow;
        break;
      }
    }
  }
  else
    newMode = IsRotationActive() ? RotateAndFollow : Follow;

  SetModeInfo(ChangeMode(m_modeInfo, newMode));
}

void State::RouteBuilded()
{
  ASSERT(GetPlatform().HasRouting(), ());
  StopAllAnimations();
  SetModeInfo(IncludeModeBit(m_modeInfo, RoutingSessionBit));
  if (GetMode() > NotFollow)
    SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
  else if (GetMode() == UnknownPosition)
  {
    m_afterPendingMode = NotFollow;
    SetModeInfo(ChangeMode(m_modeInfo, PendingPosition));
  }
}

void State::StartRouteFollow()
{
  ASSERT(TestModeBit(m_modeInfo, RoutingSessionBit), ());
  ASSERT(GetPlatform().HasRouting(), ());
  ASSERT(IsModeHasPosition(), ());
  m2::PointD const size(m_errorRadius, m_errorRadius);
  m_framework->ShowRectExVisibleScale(m2::RectD(m_position - size, m_position + size),
                                      scales::GetUpperComfortScale());

  SetModeInfo(ChangeMode(m_modeInfo, IsRotationActive() ? RotateAndFollow : Follow));
}

void State::StopRoutingMode()
{
  SetModeInfo(ExcludeModeBit(m_modeInfo, RoutingSessionBit));
}

void State::TurnOff()
{
  StopLocationFollow();
  SetModeInfo(UnknownPosition);
  setIsVisible(false);
  invalidate();
}

void State::OnLocationUpdate(location::GpsInfo const & info)
{
  m2::RectD rect = MercatorBounds::MetresToXY(info.m_longitude,
                                              info.m_latitude,
                                              info.m_horizontalAccuracy);

  m_position = rect.Center();
  m_errorRadius = rect.SizeX() / 2;

  setIsVisible(true);

  if (GetMode() == PendingPosition)
    SetModeInfo(ChangeMode(m_modeInfo, m_afterPendingMode));
  else
    AnimateFollow();

  CallPositionChangedListeners(m_position);
  invalidate();
}

void State::OnCompassUpdate(location::CompassInfo const & info)
{
  SetModeInfo(IncludeModeBit(m_modeInfo, KnownDirectionBit));

  m_drawDirection = info.m_bearing;

  AnimateFollow();
  invalidate();
}

void State::CallStateModeListeners()
{
  Mode const currentMode = GetMode();
  for (auto it : m_modeListeners)
    it.second(currentMode);
}

int State::AddStateModeListener(TStateModeListener const & l)
{
  int const slotID = m_currentSlotID++;
  m_modeListeners[slotID] = l;
  return slotID;
}

void State::RemoveStateModeListener(int slotID)
{
  m_modeListeners.erase(slotID);
}

void State::CallPositionChangedListeners(m2::PointD const & pt)
{
  for (auto it : m_positionListeners)
    it.second(pt);
}

int State::AddPositionChangedListener(State::TPositionListener const & func)
{
  int const slotID = m_currentSlotID++;
  m_positionListeners[slotID] = func;
  return slotID;
}

void State::RemovePositionChangedListener(int slotID)
{
  m_positionListeners.erase(slotID);
}

void State::InvalidatePosition()
{
  m_afterPendingMode = GetMode();
  if (m_afterPendingMode != UnknownPosition)
  {
    SetModeInfo(ChangeMode(m_modeInfo, UnknownPosition));
    SetModeInfo(ChangeMode(m_modeInfo, PendingPosition));
  }

  setIsVisible(false);
  invalidate();
}

void State::cache()
{
  CachePositionArrow();
  CacheRoutingArrow();
  CacheLocationMark();

  m_controller->GetCacheScreen()->completeCommands();
}

void State::purge()
{
  m_positionArrow.reset();
  m_locationMarkDL.reset();
  m_positionMarkDL.reset();
  m_routingArrow.reset();
}

void State::update()
{
  if (isVisible() && IsModeHasPosition())
  {
    m2::PointD const pxPosition = m_framework->GetNavigator().GtoP(Position());
    setPivot(pxPosition);
  }
}

void State::draw(graphics::OverlayRenderer * r,
                 math::Matrix<double, 3, 3> const & m) const
{
  if (!IsModeHasPosition() || !isVisible())
    return;

  checkDirtyLayout();

  m2::PointD const pxPosition = m_framework->GetNavigator().GtoP(Position());
  double const pxErrorRadius = pxPosition.Length(
        m_framework->GetNavigator().GtoP(Position() + m2::PointD(m_errorRadius, 0.0)));

  double const drawScale = pxErrorRadius / s_cacheRadius;

  math::Matrix<double, 3, 3> locationDrawM = math::Shift(
                                               math::Scale(
                                                 math::Identity<double, 3>(),
                                                 drawScale,
                                                 drawScale),
                                               pivot());

  math::Matrix<double, 3, 3> const drawM = locationDrawM * m;
  // draw error sector
  r->drawDisplayList(m_locationMarkDL.get(), drawM);

  // if we know look direction than we draw arrow
  if (IsDirectionKnown())
  {
    double rotateAngle = m_drawDirection + GetModelView().GetAngle();

    math::Matrix<double, 3, 3> compassDrawM = math::Shift(
                                                math::Rotate(
                                                  math::Identity<double, 3>(),
                                                  rotateAngle),
                                                pivot());

    if (!IsInRouting())
      r->drawDisplayList(m_positionArrow.get(), compassDrawM * m);
    else
      r->drawDisplayList(m_routingArrow.get(), compassDrawM * m);
  }
  else
    r->drawDisplayList(m_positionMarkDL.get(), drawM);
}

void State::CachePositionArrow()
{
  m_positionArrow.reset();
  m_positionArrow.reset(m_controller->GetCacheScreen()->createDisplayList());
  CacheArrow(m_positionArrow.get(), "current-position-compas");
}

void State::CacheRoutingArrow()
{
  m_routingArrow.reset();
  m_routingArrow.reset(m_controller->GetCacheScreen()->createDisplayList());
  CacheArrow(m_routingArrow.get(), "current-routing-compas");
}

void State::CacheLocationMark()
{
  graphics::Screen * cacheScreen = m_controller->GetCacheScreen();

  m_locationMarkDL.reset();
  m_locationMarkDL.reset(cacheScreen->createDisplayList());

  m_positionMarkDL.reset();
  m_positionMarkDL.reset(cacheScreen->createDisplayList());

  cacheScreen->beginFrame();
  cacheScreen->setDisplayList(m_locationMarkDL.get());

  cacheScreen->fillSector(m2::PointD(0, 0),
                          0, 2.0 * math::pi,
                          s_cacheRadius,
                          m_locationAreaColor,
                          graphics::locationFaultDepth);

  cacheScreen->setDisplayList(m_positionMarkDL.get());
  cacheScreen->drawSymbol(m2::PointD(0, 0),
                          "current-position",
                          graphics::EPosCenter,
                          graphics::locationDepth);

  cacheScreen->setDisplayList(0);

  cacheScreen->endFrame();
}

void State::CacheArrow(graphics::DisplayList * dl, const string & iconName)
{
  graphics::Screen * cacheScreen = m_controller->GetCacheScreen();
  graphics::Icon::Info info(iconName);

  graphics::Resource const * res = cacheScreen->fromID(cacheScreen->findInfo(info));
  m2::RectU const rect = res->m_texRect;
  m2::PointD const halfArrowSize(rect.SizeX() / 2.0, rect.SizeY() / 2.0);

  cacheScreen->beginFrame();
  cacheScreen->setDisplayList(dl);

  m2::PointD coords[4] =
  {
    m2::PointD(-halfArrowSize.x, -halfArrowSize.y),
    m2::PointD(-halfArrowSize.x,  halfArrowSize.y),
    m2::PointD( halfArrowSize.x, -halfArrowSize.y),
    m2::PointD( halfArrowSize.x,  halfArrowSize.y)
  };

  m2::PointF const normal(0.0, 0.0);
  shared_ptr<graphics::gl::BaseTexture> texture = cacheScreen->pipeline(res->m_pipelineID).texture();

  m2::PointF texCoords[4] =
  {
    texture->mapPixel(m2::PointF(rect.minX(), rect.minY())),
    texture->mapPixel(m2::PointF(rect.minX(), rect.maxY())),
    texture->mapPixel(m2::PointF(rect.maxX(), rect.minY())),
    texture->mapPixel(m2::PointF(rect.maxX(), rect.maxY()))
  };

  cacheScreen->addTexturedStripStrided(coords, sizeof(m2::PointD),
                                       &normal, 0,
                                       texCoords, sizeof(m2::PointF),
                                       4, graphics::locationDepth, res->m_pipelineID);
  cacheScreen->setDisplayList(0);
  cacheScreen->endFrame();
}

bool State::IsRotationActive() const
{
  return m_framework->GetNavigator().DoSupportRotation() && TestModeBit(m_modeInfo, KnownDirectionBit);
}

bool State::IsDirectionKnown() const
{
  return TestModeBit(m_modeInfo, KnownDirectionBit);
}

bool State::IsInRouting() const
{
  return TestModeBit(m_modeInfo, RoutingSessionBit);
}

m2::PointD const State::GetModeDefaultPixelBinding(State::Mode mode) const
{
  switch (mode)
  {
  case Follow: return m_framework->GetPixelCenter();
  case RotateAndFollow: return GetRaFModeDefaultPxBind();
  default: return m2::PointD(0.0, 0.0);
  }
}

bool State::FollowCompass()
{
  if (!IsRotationActive() || GetMode() != RotateAndFollow || m_animTask == nullptr)
    return false;

  RotateAndFollowAnim * task = static_cast<RotateAndFollowAnim *>(m_animTask.get());
  task->SetDestinationParams(Position(), -m_drawDirection);
  return true;
}

void State::CreateAnimTask()
{
  CreateAnimTask(m_framework->GtoP(Position()),
                 GetModeDefaultPixelBinding(GetMode()));
}

void State::CreateAnimTask(const m2::PointD & srcPx, const m2::PointD & dstPx)
{
  EndAnimation();
  m_animTask.reset(new RotateAndFollowAnim(m_framework, Position(),
                                           GetModelView().GetAngle(),
                                           srcPx, dstPx));
  m_framework->GetAnimController()->AddTask(m_animTask);
}

void State::EndAnimation()
{
  if (m_animTask != nullptr)
  {
    m_animTask->End();
    m_animTask.reset();
  }
}

void State::SetModeInfo(uint16_t modeInfo)
{
  Mode const newMode = ExcludeAllBits(modeInfo);
  Mode const oldMode = GetMode();
  m_modeInfo = modeInfo;
  if (newMode != oldMode)
  {
    CallStateModeListeners();
    AnimateStateTransition(oldMode, newMode);
    invalidate();
  }
}

void State::StopAllAnimations()
{
  EndAnimation();
  Animator & animator = m_framework->GetAnimator();
  animator.StopRotation();
  animator.StopChangeViewport();
  animator.StopMoveScreen();
}

ScreenBase const & State::GetModelView() const
{
  return m_framework->GetNavigator().Screen();
}

m2::PointD const State::GetRaFModeDefaultPxBind() const
{
  m2::RectD const & pixelRect = GetModelView().PixelRect();
  return m2::PointD(pixelRect.Center().x,
                    pixelRect.maxY() - POSITION_Y_OFFSET * visualScale());
}

void State::StopCompassFollowing()
{
  if (GetMode() != RotateAndFollow)
    return;

  StopAllAnimations();
  SetModeInfo(ChangeMode(m_modeInfo, Follow));
}

void State::StopLocationFollow()
{
  StopCompassFollowing();
  if (GetMode() > NotFollow)
  {
    StopAllAnimations();
    SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
  }
}

void State::DragStarted()
{
  m_dragModeInfo = m_modeInfo;
  m_afterPendingMode = NotFollow;
  StopLocationFollow();
}

void State::DragEnded()
{
  // reset GPS centering mode if we have dragged far from current location
  ScreenBase const & s = GetModelView();
  m2::PointD const defaultPxBinding = GetModeDefaultPixelBinding(ExcludeAllBits(m_dragModeInfo));
  m2::PointD const pxPosition = s.GtoP(Position());

  if (ExcludeAllBits(m_dragModeInfo) > NotFollow &&
      defaultPxBinding.Length(pxPosition) < s.GetMinPixelRectSize() / 5.0)
  {
    SetModeInfo(m_dragModeInfo);
  }

  m_dragModeInfo = 0;
}

void State::ScaleStarted()
{
  m_scaleModeInfo = m_modeInfo;
}

void State::CorrectScalePoint(m2::PointD & pt) const
{
  if (IsModeChangeViewport() || ExcludeAllBits(m_scaleModeInfo) > NotFollow)
    pt = m_framework->GtoP(Position());
}

void State::CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const
{
  if (IsModeChangeViewport() || ExcludeAllBits(m_scaleModeInfo) > NotFollow)
  {
    m2::PointD const ptDiff = m_framework->GtoP(Position()) - (pt1 + pt2) / 2;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }
}

void State::ScaleEnded()
{
  m_scaleModeInfo = 0;
}

void State::Rotated()
{
  m_afterPendingMode = NotFollow;
  EndAnimation();
  if (GetMode() == RotateAndFollow)
    SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
}

void State::OnSize(m2::RectD const & /*oldPixelRect*/)
{
  if (GetMode() == RotateAndFollow)
  {
    EndAnimation();
    CreateAnimTask(m_framework->GtoP(Position()), GetModeDefaultPixelBinding(GetMode()));
  }
}

namespace
{

#ifdef DEBUG
bool ValidateTransition(State::Mode oldMode, State::Mode newMode)
{
  if (oldMode == State::UnknownPosition)
    return newMode == State::PendingPosition;

  if (oldMode == State::PendingPosition)
  {
    return newMode == State::UnknownPosition ||
           newMode == State::NotFollow ||
           newMode == State::Follow;
  }

  if (oldMode == State::Follow)
  {
    return newMode == State::UnknownPosition ||
           newMode == State::NotFollow ||
           newMode == State::RotateAndFollow;
  }

  if (oldMode == State::NotFollow)
  {
    return newMode == State::Follow ||
           newMode == State::RotateAndFollow;
  }

  if (oldMode == State::RotateAndFollow)
  {
    return newMode == State::NotFollow ||
           newMode == State::Follow ||
           newMode == State::UnknownPosition;
  }

  return false;
}
#endif

}

void State::AnimateStateTransition(Mode oldMode, Mode newMode)
{
  ASSERT(ValidateTransition(oldMode, newMode), ("from", oldMode, "to", newMode));

  StopAllAnimations();

  if (oldMode == PendingPosition && newMode == Follow)
  {
    m2::PointD const size(m_errorRadius, m_errorRadius);
    m_framework->ShowRectExVisibleScale(m2::RectD(m_position - size, m_position + size),
                                        scales::GetUpperComfortScale());
  }
  else if (newMode == RotateAndFollow)
  {
    CreateAnimTask();
  }
  else if (oldMode == RotateAndFollow && newMode == UnknownPosition)
  {
    m_framework->GetAnimator().RotateScreen(GetModelView().GetAngle(), 0.0);
  }

  AnimateFollow();
}

void State::AnimateFollow()
{
  if (!IsModeChangeViewport())
    return;

  if (!FollowCompass())
  {
    if (!m_position.EqualDxDy(m_framework->GetViewportCenter(), POSITION_TOLERANCE))
      m_framework->SetViewportCenterAnimated(m_position);
  }
}

}
