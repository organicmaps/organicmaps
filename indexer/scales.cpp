#include "indexer/scales.hpp"
#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <algorithm>

using namespace std;

namespace scales
{
  static const int INITIAL_LEVEL = 1;

  int GetMinAllowableIn3dScale()
  {
    return min(16, min(GetNavigation3dScale(), GetPedestrianNavigation3dScale()));
  }

  double GetScaleLevelD(double ratio)
  {
    double const level = min(static_cast<double>(GetUpperScale()), log(ratio) / log(2.0) + INITIAL_LEVEL);
    return (level < 0.0 ? 0.0 : level);
  }

  double GetScaleLevelD(m2::RectD const & r)
  {
    // TODO: fix scale factors for mercator projection
    double const dx = MercatorBounds::kRangeX / r.SizeX();
    double const dy = MercatorBounds::kRangeY / r.SizeY();

    // get the average ratio
    return GetScaleLevelD((dx + dy) / 2.0);
  }

  int GetScaleLevel(double ratio)
  {
    return base::rounds(GetScaleLevelD(ratio));
  }

  int GetScaleLevel(m2::RectD const & r)
  {
    return base::rounds(GetScaleLevelD(r));
  }

  double GetRationForLevel(double level)
  {
    if (level < INITIAL_LEVEL)
      level = INITIAL_LEVEL;
    return pow(2.0, level - INITIAL_LEVEL);
  }

  m2::RectD GetRectForLevel(double level, m2::PointD const & center)
  {
    double const dy = GetRationForLevel(level);
    double const dx = dy;
    ASSERT_GREATER ( dy, 0.0, () );
    ASSERT_GREATER ( dx, 0.0, () );

    double const xL = MercatorBounds::kRangeX / (2.0 * dx);
    double const yL = MercatorBounds::kRangeY / (2.0 * dy);
    ASSERT_GREATER(xL, 0.0, ());
    ASSERT_GREATER(yL, 0.0, ());

    return m2::RectD(MercatorBounds::ClampX(center.x - xL),
                     MercatorBounds::ClampY(center.y - yL),
                     MercatorBounds::ClampX(center.x + xL),
                     MercatorBounds::ClampY(center.y + yL));
  }

  namespace
  {
    double GetEpsilonImpl(long level, double pixelTolerance)
    {
      return MercatorBounds::kRangeX * pixelTolerance / double(256L << level);
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
    // assume that feature is always visible in upper scale
    return (level == GetUpperScale() || max(r.SizeX(), r.SizeY()) > GetEpsilonForLevel(level));
  }
}
