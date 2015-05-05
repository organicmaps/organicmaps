#pragma once

#include "anim/task.hpp"

#include "geometry/screenbase.hpp"

#include "base/matrix.hpp"

#include "std/shared_ptr.hpp"

namespace df
{

/// Calculates screen parameters in navigation (dragging, scaling, etc.).
class Navigator
{
public:
  Navigator();

  void SetFromRect(m2::AnyRectD const & r);
  void CenterViewport(m2::PointD const & p);
  void SetFromRects(m2::AnyRectD const & glbRect, m2::RectD const & pxRect);

  void SaveState();
  /// @return false if can't load previously saved values
  bool LoadState();

  void OnSize(int w, int h);

  ScreenBase const & Screen() const { return m_Screen; }
  ScreenBase const & StartScreen() const { return m_StartScreen; }

  m2::PointD GtoP(m2::PointD const & pt) const;
  m2::PointD PtoG(m2::PointD const & pt) const;

  void StartDrag(m2::PointD const & pt);
  void DoDrag(m2::PointD const & pt);
  void StopDrag(m2::PointD const & pt);

  void StartScale(m2::PointD const & pt1, m2::PointD const & pt2);
  void DoScale(m2::PointD const & org, m2::PointD const & p1, m2::PointD const & p2);
  void DoScale(m2::PointD const & pt1, m2::PointD const & pt2);
  void StopScale(m2::PointD const & pt1, m2::PointD const & pt2);
  bool IsRotatingDuringScale() const;

  void Scale(m2::PointD const & pt, double factor);
  bool InAction() const;

private:
  bool CheckMinScale(ScreenBase const & screen) const;
  bool CheckMaxScale(ScreenBase const & screen) const;
  bool CheckBorders(ScreenBase const & screen) const;

  static bool CanShrinkInto(ScreenBase const & screen, m2::RectD const & boundRect);
  static ScreenBase const ShrinkInto(ScreenBase const & screen, m2::RectD boundRect);

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
  // Flag, which indicates, whether we are in the middle of some action.
  bool m_InAction;
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
};

m2::AnyRectD ToRotated(Navigator const & navigator, m2::RectD const & rect);
void CheckMinGlobalRect(m2::RectD & rect);

using TIsCountryLoaded = function<bool (m2::PointD const &)>;
void CheckMinMaxVisibleScale(TIsCountryLoaded const & fn, m2::RectD & rect, int maxScale/* = -1*/);

}
