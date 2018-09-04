#include "routing/speed_camera.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/string_utils.hpp"
#include "base/math.hpp"

#include <limits>

namespace
{
double constexpr kCameraCheckRadiusMeters = 2.0;
double constexpr kCoordinateEqualityDelta = 0.000001;
}  // namespace

namespace routing
{
uint8_t constexpr kNoSpeedCamera = std::numeric_limits<uint8_t>::max();

uint8_t ReadCameraRestriction(FeatureType & ft)
{
  // TODO (@gmoryes) remove this file and .hpp too
  return 0;
}

uint8_t CheckCameraInPoint(m2::PointD const & point, DataSource const & dataSource)
{
  uint8_t speedLimit = kNoSpeedCamera;

  auto const f = [&point, &speedLimit](FeatureType & ft) {
    if (ft.GetFeatureType() != feature::GEOM_POINT)
      return;

    feature::TypesHolder hl(ft);
    if (!ftypes::IsSpeedCamChecker::Instance()(hl))
      return;

    if (my::AlmostEqualAbs(ft.GetCenter().x, point.x, kCoordinateEqualityDelta) &&
        my::AlmostEqualAbs(ft.GetCenter().y, point.y, kCoordinateEqualityDelta))
      speedLimit = ReadCameraRestriction(ft);
  };

  dataSource.ForEachInRect(
      f, MercatorBounds::RectByCenterXYAndSizeInMeters(point, kCameraCheckRadiusMeters),
      scales::GetUpperScale());
  return speedLimit;
}
}  // namespace routing
