#include "map/location_state.hpp"
#include "map/navigator.hpp"
#include "map/framework.hpp"
#include "map/move_screen_task.hpp"

#ifndef USE_DRAPE
#include "graphics/display_list.hpp"
#include "graphics/icon.hpp"
#include "graphics/depth_constants.hpp"
#endif // USE_DRAPE

#include "anim/controller.hpp"
#include "anim/task.hpp"
#include "anim/angle_interpolation.hpp"
#include "anim/segment_interpolation.hpp"

#include "gui/controller.hpp"

#include "indexer/mercator.hpp"
#include "indexer/scales.hpp"

#include "platform/location.hpp"
#include "platform/settings.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/transformations.hpp"
#include "3party/Alohalytics/src/alohalytics.h"


namespace location
{

namespace
{

static const int POSITION_Y_OFFSET = 75;
static const double POSITION_TOLERANCE = 1.0E-6;  // much less than coordinates coding error
static const double ANGLE_TOLERANCE = my::DegToRad(3.0);
static const double GPS_BEARING_LIFETIME_S = 5.0;


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
    , m_hasPendingAnimation(false)
  {
    m_angleAnim.reset(new anim::SafeAngleInterpolation(srcAngle, srcAngle, 1.0));
    m_posAnim.reset(new anim::SafeSegmentInterpolation(srcPos, srcPos, 1.0));
    m2::PointD const srcInverted = InvertPxBinding(srcPixelBinding);
    m2::PointD const dstInverted = InvertPxBinding(dstPixelbinding);
    m_pxBindingAnim.reset(new anim::SafeSegmentInterpolation(srcInverted, dstInverted,
                                                             m_fw->GetNavigator().ComputeMoveSpeed(srcInverted, dstInverted)));
  }

  void SetDestinationParams(m2::PointD const & dstPos, double dstAngle)
  {
    ASSERT(m_angleAnim != nullptr, ());
    ASSERT(m_posAnim != nullptr, ());

    if (IsVisual() || m_idleFrames > 0)
    {
      //Store new params even if animation is active but don't interrupt the current one.
      //New animation to the pending params will be made after all.
      m_hasPendingAnimation = true;
      m_pendingDstPos = dstPos;
      m_pendingAngle = dstAngle;
    }
    else
      SetParams(dstPos, dstAngle);
  }

  void Update()
  {
    if (!IsVisual() && m_hasPendingAnimation && m_idleFrames == 0)
    {
      m_hasPendingAnimation = false;
      SetParams(m_pendingDstPos, m_pendingAngle);
      m_fw->Invalidate();
    }
    else if (m_idleFrames > 0)
    {
      --m_idleFrames;
      m_fw->Invalidate();
    }
  }

  m2::PointD const & GetPositionForDraw()
  {
    return m_posAnim->GetCurrentValue();
  }

  virtual void OnStep(double ts)
  {
    if (m_idleFrames > 0)
      return;

    ASSERT(m_angleAnim != nullptr, ());
    ASSERT(m_posAnim != nullptr, ());
    ASSERT(m_pxBindingAnim != nullptr, ());

    bool updateViewPort = false;
    updateViewPort |= OnStep(m_angleAnim.get(), ts);
    updateViewPort |= OnStep(m_posAnim.get(), ts);
    updateViewPort |= OnStep(m_pxBindingAnim.get(), ts);

    if (updateViewPort)
    {
      UpdateViewport();
      if (!IsVisual())
        m_idleFrames = 5;
    }
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

  void SetParams(m2::PointD const & dstPos, double dstAngle)
  {
    double const angleDist = fabs(ang::GetShortestDistance(m_angleAnim->GetCurrentValue(), dstAngle));
    if (dstPos.EqualDxDy(m_posAnim->GetCurrentValue(), POSITION_TOLERANCE) && angleDist < ANGLE_TOLERANCE)
      return;

    double const posSpeed = 2 * m_fw->GetNavigator().ComputeMoveSpeed(m_posAnim->GetCurrentValue(), dstPos);
    double const angleSpeed = angleDist < 1.0 ? 1.5 : m_fw->GetAnimator().GetRotationSpeed();
    m_angleAnim->ResetDestParams(dstAngle, angleSpeed);
    m_posAnim->ResetDestParams(dstPos, posSpeed);
  }

  bool OnStep(anim::Task * task, double ts)
  {
    if (!task->IsReady() && !task->IsRunning())
      return false;

    if (task->IsReady())
    {
      task->Start();
      task->OnStart(ts);
    }

    if (task->IsRunning())
      task->OnStep(ts);

    if (task->IsEnded())
      task->OnEnd(ts);

    return true;
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

  bool m_hasPendingAnimation;
  m2::PointD m_pendingDstPos;
  double m_pendingAngle;
  // When map has active animation, backgroung rendering pausing
  // By this beetwen animations we wait some frames to release background rendering
  int m_idleFrames = 0;
};

string const LocationStateMode = "LastLocationStateMode";

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
    m_lastGPSBearing(false),
    m_afterPendingMode(Follow),
    m_routeMatchingInfo(),
    m_currentSlotID(0)
{
  m_locationAreaColor = p.m_locationAreaColor;
  m_framework = p.m_framework;

  int mode = 0;
  if (Settings::Get(LocationStateMode, mode))
    m_modeInfo = mode;

  bool isBench = false;
  if (Settings::Get("IsBenchmarking", isBench) && isBench)
    m_modeInfo = UnknownPosition;

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
  string const kAlohalyticsClickEvent = "$onClick";
  Mode currentMode = GetMode();
  Mode newMode = currentMode;

  if (!IsInRouting())
  {
    switch (currentMode)
    {
    case UnknownPosition:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@UnknownPosition");
      newMode = PendingPosition;
      break;
    case PendingPosition:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@PendingPosition");
      newMode = UnknownPosition;
      m_afterPendingMode = Follow;
      break;
    case NotFollow:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@NotFollow");
      newMode = Follow;
      break;
    case Follow:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@Follow");
      if (IsRotationActive())
        newMode = RotateAndFollow;
      else
      {
        newMode = UnknownPosition;
        m_afterPendingMode = Follow;
      }
      break;
    case RotateAndFollow:
      alohalytics::LogEvent(kAlohalyticsClickEvent, "@RotateAndFollow");
      newMode = UnknownPosition;
      m_afterPendingMode = Follow;
      break;
    }
  }
  else
    newMode = IsRotationActive() ? RotateAndFollow : Follow;

  SetModeInfo(ChangeMode(m_modeInfo, newMode));
}

void State::RouteBuilded()
{
  StopAllAnimations();
  SetModeInfo(IncludeModeBit(m_modeInfo, RoutingSessionBit));

  Mode const mode = GetMode();
  if (mode > NotFollow)
    SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
  else if (mode == UnknownPosition)
  {
    m_afterPendingMode = NotFollow;
    SetModeInfo(ChangeMode(m_modeInfo, PendingPosition));
  }
}

void State::StartRouteFollow(int scale)
{
  ASSERT(IsInRouting(), ());
  ASSERT(IsModeHasPosition(), ());

  m2::PointD const size(m_errorRadius, m_errorRadius);
  m_framework->ShowRectExVisibleScale(m2::RectD(m_position - size, m_position + size), scale);

  SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
  SetModeInfo(ChangeMode(m_modeInfo, IsRotationActive() ? RotateAndFollow : Follow));
}

void State::StopRoutingMode()
{
  if (IsInRouting())
  {
    SetModeInfo(ChangeMode(ExcludeModeBit(m_modeInfo, RoutingSessionBit), GetMode() == RotateAndFollow ? Follow : NotFollow));
    RotateOnNorth();
    AnimateFollow();
  }
}

void State::TurnOff()
{
  StopLocationFollow();
  SetModeInfo(UnknownPosition);
  setIsVisible(false);
  invalidate();
}

void State::OnLocationUpdate(location::GpsInfo const & info, bool isNavigable, location::RouteMatchingInfo const & routeMatchingInfo)
{
  Assign(info, isNavigable);
  m_routeMatchingInfo = routeMatchingInfo;

  setIsVisible(true);

  if (GetMode() == PendingPosition)
  {
    SetModeInfo(ChangeMode(m_modeInfo, m_afterPendingMode));
    m_afterPendingMode = Follow;
  }
  else
    AnimateFollow();

  CallPositionChangedListeners(m_position);
  invalidate();
}

void State::OnCompassUpdate(location::CompassInfo const & info)
{
  if (Assign(info))
  {
    AnimateFollow();
    invalidate();
  }
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
  Mode currentMode = GetMode();
  if (currentMode > PendingPosition)
  {
    SetModeInfo(ChangeMode(m_modeInfo, UnknownPosition));
    SetModeInfo(ChangeMode(m_modeInfo, PendingPosition));
    m_afterPendingMode = currentMode;
    setIsVisible(true);
  }
  else if (currentMode == UnknownPosition)
  {
    m_afterPendingMode = Follow;
    setIsVisible(false);
  }

  invalidate();
}

void State::cache()
{
#ifndef USE_DRAPE
  CachePositionArrow();
  CacheRoutingArrow();
  CacheLocationMark();

  m_controller->GetCacheScreen()->completeCommands();
#endif // USE_DRAPE
}

void State::purge()
{
#ifndef USE_DRAPE
  m_positionArrow.reset();
  m_locationMarkDL.reset();
  m_positionMarkDL.reset();
  m_routingArrow.reset();
#endif // USE_DRAPE
}

void State::update()
{
  if (isVisible() && IsModeHasPosition())
  {
    m2::PointD const pxPosition = m_framework->GetNavigator().GtoP(Position());
    setPivot(pxPosition, false);

    if (m_animTask)
      static_cast<RotateAndFollowAnim *>(m_animTask.get())->Update();
  }
}

void State::draw(graphics::OverlayRenderer * r,
                 math::Matrix<double, 3, 3> const & m) const
{
#ifndef USE_DRAPE
  if (!IsModeHasPosition() || !isVisible())
    return;

  checkDirtyLayout();

  m2::PointD const pxPosition = m_framework->GetNavigator().GtoP(Position());
  double const pxErrorRadius = pxPosition.Length(
        m_framework->GetNavigator().GtoP(Position() + m2::PointD(m_errorRadius, 0.0)));

  double const drawScale = pxErrorRadius / s_cacheRadius;
  m2::PointD const & pivotPosition = GetPositionForDraw();

  math::Matrix<double, 3, 3> locationDrawM = math::Shift(
                                               math::Scale(
                                                 math::Identity<double, 3>(),
                                                 drawScale,
                                                 drawScale),
                                               pivotPosition);

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
                                                pivotPosition);

    if (!IsInRouting())
      r->drawDisplayList(m_positionArrow.get(), compassDrawM * m);
    else
      r->drawDisplayList(m_routingArrow.get(), compassDrawM * m);
  }
  else
    r->drawDisplayList(m_positionMarkDL.get(), drawM);
#endif // USE_DRAPE
}

#ifndef USE_DRAPE
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

#endif // USE_DRAPE

bool State::IsRotationActive() const
{
  return IsDirectionKnown();
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

void State::SetModeInfo(uint16_t modeInfo, bool callListeners)
{
  Mode const newMode = ExcludeAllBits(modeInfo);
  Mode const oldMode = GetMode();
  m_modeInfo = modeInfo;
  if (newMode != oldMode)
  {
    Settings::Set(LocationStateMode, static_cast<int>(GetMode()));

    if (callListeners)
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

void State::StopLocationFollow(bool callListeners)
{
  Mode const currentMode = GetMode();
  if (currentMode > NotFollow)
  {
    StopAllAnimations();
    SetModeInfo(ChangeMode(m_modeInfo, NotFollow), callListeners);
  }
  else if (currentMode == PendingPosition)
  {
    StopAllAnimations();
    m_afterPendingMode = NotFollow;
  }
}

void State::SetFixedZoom()
{
  SetModeInfo(IncludeModeBit(m_modeInfo, FixedZoomBit));
}

void State::DragStarted()
{
  m_dragModeInfo = m_modeInfo;
  m_afterPendingMode = Follow;
  StopLocationFollow(false);
}

void State::DragEnded()
{
  Mode const currentMode = ExcludeAllBits(m_dragModeInfo);
  if (currentMode > NotFollow)
  {
    // reset GPS centering mode if we have dragged far from the current location
    ScreenBase const & s = GetModelView();
    m2::PointD const defaultPxBinding = GetModeDefaultPixelBinding(currentMode);
    m2::PointD const pxPosition = s.GtoP(Position());

    if (defaultPxBinding.Length(pxPosition) < s.GetMinPixelRectSize() / 5.0)
      SetModeInfo(m_dragModeInfo, false);
    else
      CallStateModeListeners();
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

void State::OnCompassTaped()
{
  StopCompassFollowing();
  RotateOnNorth();
  AnimateFollow();
}

void State::OnSize(m2::RectD const & /*oldPixelRect*/)
{
  if (GetMode() == RotateAndFollow)
  {
    EndAnimation();
    CreateAnimTask(m_framework->GtoP(Position()), GetModeDefaultPixelBinding(GetMode()));
  }
}

void State::AnimateStateTransition(Mode oldMode, Mode newMode)
{
  StopAllAnimations();

  if (oldMode == PendingPosition && newMode == Follow)
  {
    if (!TestModeBit(m_modeInfo, FixedZoomBit))
    {
      m2::PointD const size(m_errorRadius, m_errorRadius);
      m_framework->ShowRectExVisibleScale(m2::RectD(m_position - size, m_position + size),
                                          scales::GetUpperComfortScale());
    }
  }
  else if (newMode == RotateAndFollow)
  {
    CreateAnimTask();
  }
  else if (oldMode == RotateAndFollow && newMode == UnknownPosition)
  {
    RotateOnNorth();
  }
  else if (oldMode == NotFollow && newMode == Follow)
  {
    m2::AnyRectD screenRect = GetModelView().GlobalRect();
    m2::RectD const & clipRect = GetModelView().ClipRect();
    screenRect.Inflate(clipRect.SizeX() / 2.0, clipRect.SizeY() / 2.0);
    if (!screenRect.IsPointInside(m_position))
      m_framework->SetViewportCenter(m_position);
  }

  AnimateFollow();
}

void State::AnimateFollow()
{
  if (!IsModeChangeViewport())
    return;

  SetModeInfo(ExcludeModeBit(m_modeInfo, FixedZoomBit));

  if (!FollowCompass())
  {
    if (!m_position.EqualDxDy(m_framework->GetViewportCenter(), POSITION_TOLERANCE))
      m_framework->SetViewportCenterAnimated(m_position);
  }
}

void State::RotateOnNorth()
{
  m_framework->GetAnimator().RotateScreen(GetModelView().GetAngle(), 0.0);
}

void State::Assign(location::GpsInfo const & info, bool isNavigable)
{
  m2::RectD rect = MercatorBounds::MetresToXY(info.m_longitude,
                                              info.m_latitude,
                                              info.m_horizontalAccuracy);
  m_position = rect.Center();
  m_errorRadius = rect.SizeX() / 2;

  bool const hasBearing = info.HasBearing();
  if ((isNavigable && hasBearing)
      || (!isNavigable && hasBearing && info.HasSpeed() && info.m_speed > 1.0))
  {
    SetDirection(my::DegToRad(info.m_bearing));
    m_lastGPSBearing.Reset();
  }
}

bool State::Assign(location::CompassInfo const & info)
{
  if ((IsInRouting() && GetMode() >= Follow) ||
      (m_lastGPSBearing.ElapsedSeconds() < GPS_BEARING_LIFETIME_S))
    return false;

  SetDirection(info.m_bearing);
  return true;
}

void State::SetDirection(double bearing)
{
  m_drawDirection = bearing;
  SetModeInfo(IncludeModeBit(m_modeInfo, KnownDirectionBit));
}

void State::ResetDirection()
{
  SetModeInfo(ExcludeModeBit(m_modeInfo, KnownDirectionBit));
}

m2::PointD const State::GetPositionForDraw() const
{
  if (m_animTask != nullptr)
    return m_framework->GtoP(static_cast<RotateAndFollowAnim *>(m_animTask.get())->GetPositionForDraw());

  return pivot();
}

}
