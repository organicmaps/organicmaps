#include "indexer/scales.hpp"
#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <algorithm>

namespace scales
{
  static const int INITIAL_LEVEL = 1;

  int GetMinAllowableIn3dScale()
  {
    return std::min(16, std::min(GetNavigation3dScale(), GetPedestrianNavigation3dScale()));
  }

  double GetScaleLevelD(double ratio)
  {
    double const level = std::min(static_cast<double>(GetUpperScale()), std::log2(ratio) + INITIAL_LEVEL);
    return level < 0.0 ? 0.0 : level;
  }

  double GetScaleLevelD(m2::RectD const & r)
  {
    // TODO: fix scale factors for mercator projection
    double const dx = mercator::Bounds::kRangeX / r.SizeX();
    double const dy = mercator::Bounds::kRangeY / r.SizeY();

    // get the average ratio
    return GetScaleLevelD((dx + dy) / 2.0);
  }

  int GetScaleLevel(double ratio)
  {
    return base::SignedRound(GetScaleLevelD(ratio));
  }

  int GetScaleLevel(m2::RectD const & r)
  {
    return base::SignedRound(GetScaleLevelD(r));
  }

  namespace
  {
    double GetEpsilonImpl(long level, double pixelTolerance)
    {
      return mercator::Bounds::kRangeX * pixelTolerance / double(256L << level);
    }
  }

  double GetEpsilonForLevel(int level)
  {
    return GetEpsilonImpl(level, 7);
  }

  double GetEpsilonForSimplify(int level)
  {
    // Keep better geometries on highest zoom to allow scaling them deeper
    if (level == GetUpperScale())
      return GetEpsilonImpl(level, 0.4);
    // Keep crude geometries for all other zooms
    else
      return GetEpsilonImpl(level, 1.3);
  }

  bool IsGoodForLevel(int level, m2::RectD const & r)
  {
    ASSERT(level >= 0 && level <= GetUpperScale(), (level));
    // assume that feature is always visible in upper scale
    return (level == GetUpperScale() || std::max(r.SizeX(), r.SizeY()) > GetEpsilonForLevel(level));
  }
} // namespace scales
