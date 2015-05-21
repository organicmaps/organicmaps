#include "map/location_state.hpp"
#include "map/framework.hpp"

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

//static const int POSITION_Y_OFFSET = 120;
//static const double POSITION_TOLERANCE = 1.0E-6;  // much less than coordinates coding error
//static const double ANGLE_TOLERANCE = my::DegToRad(3.0);

//class RotateAndFollowAnim : public anim::Task
//{
//public:
//  RotateAndFollowAnim(Framework * fw, m2::PointD const & srcPos,
//                      double srcAngle,
//                      m2::PointD const & srcPixelBinding,
//                      m2::PointD const & dstPixelbinding)
//    : m_fw(fw)
//    , m_hasPendingAnimation(false)
//  {
//    //@TODO UVR
////    m_angleAnim.reset(new anim::SafeAngleInterpolation(srcAngle, srcAngle, 1.0));
////    m_posAnim.reset(new anim::SafeSegmentInterpolation(srcPos, srcPos, 1.0));
////    m2::PointD const srcInverted = InvertPxBinding(srcPixelBinding);
////    m2::PointD const dstInverted = InvertPxBinding(dstPixelbinding);
////    m_pxBindingAnim.reset(new anim::SafeSegmentInterpolation(srcInverted, dstInverted,
////                                                             m_fw->GetNavigator().ComputeMoveSpeed(srcInverted, dstInverted)));
//  }

//  void SetDestinationParams(m2::PointD const & dstPos, double dstAngle)
//  {
//    ASSERT(m_angleAnim != nullptr, ());
//    ASSERT(m_posAnim != nullptr, ());

//    if (IsVisual() || m_idleFrames > 0)
//    {
//      //Store new params even if animation is active but don't interrupt the current one.
//      //New animation to the pending params will be made after all.
//      m_hasPendingAnimation = true;
//      m_pendingDstPos = dstPos;
//      m_pendingAngle = dstAngle;
//    }
//    else
//      SetParams(dstPos, dstAngle);
//  }

//  void Update()
//  {
//    if (!IsVisual() && m_hasPendingAnimation && m_idleFrames == 0)
//    {
//      m_hasPendingAnimation = false;
//      SetParams(m_pendingDstPos, m_pendingAngle);
//      ///@TODO UVR
//      //m_fw->Invalidate();
//    }
//    else if (m_idleFrames > 0)
//    {
//      --m_idleFrames;
//      ///@TODO UVR
//      //m_fw->Invalidate();
//    }
//  }

//  m2::PointD const & GetPositionForDraw()
//  {
//    return m_posAnim->GetCurrentValue();
//  }

//  virtual void OnStep(double ts)
//  {
//    if (m_idleFrames > 0)
//      return;

//    ASSERT(m_angleAnim != nullptr, ());
//    ASSERT(m_posAnim != nullptr, ());
//    ASSERT(m_pxBindingAnim != nullptr, ());

//    bool updateViewPort = false;
//    updateViewPort |= OnStep(m_angleAnim.get(), ts);
//    updateViewPort |= OnStep(m_posAnim.get(), ts);
//    updateViewPort |= OnStep(m_pxBindingAnim.get(), ts);

//    if (updateViewPort)
//    {
//      UpdateViewport();
//      if (!IsVisual())
//        m_idleFrames = 5;
//    }
//  }

//  virtual bool IsVisual() const
//  {
//    ASSERT(m_posAnim != nullptr, ());
//    ASSERT(m_angleAnim != nullptr, ());
//    ASSERT(m_pxBindingAnim != nullptr, ());

//    return m_posAnim->IsRunning() ||
//           m_angleAnim->IsRunning() ||
//           m_pxBindingAnim->IsRunning();
//  }

//private:
//  void UpdateViewport()
//  {
//    ASSERT(m_posAnim != nullptr, ());
//    ASSERT(m_angleAnim != nullptr, ());
//    ASSERT(m_pxBindingAnim != nullptr, ());

//    m2::PointD const & pxBinding = m_pxBindingAnim->GetCurrentValue();
//    m2::PointD const & currentPosition = m_posAnim->GetCurrentValue();
//    double currentAngle = m_angleAnim->GetCurrentValue();

//    //@{ pixel coord system
//    m2::PointD const pxCenter = GetPixelRect().Center();
//    m2::PointD vectorToCenter = pxCenter - pxBinding;
//    if (!vectorToCenter.IsAlmostZero())
//      vectorToCenter = vectorToCenter.Normalize();
//    m2::PointD const vectorToTop = m2::PointD(0.0, 1.0);
//    double sign = m2::CrossProduct(vectorToTop, vectorToCenter) > 0 ? 1 : -1;
//    double angle = sign * acos(m2::DotProduct(vectorToTop, vectorToCenter));
//    //@}

//    //@{ global coord system
//    double offset = (m_fw->PtoG(pxCenter) - m_fw->PtoG(pxBinding)).Length();
//    m2::PointD const viewPoint = currentPosition.Move(1.0, currentAngle + my::DegToRad(90.0));
//    m2::PointD const viewVector = viewPoint - currentPosition;
//    m2::PointD rotateVector = viewVector;
//    rotateVector.Rotate(angle);
//    rotateVector.Normalize();
//    rotateVector *= offset;
//    //@}

//    ///@TODO UVR
////    m_fw->SetViewportCenter(currentPosition + rotateVector);
////    m_fw->GetNavigator().SetAngle(currentAngle);
//    //m_fw->Invalidate();
//  }

//  void SetParams(m2::PointD const & dstPos, double dstAngle)
//  {
//    ///@TODO UVR
////    double const angleDist = fabs(ang::GetShortestDistance(m_angleAnim->GetCurrentValue(), dstAngle));
////    if (dstPos.EqualDxDy(m_posAnim->GetCurrentValue(), POSITION_TOLERANCE) && angleDist < ANGLE_TOLERANCE)
////      return;

////    double const posSpeed = 2 * m_fw->GetNavigator().ComputeMoveSpeed(m_posAnim->GetCurrentValue(), dstPos);
////    double const angleSpeed = angleDist < 1.0 ? 1.5 : m_fw->GetAnimator().GetRotationSpeed();
////    m_angleAnim->ResetDestParams(dstAngle, angleSpeed);
////    m_posAnim->ResetDestParams(dstPos, posSpeed);
//  }

//  bool OnStep(anim::Task * task, double ts)
//  {
//    if (!task->IsReady() && !task->IsRunning())
//      return false;

//    if (task->IsReady())
//    {
//      task->Start();
//      task->OnStart(ts);
//    }

//    if (task->IsRunning())
//      task->OnStep(ts);

//    if (task->IsEnded())
//      task->OnEnd(ts);

//    return true;
//  }

//private:
//  m2::PointD InvertPxBinding(m2::PointD const & px) const
//  {
//    return m2::PointD(px.x, GetPixelRect().maxY() - px.y);
//  }

//  m2::RectD const & GetPixelRect() const
//  {
//    ///@TODO UVR
//    return m2::RectD();
//    //return m_fw->GetNavigator().Screen().PixelRect();
//  }

//private:
//  Framework * m_fw;

//  unique_ptr<anim::SafeAngleInterpolation> m_angleAnim;
//  unique_ptr<anim::SafeSegmentInterpolation> m_posAnim;
//  unique_ptr<anim::SafeSegmentInterpolation> m_pxBindingAnim;

//  bool m_hasPendingAnimation;
//  m2::PointD m_pendingDstPos;
//  double m_pendingAngle;
//  // When map has active animation, backgroung rendering pausing
//  // By this beetwen animations we wait some frames to release background rendering
//  int m_idleFrames = 0;
//};

}

//void State::RouteBuilded()
//{
//  StopAllAnimations();
//  SetModeInfo(IncludeModeBit(m_modeInfo, RoutingSessionBit));

//  Mode const mode = GetMode();
//  if (mode > NotFollow)
//    SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
//  else if (mode == UnknownPosition)
//  {
//    m_afterPendingMode = NotFollow;
//    SetModeInfo(ChangeMode(m_modeInfo, PendingPosition));
//  }
//}

//void State::StartRouteFollow()
//{
//  ASSERT(IsInRouting(), ());
//  ASSERT(IsModeHasPosition(), ());

//  m2::PointD const size(m_errorRadius, m_errorRadius);
//  m_framework->ShowRect(m2::RectD(m_position - size, m_position + size),
//                        scales::GetNavigationScale());

//  SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
//  SetModeInfo(ChangeMode(m_modeInfo, IsRotationActive() ? RotateAndFollow : Follow));
//}

//void State::StopRoutingMode()
//{
//  if (IsInRouting())
//  {
//    SetModeInfo(ChangeMode(ExcludeModeBit(m_modeInfo, RoutingSessionBit), GetMode() == RotateAndFollow ? Follow : NotFollow));
//    RotateOnNorth();
//    AnimateFollow();
//  }
//}


//m2::PointD const State::GetModeDefaultPixelBinding(State::Mode mode) const
//{
//  switch (mode)
//  {
//  case Follow: return m_framework->GetPixelCenter();
//  case RotateAndFollow: return GetRaFModeDefaultPxBind();
//  default: return m2::PointD(0.0, 0.0);
//  }
//}

//bool State::FollowCompass()
//{
//  if (!IsRotationActive() || GetMode() != RotateAndFollow || m_animTask == nullptr)
//    return false;

//  RotateAndFollowAnim * task = static_cast<RotateAndFollowAnim *>(m_animTask.get());
//  task->SetDestinationParams(Position(), -m_drawDirection);
//  return true;
//}

//m2::PointD const State::GetRaFModeDefaultPxBind() const
//{
//  return m2::PointD();
//  ///@TODO UVR
////  m2::RectD const & pixelRect = GetModelView().PixelRect();
////  return m2::PointD(pixelRect.Center().x,
////                    pixelRect.maxY() - POSITION_Y_OFFSET * visualScale());
//}

//void State::DragStarted()
//{
//  m_dragModeInfo = m_modeInfo;
//  m_afterPendingMode = NotFollow;
//  StopLocationFollow();
//}

//void State::DragEnded()
//{
//  // reset GPS centering mode if we have dragged far from current location
//  ScreenBase const & s = GetModelView();
//  m2::PointD const defaultPxBinding = GetModeDefaultPixelBinding(ExcludeAllBits(m_dragModeInfo));
//  m2::PointD const pxPosition = s.GtoP(Position());

//  if (ExcludeAllBits(m_dragModeInfo) > NotFollow &&
//      defaultPxBinding.Length(pxPosition) < s.GetMinPixelRectSize() / 5.0)
//  {
//    SetModeInfo(m_dragModeInfo);
//  }

//  m_dragModeInfo = 0;
//}

//void State::ScaleStarted()
//{
//  m_scaleModeInfo = m_modeInfo;
//}

//void State::CorrectScalePoint(m2::PointD & pt) const
//{
//  if (IsModeChangeViewport() || ExcludeAllBits(m_scaleModeInfo) > NotFollow)
//    pt = m_framework->GtoP(Position());
//}

//void State::CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const
//{
//  if (IsModeChangeViewport() || ExcludeAllBits(m_scaleModeInfo) > NotFollow)
//  {
//    m2::PointD const ptDiff = m_framework->GtoP(Position()) - (pt1 + pt2) / 2;
//    pt1 += ptDiff;
//    pt2 += ptDiff;
//  }
//}

//void State::ScaleEnded()
//{
//  m_scaleModeInfo = 0;
//}

//void State::Rotated()
//{
//  m_afterPendingMode = NotFollow;
//  EndAnimation();
//  if (GetMode() == RotateAndFollow)
//    SetModeInfo(ChangeMode(m_modeInfo, NotFollow));
//}

//void State::OnCompassTaped()
//{
//  StopCompassFollowing();
//  RotateOnNorth();
//  AnimateFollow();
//}

//void State::OnSize()
//{
//  if (GetMode() == RotateAndFollow)
//  {
//    EndAnimation();
//    CreateAnimTask(m_framework->GtoP(Position()), GetModeDefaultPixelBinding(GetMode()));
//  }
//}

//void State::AnimateStateTransition(Mode oldMode, Mode newMode)
//{
//  StopAllAnimations();

//  if (oldMode == PendingPosition && newMode == Follow)
//  {
//    if (!TestModeBit(m_modeInfo, FixedZoomBit))
//    {
//      m2::PointD const size(m_errorRadius, m_errorRadius);
//      m_framework->ShowRect(m2::RectD(m_position - size, m_position + size),
//                            scales::GetUpperComfortScale());
//    }
//  }
//  else if (newMode == RotateAndFollow)
//  {
//    CreateAnimTask();
//  }
//  else if (oldMode == RotateAndFollow && newMode == UnknownPosition)
//  {
//    RotateOnNorth();
//  }
//  else if (oldMode == NotFollow && newMode == Follow)
//  {
//    m2::AnyRectD screenRect = GetModelView().GlobalRect();
//    m2::RectD const & clipRect = GetModelView().ClipRect();
//    screenRect.Inflate(clipRect.SizeX() / 2.0, clipRect.SizeY() / 2.0);
//    if (!screenRect.IsPointInside(m_position))
//      m_framework->SetViewportCenter(m_position);
//  }

//  AnimateFollow();
//}

//void State::AnimateFollow()
//{
//  if (!IsModeChangeViewport())
//    return;

//  SetModeInfo(ExcludeModeBit(m_modeInfo, FixedZoomBit));

//  if (!FollowCompass())
//  {
//    ///@TODO UVR
////    if (!m_position.EqualDxDy(m_framework->GetViewportCenter(), POSITION_TOLERANCE))
////      m_framework->SetViewportCenterAnimated(m_position);
//  }
//}
}
