#include "scales.hpp"
#include "mercator.hpp"

#include "../base/math.hpp"

#include "../std/algorithm.hpp"

#include "../base/start_mem_debug.hpp"

namespace scales
{
  /// @name This parameters should be tuned.
  //@{
  static const int initial_level = 1;

  double GetM2PFactor(int level)
  {
    int const base_scale = 14;
    int const factor = 1 << my::Abs(level - base_scale);

    if (level < base_scale)
      return 1 / double(factor);
    else
      return factor;
  }
  //@}

  int GetScaleLevel(double ratio)
  {
    double const level = min(static_cast<double>(GetUpperScale()), log(ratio) / log(2.0) + initial_level);
    return (level < 0 ? 0 : static_cast<int>(level + 0.5));
  }

  int GetScaleLevel(m2::RectD const & r)
  {
    // TODO: fix scale coefficients for mercator
    double const dx = (MercatorBounds::maxX - MercatorBounds::minX) / r.SizeX();
    double const dy = (MercatorBounds::maxY - MercatorBounds::minY) / r.SizeY();

    // get the average ratio
    return GetScaleLevel((dx + dy) / 2.0);
  }

  double GetEpsilonForLevel(int level)
  {
    return (MercatorBounds::maxX - MercatorBounds::minX) / pow(2.0, double(level + 6 - initial_level));
  }

  bool IsGoodForLevel(int level, m2::RectD const & r)
  {
    // assume that feature is always visible in upper scale
    return (level == GetUpperScale() || max(r.SizeX(), r.SizeY()) > GetEpsilonForLevel(level));
  }
}
