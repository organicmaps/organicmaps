#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/point2d.hpp"


namespace scales
{
  int const UPPER_STYLE_SCALE = 19;

  /// Upper scale for data generation and indexer buckets.
  inline int GetUpperScale() { return 17; }
  /// Upper scale according to drawing rules.
  inline int GetUpperStyleScale() { return UPPER_STYLE_SCALE; }
  /// Upper scales for World visible styles and indexer buckets.
  inline int GetUpperWorldScale() { return 9; }
  /// Upper scale for user comfort view (e.g. location zoom).
  inline int GetUpperComfortScale() { return UPPER_STYLE_SCALE - 2; }
  /// Default navigation mode scale
  inline int GetNavigationScale() { return UPPER_STYLE_SCALE - 3; }
  /// Default pedestrian navigation mode scale
  inline int GetPedestrianNavigationScale() { return UPPER_STYLE_SCALE - 2; }
  /// Default navigation 3d mode scale
  inline int GetNavigation3dScale() { return UPPER_STYLE_SCALE - 2; }
  /// Default pedestrian navigation 3d mode scale
  inline int GetPedestrianNavigation3dScale() { return UPPER_STYLE_SCALE - 2; }

  int GetMinAllowableIn3dScale();

  double GetScaleLevelD(double ratio);
  double GetScaleLevelD(m2::RectD const & r);
  int GetScaleLevel(double ratio);
  int GetScaleLevel(m2::RectD const & r);

  /// @return such ration, that GetScaleLevel(ration) == level
  double GetRationForLevel(double level);

  /// @return such rect, that GetScaleLevel(rect) == level
  m2::RectD GetRectForLevel(double level, m2::PointD const & center);

  double GetEpsilonForLevel(int level);
  double GetEpsilonForSimplify(int level);
  bool IsGoodForLevel(int level, m2::RectD const & r);
}
