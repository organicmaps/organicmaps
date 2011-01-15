#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

class DrawerYG;

/// Class, which displays additional information on the primary layer.
/// like rules, coordinates, GPS position and heading
class InformationDisplay
{
private:

  ScreenBase m_screen;
  m2::RectI m_displayRect;

  double m_headingOrientation;

  bool m_isHeadingEnabled;
  double m_trueHeading;
  double m_magneticHeading;
  double m_headingAccuracy;

  bool m_isPositionEnabled;
  m2::PointD m_position;
  double m_confidenceRadius;

  /// for debugging purposes
  /// up to 10 debugging points
  bool m_isDebugPointsEnabled;
  m2::PointD m_DebugPts[10];

  bool m_isRulerEnabled;
  m2::PointD m_basePoint;

  bool m_isCenterEnabled;
  m2::PointD m_centerPt;
  int m_currentScale;

  bool m_isDebugInfoEnabled;
  double m_frameDuration;
  double m_bottomShift;
  double m_visualScale;

public:

  InformationDisplay();

  void setScreen(ScreenBase const & screen);
  void setDisplayRect(m2::RectI const & rect);
  void setBottomShift(double bottomShift);
  void setVisualScale(double visualScale);
  void setOrientation(EOrientation orientation);

  void enablePosition(bool doEnable);
  void setPosition(m2::PointD const & mercatorPos, double confidenceRadius);
  m2::PointD const & position() const;
  void drawPosition(DrawerYG * pDrawer);

  void enableHeading(bool doEnable);
  void setHeading(double trueHeading, double magneticHeading, double accuracy);
  void drawHeading(DrawerYG * pDrawer);

  void enableDebugPoints(bool doEnable);
  void setDebugPoint(int pos, m2::PointD const & pt);
  void drawDebugPoints(DrawerYG * pDrawer);

  void enableRuler(bool doEnable);
  void drawRuler(DrawerYG * pDrawer);

  void enableCenter(bool doEnable);
  void setCenter(m2::PointD const & latLongPt);
  void drawCenter(DrawerYG * pDrawer);

  void enableDebugInfo(bool doEnable);
  void setDebugInfo(double frameDuration, int currentScale);
  void drawDebugInfo(DrawerYG * pDrawer);

  void doDraw(DrawerYG * drawer);
};
