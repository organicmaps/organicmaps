#pragma once

#include "window_handle.hpp"
#include "ruler.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"
#include "../base/timer.hpp"
#include "../base/logging.hpp"
#include "../gui/button.hpp"

namespace location
{
  class State;
}

class DrawerYG;

namespace gui
{
  class Button;
  class Controller;
}

class Framework;
class CountryStatusDisplay;
class CompassArrow;

/// Class, which displays additional information on the primary layer.
/// like rules, coordinates, GPS position and heading
class InformationDisplay
{
private:

  yg::FontDesc m_fontDesc;

  ScreenBase m_screen;
  m2::RectI m_displayRect;
  int m_yOffset;

  /// for debugging purposes
  /// up to 10 debugging points
  bool m_isDebugPointsEnabled;
  m2::PointD m_DebugPts[10];

  bool m_isRulerEnabled;
  Ruler m_ruler;

  bool m_isCenterEnabled;
  m2::PointD m_centerPtLonLat;
  int m_currentScale;

  bool m_isDebugInfoEnabled;
  double m_frameDuration;

  shared_ptr<gui::Button> m_downloadButton;
  gui::Controller * m_controller;

  bool m_isBenchmarkInfoEnabled;

  struct BenchmarkInfo
  {
    string m_name;
    m2::RectD m_rect;
    double m_duration;
  };

  vector<BenchmarkInfo> m_benchmarkInfo;

  double m_bottomShift;
  double m_visualScale;

  my::Timer m_lastMemoryWarning;
  bool m_isMemoryWarningEnabled;

  /*
  static bool s_isLogEnabled;
  static my::LogMessageFn s_oldLogFn;
  static list<string> s_log;
  static size_t s_logSize;
  static WindowHandle * s_windowHandle;
  */
  shared_ptr<CountryStatusDisplay> m_countryStatusDisplay;
  shared_ptr<CompassArrow> m_compassArrow;
  shared_ptr<location::State> m_locationState;

public:

  InformationDisplay(Framework * framework);

  void setController(gui::Controller * controller);

  void setScreen(ScreenBase const & screen);
  void setDisplayRect(m2::RectI const & rect);
  void setBottomShift(double bottomShift);
  void setVisualScale(double visualScale);

  void enableDebugPoints(bool doEnable);
  void setDebugPoint(int pos, m2::PointD const & pt);
  void drawDebugPoints(DrawerYG * pDrawer);

  void enableRuler(bool doEnable);
  void drawRuler(DrawerYG * pDrawer);
  void setRulerParams(unsigned pxMinWidth, double metresMinWidth, double metresMaxWidth);
  void setupRuler();

  void enableCenter(bool doEnable);
  void setCenter(m2::PointD const & latLongPt);
  void drawCenter(DrawerYG * pDrawer);

  void enableDebugInfo(bool doEnable);
  void setDebugInfo(double frameDuration, int currentScale);
  void drawDebugInfo(DrawerYG * pDrawer);

  void enableMemoryWarning(bool doEnable);
  void memoryWarning();
  void drawMemoryWarning(DrawerYG * pDrawer);

  void drawPlacemark(DrawerYG * pDrawer, string const & symbol, m2::PointD const & pt);

  void enableBenchmarkInfo(bool doEnable);
  bool addBenchmarkInfo(string const & name, m2::RectD const & globalRect, double frameDuration);
  void drawBenchmarkInfo(DrawerYG * pDrawer);

  void doDraw(DrawerYG * drawer);

  void enableLog(bool doEnable, WindowHandle * windowHandle);
  void setLogSize(size_t logSize);
  void drawLog(DrawerYG * pDrawer);

  void enableCompassArrow(bool doEnable);
  void setCompassArrowAngle(double angle);

  shared_ptr<location::State> const & locationState() const;

  void enableCountryStatusDisplay(bool doEnable);
  void setDownloadListener(gui::Button::TOnClickListener l);
  void setEmptyCountryName(string const & country);

  shared_ptr<CountryStatusDisplay> const & countryStatusDisplay() const;

  static void logMessage(my::LogLevel, my::SrcPoint const &, string const &);
};
