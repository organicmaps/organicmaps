#pragma once

#include "geometry/screenbase.hpp"

namespace df
{
extern double const kDefault3dScale;

// Calculates screen parameters in navigation (dragging, scaling, etc.).
class Navigator
{
public:
  Navigator() = default;

  void SetFromRect(m2::AnyRectD const & r);
  void SetFromRect(m2::AnyRectD const & r, uint32_t tileSize, double visualScale);
  void SetFromScreen(ScreenBase const & screen);
  void SetFromScreen(ScreenBase const & screen, uint32_t tileSize, double visualScale);
  void CenterViewport(m2::PointD const & p);

  void OnSize(int w, int h);

  ScreenBase const & Screen() const { return m_Screen; }
  ScreenBase const & StartScreen() const { return m_StartScreen; }

  m2::PointD GtoP(m2::PointD const & pt) const;
  m2::PointD PtoG(m2::PointD const & pt) const;
  m2::PointD P3dtoP(m2::PointD const & pt) const;

  void StartDrag(m2::PointD const & pt);
  void DoDrag(m2::PointD const & pt);
  void StopDrag(m2::PointD const & pt);

  void StartScale(m2::PointD const & pt1, m2::PointD const & pt2);
  void DoScale(m2::PointD const & org, m2::PointD const & p1, m2::PointD const & p2);
  void DoScale(m2::PointD const & pt1, m2::PointD const & pt2);
  void StopScale(m2::PointD const & pt1, m2::PointD const & pt2);
  bool IsRotatingDuringScale() const;

  void Scale(m2::PointD const & pixelScaleCenter, double factor);
  bool InAction() const;

  void SetAutoPerspective(bool enable);
  void Enable3dMode();
  void SetRotationIn3dMode(double rotationAngle);
  void Disable3dMode();

private:
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
  bool m_InAction = false;
  // Should we check for threshold while scaling by two fingers.
  bool m_DoCheckRotationThreshold = false;
  // Do screen rotates during the two fingers scaling.
  bool m_IsRotatingDuringScale = false;
  // Used in DoScale and ScaleByPoint
  bool ScaleImpl(m2::PointD const & newPt1, m2::PointD const & newPt2, m2::PointD const & oldPt1,
                 m2::PointD const & oldPt2, bool skipMinScaleAndBordersCheck, bool doRotateScreen, ScreenBase & screen);
};

m2::AnyRectD ToRotated(Navigator const & navigator, m2::RectD const & rect);
void CheckMinGlobalRect(m2::RectD & rect, uint32_t tileSize, double visualScale, double scale3d = kDefault3dScale);
void CheckMinGlobalRect(m2::RectD & rect, double scale3d = kDefault3dScale);

void CheckMinMaxVisibleScale(m2::RectD & rect, int maxScale /* = -1 */, uint32_t tileSize, double visualScale,
                             double scale3d = kDefault3dScale);
void CheckMinMaxVisibleScale(m2::RectD & rect, int maxScale /* = -1 */, double scale3d = kDefault3dScale);
}  // namespace df
