#include "indexer/data_source_helpers.hpp"

#include "indexer/data_source.hpp"
#include "indexer/feature_algo.hpp"

namespace indexer
{
using namespace std;

void ForEachFeatureAtPoint(DataSource const & dataSource, function<void(FeatureType &)> && fn,
                           m2::PointD const & mercator, int scale /* = scales::GetUpperScale() */)
{
  ASSERT(scale <= scales::GetUpperScale(), (scale));
  int const factor = 1 << (scales::GetUpperScale() - scale);

  double const queryRectWidthM = 1.1 * factor;
  double const distanceToLinearFeatureM = 3 * factor;

  m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(mercator, queryRectWidthM);

  auto const emitter = [&](FeatureType & ft)
  {
    switch (ft.GetGeomType())
    {
    case feature::GeomType::Point:
      if (rect.IsPointInside(ft.GetCenter()))
        fn(ft);
      break;
    case feature::GeomType::Line:
      if (feature::GetMinDistanceMeters(ft, mercator) < distanceToLinearFeatureM)
        fn(ft);
      break;
    case feature::GeomType::Area:
    {
      auto limitRect = ft.GetLimitRect(scale);
      // Be a little more tolerant. When used by editor mercator is given
      // with some error, so we must extend limit rect a bit.
      // kMwmPointAccuracy ~ 1m, we compare a real distance with 0.01m epsilon.
      limitRect.Inflate(kMwmPointAccuracy, kMwmPointAccuracy);
      if (limitRect.IsPointInside(mercator) && feature::GetMinDistanceMeters(ft, mercator) <= 0.01)
        fn(ft);
    }
    break;
    case feature::GeomType::Undefined: ASSERT(false, ("case feature::GeomType::Undefined")); break;
    }
  };

  dataSource.ForEachInRect(emitter, rect, scale);
}
}  // namespace indexer
