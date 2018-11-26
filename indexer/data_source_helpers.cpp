#include "indexer/data_source_helpers.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/scales.hpp"

using namespace std;

namespace indexer
{
void ForEachFeatureAtPoint(DataSource const & dataSource, function<void(FeatureType &)> && fn,
                           m2::PointD const & mercator, double toleranceInMeters)
{
  double constexpr kSelectRectWidthInMeters = 1.1;
  double constexpr kMetersToLinearFeature = 3;
  int constexpr kScale = scales::GetUpperScale();
  m2::RectD const rect =
      MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, kSelectRectWidthInMeters);

  auto const emitter = [&fn, &rect, &mercator, toleranceInMeters](FeatureType & ft) {
    switch (ft.GetFeatureType())
    {
    case feature::GEOM_POINT:
      if (rect.IsPointInside(ft.GetCenter()))
        fn(ft);
      break;
    case feature::GEOM_LINE:
      if (feature::GetMinDistanceMeters(ft, mercator) < kMetersToLinearFeature)
        fn(ft);
      break;
    case feature::GEOM_AREA:
    {
      auto limitRect = ft.GetLimitRect(kScale);
      // Be a little more tolerant. When used by editor mercator is given
      // with some error, so we must extend limit rect a bit.
      limitRect.Inflate(kMwmPointAccuracy, kMwmPointAccuracy);
      if (limitRect.IsPointInside(mercator) &&
          feature::GetMinDistanceMeters(ft, mercator) <= toleranceInMeters)
      {
        fn(ft);
      }
    }
    break;
    case feature::GEOM_UNDEFINED: ASSERT(false, ("case feature::GEOM_UNDEFINED")); break;
    }
  };

  dataSource.ForEachInRect(emitter, rect, kScale);
}
}  // namespace indexer
