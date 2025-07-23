#include "indexer/data_source_helpers.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/scales.hpp"

namespace indexer
{
using namespace std;

void ForEachFeatureAtPoint(DataSource const & dataSource, function<void(FeatureType &)> && fn,
                           m2::PointD const & mercator, double toleranceInMeters)
{
  double constexpr kSelectRectWidthInMeters = 1.1;
  double constexpr kMetersToLinearFeature = 3;
  int constexpr kScale = scales::GetUpperScale();
  m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(mercator, kSelectRectWidthInMeters);

  auto const emitter = [&fn, &rect, &mercator, toleranceInMeters](FeatureType & ft)
  {
    switch (ft.GetGeomType())
    {
    case feature::GeomType::Point:
      if (rect.IsPointInside(ft.GetCenter()))
        fn(ft);
      break;
    case feature::GeomType::Line:
      if (feature::GetMinDistanceMeters(ft, mercator) < kMetersToLinearFeature)
        fn(ft);
      break;
    case feature::GeomType::Area:
    {
      auto limitRect = ft.GetLimitRect(kScale);
      // Be a little more tolerant. When used by editor mercator is given
      // with some error, so we must extend limit rect a bit.
      limitRect.Inflate(kMwmPointAccuracy, kMwmPointAccuracy);
      if (limitRect.IsPointInside(mercator) && feature::GetMinDistanceMeters(ft, mercator) <= toleranceInMeters)
        fn(ft);
    }
    break;
    case feature::GeomType::Undefined: ASSERT(false, ("case feature::GeomType::Undefined")); break;
    }
  };

  dataSource.ForEachInRect(emitter, rect, kScale);
}
}  // namespace indexer
