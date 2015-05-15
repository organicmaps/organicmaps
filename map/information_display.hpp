#pragma once

#include "gui/button.hpp"

#include "graphics/font_desc.hpp"

#include "storage/index.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/timer.hpp"

#include "std/shared_ptr.hpp"


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

  shared_ptr<Ruler> m_ruler;
  gui::Controller * m_controller;

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

  bool isCopyrightActive() const;
  void enableCopyright(bool doEnable);

  void enableRuler(bool doEnable);
  bool isRulerEnabled() const;

  void enableDebugInfo(bool doEnable);
  void setDebugInfo(double frameDuration, int currentScale);

  void measurementSystemChanged();

  void enableCompassArrow(bool doEnable);
  bool isCompassArrowEnabled() const;
  void setCompassArrowAngle(double angle);

  shared_ptr<location::State> const & locationState() const;

  void setEmptyCountryIndex(storage::TIndex const & idx);

  shared_ptr<CountryStatusDisplay> const & countryStatusDisplay() const;

  void ResetRouteMatchingInfo();
};
