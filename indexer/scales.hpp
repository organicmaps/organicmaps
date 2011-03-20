#pragma once

#include "../geometry/rect2d.hpp"

namespace scales
{
  inline int GetUpperScale() { return 17; }
  inline int GetUpperWorldScale() { return 6; }

  double GetM2PFactor(int level);
  int GetScaleLevel(double ratio);
  int GetScaleLevel(m2::RectD const & r);
  double GetEpsilonForLevel(int level);
  double GetEpsilonForSimplify(int level);
  bool IsGoodForLevel(int level, m2::RectD const & r);
}
