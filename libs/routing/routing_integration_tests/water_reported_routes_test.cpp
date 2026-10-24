#include "testing/testing.hpp"

#include "routing/index_router.hpp"
#include "routing/route.hpp"
#include "routing/water_geometry_provider.hpp"
#include "routing/water_router.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"
#include "traffic/traffic_cache.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <string>
#include <vector>

namespace water_reported_routes_test
{
// Run with ordinary downloaded MWMs in --data_path; no generated water section is required.
void CheckRoute(std::string const & name, ms::LatLon const & start, ms::LatLon const & finish, bool myPosition,
                std::vector<std::string> const & countries)
{
  classificator::Load();
  FrozenDataSource data;
  auto ids = std::make_shared<routing::NumMwmIds>();
  for (auto const & country : countries)
  {
    auto const file =
        platform::LocalCountryFile::MakeTemporary(base::JoinPath(GetPlatform().WritableDir(), country + ".mwm"));
    TEST(Platform::IsFileExistsByFullPath(file.GetPath(MapFileType::Map)), (country));
    auto const [id, status] = data.RegisterMap(file);
    TEST(status == MwmSet::RegResult::Success, (country));
    ids->RegisterFile(file.GetCountryFile());
  }
  auto coverage = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  auto info = storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform());
  LOG(LINFO,
      (name, "region candidates",
       info->GetRegionsCountryIdByRect(m2::RectD(mercator::FromLatLon(start), mercator::FromLatLon(finish)), false)));
  routing::WaterGeometryProvider provider(data, [&](auto const & country, auto const & bounds) {
    return coverage->IsRectCoveredByRegion(country, bounds);
  }, [&](auto const & names, auto const & bounds) { return coverage->IsRectCoveredByRegions(names, bounds); });
  traffic::TrafficCache traffic;
  auto pedestrian =
      std::make_unique<routing::IndexRouter>(routing::VehicleType::Pedestrian, false, [](std::string const & country)
  { return country.substr(0, country.find('_')); }, [&](auto const & point) { return info->GetRegionCountryId(point); },
                                             [&](auto const & country) { return info->GetLimitRectForLeaf(country); },
                                             ids, routing::MakeNumMwmTree(*ids, *info), traffic, data);
  auto const geometry = [&](auto const & checkpoints, auto const & delegate, double margin)
  {
    auto result = provider.GetGeometry(checkpoints, delegate, margin);
    LOG(LINFO, (name, "geometry", margin, result.m_complete, result.m_bounds, result.m_triangles.size()));
    return result;
  };
  routing::WaterRouter router([&](auto const & checkpoints, auto const & delegate)
  { return geometry(checkpoints, delegate, 0); }, std::move(pedestrian),
                              [&](auto const & checkpoints, auto const & delegate)
  { return geometry(checkpoints, delegate, 2000); });
  routing::Checkpoints checkpoints(mercator::FromLatLon(start), mercator::FromLatLon(finish));
  checkpoints.SetStartIsMyPosition(myPosition);
  routing::RouterDelegate delegate;
  int lastProgress = -1;
  delegate.SetProgressCallback([&](float value)
  {
    int const progress = static_cast<int>(value) / 10;
    if (progress != lastProgress)
    {
      lastProgress = progress;
      LOG(LINFO, (name, "progress", value));
    }
  });
  routing::RoutesResult result;
  auto const code = router.CalculateRoute(checkpoints, {}, false, false, delegate, result);
  LOG(LINFO, (name, "result", static_cast<int>(code), result.IsValid()));
  TEST(code == routing::RouterResultCode::NoError, (name));
  TEST(result.IsValid(), (name));
  routing::Route const route(result.GetActive());
  TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(route.GetPoly().GetPoints().front(), checkpoints.GetStart()), 0.1,
                     (name));
  TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(route.GetPoly().GetPoints().back(), checkpoints.GetFinish()), 0.1,
                     (name));
  auto const distances = route.GetWaterDistances();
  TEST(distances.has_value(), (name));
  TEST_GREATER(distances->m_waterMeters, 0.0, (name));
  LOG(LINFO, (name, "distances", distances->m_waterMeters, distances->m_landMeters, distances->m_shortestConfirmed));
}

void CheckAlmere(std::string const & name, ms::LatLon const & finish)
{
  CheckRoute(name, {52.35483470, 5.17963790}, finish, true,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}

UNIT_TEST(WaterReported_AlmereSouthGooimeer)
{
  CheckAlmere("SouthGooimeer", {52.31592888, 5.18930914});
}
UNIT_TEST(WaterReported_AlmereNorthFar)
{
  CheckAlmere("NorthFar", {52.42719742, 5.21305839});
}
UNIT_TEST(WaterReported_AlmereNorthNear)
{
  CheckAlmere("NorthNear", {52.40677214, 5.21760280});
}
UNIT_TEST(WaterReported_AlmereNorthMiddle)
{
  CheckAlmere("NorthMiddle", {52.41585159, 5.23250400});
}
UNIT_TEST(WaterReported_AlmereEast)
{
  CheckAlmere("East", {52.38056521, 5.26966545});
}
UNIT_TEST(WaterReported_Roma)
{
  CheckRoute("Roma", {52.354819, 5.179565}, {52.328613, 5.232172}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam"});
}
UNIT_TEST(WaterReported_Arthur)
{
  CheckRoute("Arthur", {54.614027, 18.226024}, {54.606715, 18.354427}, false, {"Poland_Pomeranian Voivodeship"});
}
UNIT_TEST(WaterReported_LandToLand)
{
  CheckRoute("LandToLand", {52.354819, 5.179565}, {52.355862, 5.185028}, false, {"Netherlands_Flevoland"});
}
UNIT_TEST(WaterReported_RomaSouthLand)
{
  CheckRoute("RomaSouthLand", {52.354835, 5.17963}, {52.328882, 5.202655}, false, {"Netherlands_Flevoland"});
}
UNIT_TEST(WaterReported_PhoneManualFailure)
{
  CheckRoute("PhoneManualFailure", {52.3549188, 5.1797099}, {52.3306528, 5.1975481}, false, {"Netherlands_Flevoland"});
}
UNIT_TEST(WaterReported_PolandSouth)
{
  CheckRoute("PolandSouth", {54.09895, 17.784224}, {54.093102, 17.785852}, false, {"Poland_Pomeranian Voivodeship"});
}
UNIT_TEST(WaterReported_AlmereNorthSouth)
{
  CheckRoute("AlmereNorthSouth", {52.375208, 5.243226}, {52.323037, 5.229683}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam"});
}
UNIT_TEST(WaterReported_DebugCopySouth)
{
  CheckRoute("DebugCopySouth", {52.3548453, 5.1796368}, {52.32768966, 5.18361091}, true,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam"});
}
}  // namespace water_reported_routes_test
