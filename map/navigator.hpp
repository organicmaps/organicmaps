#pragma once
#include "scales_processor.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/matrix.hpp"
#include "../base/scheduled_task.hpp"

#include "../std/function.hpp"
#include "../std/scoped_ptr.hpp"


/// Calculates screen parameters in navigation (dragging, scaling, etc.).
class Navigator
{
public:
  typedef function<void ()> invalidate_fn;
  explicit Navigator(ScalesProcessor const & scales, invalidate_fn const & invalidateFn);

  void SetFromRect(m2::AnyRectD const & r);
  void CenterViewport(m2::PointD const & p);
  void SetFromRects(m2::AnyRectD const & glbRect, m2::RectD const & pxRect);

  double ComputeMoveSpeed(m2::PointD const & p0, m2::PointD const & p1) const;

  void SaveState();
  /// @return false if can't load previously saved values
  bool LoadState();

  void OnSize(int x0, int y0, int w, int h);

  ScreenBase const & Screen() const { return m_Screen; }
  ScreenBase const & StartScreen() const { return m_StartScreen; }

  m2::PointD GtoP(m2::PointD const & pt) const;
  m2::PointD PtoG(m2::PointD const & pt) const;

  void GetTouchRect(m2::PointD const & pixPoint, double pixRadius, m2::AnyRectD & glbRect) const;

  void StartDrag(m2::PointD const & pt, double timeInSec);
  void DoDrag(m2::PointD const & pt, double timeInSec);
  void StopDrag(m2::PointD const & pt, double timeInSec, bool animate);

  void StartRotate(double Angle, double timeInSec);
  void DoRotate(double Angle, double timeInSec);
  void StopRotate(double Angle, double timeInSec);

  void StartScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec);
  void DoScale(m2::PointD const & org, m2::PointD const & p1, m2::PointD const & p2);
  void DoScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec);
  void StopScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec);
  bool IsRotatingDuringScale() const;

  void ScaleToPoint(m2::PointD const & pt, double factor, double timeInSec);

  void Scale(double scale);
  void Rotate(double angle);
  void SetAngle(double angle);
  void SetOrg(m2::PointD const & org);

  void Move(double azDir, double factor);

  // Returns true if another update is necessary, i.e. animation is not finished.
  bool Update(double timeInSec);

  bool InAction() const;

  /// enabling/disabling screen rotation handling
  void SetSupportRotation(bool flag);
  /// checking, whether the navigator supports rotation
  bool DoSupportRotation() const;
  /// Our surface is a square which is bigger than visible screen area on the device,
  /// so we should take it into an account
  m2::PointD ShiftPoint(m2::PointD const & pt) const;

private:
  ScalesProcessor const & m_scales;

  bool CheckMinScale(ScreenBase const & screen) const;
  bool CheckMaxScale(ScreenBase const & screen) const;
  bool CheckBorders(ScreenBase const & screen) const;

  static bool CanShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect);
  static ScreenBase const ShrinkInto(ScreenBase const & screen, m2::RectD boundRect);

  static bool CanRotateInto(ScreenBase const & screen, m2::RectD const & boundRect);
  static ScreenBase const RotateInto(ScreenBase const & screen, m2::RectD const & boundRect);

  static ScreenBase const ScaleInto(ScreenBase const & screen, m2::RectD boundRect);
  static ScreenBase const ShrinkAndScaleInto(ScreenBase const & screen, m2::RectD boundRect);

  // Internal screen corresponding to the state when navigation began with StartDrag or StartScale.
  ScreenBase m_StartScreen;
  // Internal screen to do GtoP() and PtoG() calculations. It is always up to date with navigation.
  ScreenBase m_Screen;
  // Intial point for dragging and scaling.
  m2::PointD m_StartPt1;
  // Last point for dragging and scaling.
  m2::PointD m_LastPt1;
  // Second initial point for scaling.
  m2::PointD m_StartPt2;
  // Second Last point for scaling.
  m2::PointD m_LastPt2;
  // Start value of rotation angle
  double m_StartAngle;
  // Current screen speed in post-dragging animation.
  double m_DragAnimationSpeed;
  // Move direction of the screen in post-dragging animation.
  double m_DragAnimationDirection;
  // Last update time.
  double m_LastUpdateTimeInSec;
  // Delta matrix which stores transformation between m_StartScreen and m_Screen.
  math::Matrix<float, 3, 3> m_DeltaMatrix;
  // Flag, which indicates, whether we are in the middle of some action.
  bool m_InAction;
  bool m_InMomentScaleAction;
  // Does Navigator supports screen rotation.
  bool m_DoSupportRotation;
  // Should we check for threshold while scaling by two fingers.
  bool m_DoCheckRotationThreshold;
  // Do screen rotates during the two fingers scaling.
  bool m_IsRotatingDuringScale;
  // Used in DoScale and ScaleByPoint
  bool ScaleImpl(m2::PointD const & newPt1,
                 m2::PointD const & newPt2,
                 m2::PointD const & oldPt1,
                 m2::PointD const & oldPt2,
                 bool skipMinScaleAndBordersCheck,
                 bool doRotateScreen);

  void ResetMomentScaleAction();
  void StartMomentScaleReseter();
  void KillMomentScalereseter();

  scoped_ptr<ScheduledTask> m_reseterTask;
  invalidate_fn m_invalidateFn;
};
