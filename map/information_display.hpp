#pragma once

#include "../gui/button.hpp"

#include "../graphics/font_desc.hpp"

#include "../storage/index.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../base/timer.hpp"

#include "../std/shared_ptr.hpp"


namespace location
{
  class State;
}

class Drawer;

namespace gui
{
  class Button;
  class Controller;
  class CachedTextView;
}

class Framework;
class CountryStatusDisplay;
class CompassArrow;
class Ruler;

/// Class, which displays additional information on the primary layer like:
/// rules, coordinates, GPS position and heading, compass, Download button, etc.
class InformationDisplay
{
  Framework * m_framework;
  graphics::FontDesc m_fontDesc;

  m2::RectI m_displayRect;
  int m_yOffset;

  /// For debugging purposes up to 10 drawable points.
  bool m_isDebugPointsEnabled;
  m2::PointD m_DebugPts[10];

  shared_ptr<Ruler> m_ruler;

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

  double m_visualScale;

  my::Timer m_lastMemoryWarning;
  bool m_isMemoryWarningEnabled;

  shared_ptr<CountryStatusDisplay> m_countryStatusDisplay;
  shared_ptr<CompassArrow> m_compassArrow;
  shared_ptr<location::State> m_locationState;
  shared_ptr<gui::CachedTextView> m_debugLabel;
  shared_ptr<gui::CachedTextView> m_copyrightLabel;

  void InitRuler(Framework * fw);
  void InitDebugLabel();
  void InitLocationState(Framework * fw);
  void InitCompassArrow(Framework * fw);
  void InitCountryStatusDisplay(Framework * fw);

  void InitCopyright(Framework * fw);

public:

  InformationDisplay(Framework * framework);

  void setController(gui::Controller * controller);

  void setDisplayRect(m2::RectI const & rect);
  void setVisualScale(double visualScale);

  void enableDebugPoints(bool doEnable);
  void setDebugPoint(int pos, m2::PointD const & pt);
  void drawDebugPoints(Drawer * pDrawer);

  bool isCopyrightActive() const;

  void enableRuler(bool doEnable);
  bool isRulerEnabled() const;

  void enableDebugInfo(bool doEnable);
  void setDebugInfo(double frameDuration, int currentScale);

  void enableMemoryWarning(bool doEnable);
  void memoryWarning();
  void drawMemoryWarning(Drawer * pDrawer);

  void measurementSystemChanged();

  void enableBenchmarkInfo(bool doEnable);
  bool addBenchmarkInfo(string const & name, m2::RectD const & globalRect, double frameDuration);
  void drawBenchmarkInfo(Drawer * pDrawer);

  void doDraw(Drawer * drawer);

  void enableCompassArrow(bool doEnable);
  bool isCompassArrowEnabled() const;
  void setCompassArrowAngle(double angle);

  shared_ptr<location::State> const & locationState() const;

  void enableCountryStatusDisplay(bool doEnable);
  void setDownloadListener(gui::Button::TOnClickListener const & l);
  void setEmptyCountryIndex(storage::TIndex const & idx);

  shared_ptr<CountryStatusDisplay> const & countryStatusDisplay() const;
};
