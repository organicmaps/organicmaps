#include "location_state.hpp"
#include "navigator.hpp"
#include "framework.hpp"
#include "move_screen_task.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/icon.hpp"

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
#ifdef USE_FRAME_COUNT
static const int ANIM_THRESHOLD = 4;
#endif

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

class PxBindingAnim : public anim::SegmentInterpolation
{
  typedef anim::SegmentInterpolation TBase;
public:
  PxBindingAnim(m2::PointD const & start, m2::PointD const & end,
                double speed, m2::PointD & outValue)
    : TBase(start, end, speed, outValue)
  {
  }

  virtual bool IsVisual() const
  {
    return true;
  }
};

class RotateAndFollowAnim : public anim::Task
{
public:
  RotateAndFollowAnim(Framework * fw, m2::PointD const & srcPos,
                      double srcAngle, m2::PointD const & dstPixelBinding)
    : m_fw(fw)
    , m_currentPosition(srcPos)
    , m_currentAngle(srcAngle)
#ifdef USE_FRAME_COUNT
    , m_anglePrevState(EReady)
    , m_posPrevState(EReady)
    , m_freeFrameCount(0)
#endif
  {
    m_pxCurrentBinding = m_fw->GetPixelCenter();
    UpdatePxBindingAnim(dstPixelBinding);
  }

  void SetDestinationParams(m2::PointD const & dstPos, double dstAngle)
  {
    double const  posSpeed = m_fw->GetNavigator().ComputeMoveSpeed(m_currentPosition, dstPos);
    double const angleSpeed = m_fw->GetAnimator().GetRotationSpeed();

    UpdateInnerAnim(m_currentPosition, dstPos, posSpeed,
                    m_currentAngle, dstAngle, angleSpeed);
  }

  void SetDestinationPxBinding(m2::PointD const & pxBinding)
  {
    UpdatePxBindingAnim(pxBinding);
  }

  virtual void OnStart(double ts)
  {
    ASSERT(m_angleAnim != nullptr, ());
    ASSERT(m_posAnim != nullptr, ());
    if (m_angleAnim->IsReady())
    {
      m_angleAnim->Start();
      m_angleAnim->OnStart(ts);
    }

    if (m_posAnim->IsReady())
    {
      m_posAnim->Start();
      m_posAnim->OnStart(ts);
    }
  }

  virtual void OnStep(double ts)
  {
    ASSERT(m_angleAnim != nullptr, ());
    ASSERT(m_posAnim != nullptr, ());

    bool const angEnded = m_angleAnim->IsEnded();
    bool const posEnded = m_posAnim->IsEnded();
    if (angEnded && posEnded && m_pxBindingAnim->IsEnded())
      return;

    if (!angEnded)
      m_angleAnim->OnStep(ts);
    if (!posEnded)
      m_posAnim->OnStep(ts);

    UpdateViewport();

#ifdef USE_FRAME_COUNT
    AnimStateChanged();
#endif
  }

  virtual bool IsVisual() const
  {
    if (m_posAnim == nullptr || m_angleAnim == nullptr)
      return false;

#ifdef USE_FRAME_COUNT
    if (m_freeFrameCount < ANIM_THRESHOLD)
      return true;
#endif

    return m_posAnim->State() == anim::Task::EInProgress ||
           m_angleAnim->State() == anim::Task::EInProgress;
  }

private:
  void UpdatePxBindingAnim(m2::PointD const & dstPxBinding)
  {
    if (m_pxBindingAnim && !m_pxBindingAnim->IsEnded())
    {
      m_pxBindingAnim->Cancel();
      m_pxBindingAnim.reset();
    }

    m2::RectD const & pixelRect = m_fw->GetNavigator().Screen().PixelRect();
    m2::PointD yInvertedDstPixelBind(dstPxBinding.x, pixelRect.maxY() - dstPxBinding.y);
    m_pxBindingAnim.reset(new PxBindingAnim(m_pxCurrentBinding, yInvertedDstPixelBind,
                                            0.5, m_pxCurrentBinding));
    m_fw->GetAnimController()->AddTask(m_pxBindingAnim);
  }

  void UpdateViewport()
  {
    //@{ pixel coord system
    m2::RectD const pixelRect = m_fw->GetNavigator().Screen().PixelRect();
    m2::PointD const pxCenter = pixelRect.Center();
    m2::PointD vectorToCenter = pxCenter - m_pxCurrentBinding;
    if (!vectorToCenter.IsAlmostZero())
      vectorToCenter = vectorToCenter.Normalize();
    m2::PointD const vectorToTop = m2::PointD(0.0, 1.0);
    double sign = m2::CrossProduct(vectorToTop, vectorToCenter) > 0 ? 1 : -1;
    double angle = sign * acos(m2::DotProduct(vectorToTop, vectorToCenter));
    //@}

    //@{ global coord system
    double offset = (m_fw->PtoG(pxCenter) - m_fw->PtoG(m_pxCurrentBinding)).Length();
    m2::PointD const viewPoint = m_currentPosition.Move(1.0, m_currentAngle + my::DegToRad(90.0));
    m2::PointD const viewVector = viewPoint - m_currentPosition;
    m2::PointD rotateVector = viewVector;
    rotateVector.Rotate(angle);
    rotateVector.Normalize();
    rotateVector *= offset;
    //@}

    m_fw->SetViewportCenter(m_currentPosition + rotateVector);
    m_fw->GetNavigator().SetAngle(m_currentAngle);
    m_fw->Invalidate();
  }

  void UpdateInnerAnim(m2::PointD const & srcPos, m2::PointD const & dstPos, double posSpeed,
                       double srcAngle, double dstAngle, double angleSpeed)
  {
    if (m_angleAnim == nullptr)
      m_angleAnim.reset(new anim::AngleInterpolation(srcAngle, dstAngle, angleSpeed, m_currentAngle));
    else
      m_angleAnim->Reset(srcAngle, dstAngle, angleSpeed);

    if (m_posAnim == nullptr)
      m_posAnim.reset(new anim::SegmentInterpolation(srcPos, dstPos, posSpeed, m_currentPosition));
    else
      m_posAnim->Reset(srcPos, dstPos, posSpeed);
  }

#ifdef USE_FRAME_COUNT
  void AnimStateChanged()
  {
    anim::Task::EState const  angleState = m_angleAnim->State();
    anim::Task::EState const  posState = m_posAnim->State();
    if ((posState == anim::Task::EEnded && angleState == anim::Task::EEnded) &&
        (posState != m_posPrevState || angleState != m_anglePrevState))
    {
      ++m_freeFrameCount;
    }
    else if ((angleState == anim::Task::EInProgress || posState == anim::Task::EInProgress) &&
             m_freeFrameCount > ANIM_THRESHOLD)
    {
      m_freeFrameCount = 0;
    }

    m_posPrevState = posState;
    m_anglePrevState = angleState;
  }
#endif

private:
  Framework * m_fw;

  unique_ptr<anim::AngleInterpolation> m_angleAnim;
  unique_ptr<anim::SegmentInterpolation> m_posAnim;

  shared_ptr<PxBindingAnim> m_pxBindingAnim;

  m2::PointD m_currentPosition;
  double m_currentAngle;

  m2::PointD m_pxCurrentBinding;
#ifdef USE_FRAME_COUNT
  anim::Task::EState m_anglePrevState;
  anim::Task::EState m_posPrevState;
  int m_freeFrameCount;
#endif
};

}

State::Params::Params()
  : m_locationAreaColor(0, 0, 0, 0),
    m_framework(0)
{}

State::State(Params const & p)
  : TBase(p),
    m_modeInfo(UnknownPosition),
    m_errorRadius(0),
    m_position(0, 0),
    m_drawDirection(0.0),
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
      break;
    case NotFollow:
      newMode = Follow;
      break;
    case Follow:
      if (IsRotationActive())
        newMode = RotateAndFollow;
      else
        newMode = UnknownPosition;
      break;
    case RotateAndFollow:
      newMode = UnknownPosition;
      break;
    }
  }
  else
    newMode = IsRotationActive() ? RotateAndFollow : Follow;

  if (newMode > NotFollow)
    SetCurrentPixelBinding(GetModeDefaultPixelBinding(newMode));

  SetModeInfo(ChangeMode(m_modeInfo, newMode));
}

void State::StartRoutingMode()
{
  ASSERT(GetPlatform().HasRouting(), ());
  ASSERT(IsModeHasPosition(), ());
  State::Mode newMode = IsRotationActive() ? RotateAndFollow : Follow;
  SetModeInfo(ChangeMode(IncludeModeBit(m_modeInfo, RoutingSessionBit), newMode));
}

void State::StopRoutingMode()
{
  SetModeInfo(ExcludeModeBit(m_modeInfo, RoutingSessionBit));
}

void State::TurnOff()
{
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
  {
    SetCurrentPixelBinding(GetModeDefaultPixelBinding(Follow));
    SetModeInfo(ChangeMode(m_modeInfo, Follow));
  }
  else
    AnimateFollow();

  CallPositionChangedListeners(m_position);
  invalidate();
}

void State::OnCompassUpdate(location::CompassInfo const & info)
{
  SetModeInfo(IncludeModeBit(m_modeInfo, KnownDirectionBit));

  if (info.m_trueHeading >= 0.0)
    m_drawDirection = info.m_trueHeading;
  else
    m_drawDirection = info.m_magneticHeading;

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
  SetModeInfo(ChangeMode(m_modeInfo, PendingPosition));
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
    double rotateAngle = m_drawDirection + m_framework->GetNavigator().Screen().GetAngle();

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
                          depth() - 3);

  cacheScreen->setDisplayList(m_positionMarkDL.get());
  cacheScreen->drawSymbol(m2::PointD(0, 0),
                          "current-position",
                          graphics::EPosCenter,
                          depth() - 1);

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
                                       4, depth(), res->m_pipelineID);
  cacheScreen->setDisplayList(0);
  cacheScreen->endFrame();
}

bool State::IsRotationActive() const
{
  return m_framework->GetNavigator().DoSupportRotation() && IsDirectionKnown();
}

bool State::IsDirectionKnown() const
{
  return TestModeBit(m_modeInfo, KnownDirectionBit);
}

bool State::IsInRouting() const
{
  return TestModeBit(m_modeInfo, RoutingSessionBit);
}

m2::PointD const & State::GetCurrentPixelBinding() const
{
  return m_dstPixelBinding;
}

void State::SetCurrentPixelBinding(m2::PointD const & pxBinding)
{
  m_dstPixelBinding = pxBinding;
  if (m_animTask)
  {
    RotateAndFollowAnim * anim = static_cast<RotateAndFollowAnim *>(m_animTask.get());
    anim->SetDestinationPxBinding(pxBinding);
  }
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
  return  true;
}

void State::CreateAnimTask()
{
  EndAnimation();
  m_animTask.reset(new RotateAndFollowAnim(m_framework, m_framework->PtoG(m_framework->GetPixelCenter()),
                                           m_framework->GetNavigator().Screen().GetAngle(), GetCurrentPixelBinding()));
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

m2::PointD const State::GetRaFModeDefaultPxBind() const
{
  m2::RectD const & pixelRect = m_framework->GetNavigator().Screen().PixelRect();
  return m2::PointD(pixelRect.Center().x,
                    pixelRect.maxY() - POSITION_Y_OFFSET * visualScale());
}

void State::StopCompassFollowing(Mode mode)
{
  if (GetMode() != RotateAndFollow)
    return;

  SetModeInfo(ChangeMode(m_modeInfo, mode));

  m_framework->GetAnimator().StopRotation();
  m_framework->GetAnimator().StopChangeViewport();
  m_framework->GetAnimator().StopMoveScreen();
}

void State::StopLocationFollow()
{
  StopCompassFollowing();
  if (GetMode() > NotFollow)
    SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
}

void State::DragStarted()
{
  m_dragModeInfo = m_modeInfo;
}

void State::Draged()
{
  if (!IsModeChangeViewport())
    return;

  StopCompassFollowing();
  SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
}

void State::DragEnded()
{
  // reset GPS centering mode if we have dragged far from current location
  ScreenBase const & s = m_framework->GetNavigator().Screen();
  m2::PointD const & pxBinding = GetCurrentPixelBinding();
  m2::PointD const defaultPxBinding = GetModeDefaultPixelBinding(ExcludeAllBits(m_dragModeInfo));
  m2::PointD const pxPosition = s.GtoP(Position());

  if (ExcludeAllBits(m_dragModeInfo) > NotFollow &&
      pxBinding == defaultPxBinding &&
      pxBinding.Length(pxPosition) < s.GetMinPixelRectSize() / 5.0)
  {
    SetModeInfo(m_dragModeInfo);
  }
  else if (IsInRouting() && s.PixelRect().IsPointInside(pxPosition))
  {
    SetCurrentPixelBinding(pxPosition);
    SetModeInfo(ChangeMode(m_modeInfo, RotateAndFollow));
  }
}

void State::ScaleCorrection(m2::PointD & pt)
{
  if (IsModeChangeViewport())
    pt = m_framework->GetPixelCenter();
}

void State::ScaleCorrection(m2::PointD & pt1, m2::PointD & pt2)
{
  if (IsModeChangeViewport())
  {
    m2::PointD const ptC = (pt1 + pt2) / 2;
    m2::PointD const ptDiff = m_framework->GetPixelCenter() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }
}

void State::Rotated()
{
  StopCompassFollowing(IsInRouting() ? NotFollow : Follow);
}

void State::OnSize(m2::RectD const & oldPixelRect)
{
  if (!IsModeChangeViewport())
    return;

  m2::PointD const & currentPxOffset =  GetCurrentPixelBinding() - oldPixelRect.Center();
  double oldHalfW = oldPixelRect.SizeX() / 2.0;
  double oldHalfH = oldPixelRect.SizeY() / 2.0;
  double wPart = currentPxOffset.x / oldHalfW;
  double hPart = currentPxOffset.y / oldHalfH;

  m2::RectD const & newPixelRect = m_framework->GetNavigator().Screen().PixelRect();
  m2::PointD const newCenter = newPixelRect.Center();
  double newHalfW = newPixelRect.SizeX() / 2.0;
  double newHalfH = newPixelRect.SizeY() / 2.0;

  SetCurrentPixelBinding(newCenter + m2::PointD(wPart * newHalfW, hPart * newHalfH));
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

  if (oldMode == RotateAndFollow)
    EndAnimation();
  if (oldMode == Follow)
    m_framework->GetAnimator().StopMoveScreen();

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
    m_framework->GetAnimator().RotateScreen(m_framework->GetNavigator().Screen().GetAngle(), 0.0);

  AnimateFollow();
}

void State::AnimateFollow()
{
  if (!IsModeChangeViewport())
    return;

  if (!FollowCompass())
    m_framework->SetViewportCenterAnimated(Position());
}

}
