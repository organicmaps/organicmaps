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

#include "std/limits.hpp"

namespace
{
double constexpr kCameraCheckRadiusMeters = 2.0;
double constexpr kCoordinateEqualityDelta = 0.000001;
}  // namespace

namespace routing
{
uint8_t const kNoSpeedCamera = numeric_limits<uint8_t>::max();

uint8_t ReadCameraRestriction(FeatureType & ft)
{
  using feature::Metadata;
  feature::Metadata const & md = ft.GetMetadata();
  string const & speed = md.Get(Metadata::FMD_MAXSPEED);
  if (speed.empty())
    return 0;
  int result;
  if (strings::to_int(speed, result))
    return result;
  return 0;
}

uint8_t CheckCameraInPoint(m2::PointD const & point, DataSourceBase const & index)
{
  uint32_t speedLimit = kNoSpeedCamera;

  auto const f = [&point, &speedLimit](FeatureType & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_POINT)
      return;

    feature::TypesHolder hl = ft;
    if (!ftypes::IsSpeedCamChecker::Instance()(hl))
      return;

    if (my::AlmostEqualAbs(ft.GetCenter().x, point.x, kCoordinateEqualityDelta) &&
        my::AlmostEqualAbs(ft.GetCenter().y, point.y, kCoordinateEqualityDelta))
      speedLimit = ReadCameraRestriction(ft);
  };

  index.ForEachInRect(f,
                      MercatorBounds::RectByCenterXYAndSizeInMeters(point,
                                                                    kCameraCheckRadiusMeters),
                      scales::GetUpperScale());
  return speedLimit;
}
}  // namespace routing
