#include "speed_camera.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"
#include "coding/read_write_utils.hpp"

#include "base/string_utils.hpp"

namespace
{
double constexpr kMwmReadingRadiusMeters = 2.0;
}  // namespace

namespace routing
{
uint8_t ReadCamRestriction(FeatureType & ft)
{
  using feature::Metadata;
  ft.ParseMetadata();
  feature::Metadata const & md = ft.GetMetadata();
  string const & speed = md.Get(Metadata::FMD_MAXSPEED);
  if (!speed.length())
    return 0;
  int result;
  strings::to_int(speed, result);
  return result;
}

uint8_t CheckCameraInPoint(m2::PointD const & point, Index const & index)
{
  uint32_t speedLimit = kNoSpeedCamera;

  auto const f = [&point, &speedLimit](FeatureType & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_POINT)
      return;

    feature::TypesHolder hl = ft;
    if (!ftypes::IsSpeedCamChecker::Instance()(hl))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    if (ft.GetCenter() == point)
      speedLimit = ReadCamRestriction(ft);
  };

  index.ForEachInRect(f,
                      MercatorBounds::RectByCenterXYAndSizeInMeters(point, kMwmReadingRadiusMeters),
                      scales::GetUpperScale());
  return speedLimit;
}
}  // namespace routing
