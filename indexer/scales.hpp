#pragma once

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"

int const UPPER_STYLE_SCALE = 19;

namespace scales
{
  inline int GetUpperScale() { return 17; }
  inline int GetUpperStyleScale() { return UPPER_STYLE_SCALE; }
  inline int GetUpperWorldScale() { return 9; }

  double GetM2PFactor(int level);

  double GetScaleLevelD(double ratio);
  double GetScaleLevelD(m2::RectD const & r);
  int GetScaleLevel(double ratio);
  int GetScaleLevel(m2::RectD const & r);

  /// @return such ration, that GetScaleLevel(ration) == level
  double GetRationForLevel(double level);

  /// @return such rect, that GetScaleLevel(rect) == level
  m2::RectD GetRectForLevel(double level, m2::PointD const & center, double X2YRatio);

  double GetEpsilonForLevel(int level);
  double GetEpsilonForSimplify(int level);
  bool IsGoodForLevel(int level, m2::RectD const & r);
}
