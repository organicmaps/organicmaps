#pragma once

#include "storage/index.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/timer.hpp"

#include "std/shared_ptr.hpp"


namespace location
{
  class State;
}

/// Class, which displays additional information on the primary layer like:
/// rules, coordinates, GPS position and heading, compass, Download button, etc.
class InformationDisplay
{
  shared_ptr<location::State> m_locationState;

  void InitLocationState();
public:
  enum class WidgetType {
    Ruler = 0,
    CopyrightLabel,
    CountryStatusDisplay,
    CompassArrow,
    DebugLabel
  };

  void setDisplayRect(m2::RectI const & rect);

  void measurementSystemChanged();

  shared_ptr<location::State> const & locationState() const;

  void ResetRouteMatchingInfo();

  void SetWidgetPivot(WidgetType widget, m2::PointD const & pivot);
  m2::PointD GetWidgetSize(WidgetType widget) const;
};
