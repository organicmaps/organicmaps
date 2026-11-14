#include "testing/testing.hpp"

#include "routing/absent_regions_finder.hpp"
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
#include "geometry/rect_intersect.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace water_reported_routes_test
{
// Run with ordinary downloaded MWMs in --data_path; no generated water section is required.
std::vector<m2::PointD> CheckRoute(std::string const & name, ms::LatLon const & start, ms::LatLon const & finish,
                                   bool myPosition, std::vector<std::string> const & countries, bool waterOnly = false,
                                   std::vector<ms::LatLon> const & stops = {}, bool cacheWalkingGraph = true,
                                   time_t routingTime = 0, double maxWalking = std::numeric_limits<double>::max(),
                                   double minWater = 0, double maxWater = std::numeric_limits<double>::max())
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
  {
    auto const separator = country.find('_');
    return separator == std::string::npos ? std::string{} : country.substr(0, separator);
  }, [&](auto const & point) { return info->GetRegionCountryId(point); }, [&](auto const & country) {
    return info->GetLimitRectForLeaf(country);
  }, ids, routing::MakeNumMwmTree(*ids, *info), traffic, data, cacheWalkingGraph);
  pedestrian->SetAvoidRoads(routing::RoutingOptions::Ferry);
  auto const savedOptions = routing::RoutingOptions::LoadFromSettings(routing::VehicleType::Pedestrian).GetOptions();
  if (routingTime != 0)
    pedestrian->SetCurrentTimeGetter([routingTime] { return routingTime; });
  auto const geometry = [&](auto const & checkpoints, auto const & delegate, double margin)
  {
    auto result = provider.GetGeometry(checkpoints, delegate, margin);
    LOG(LINFO, (name, "geometry", margin, result.m_complete, result.m_bounds, result.m_triangles.size()));
    return result;
  };
  routing::WaterRouter router(
      [&](auto const & checkpoints, auto const & delegate) { return geometry(checkpoints, delegate, 0); },
      std::move(pedestrian),
      [&](auto const & checkpoints, auto const & delegate, auto const & previous, auto const & frontiers, bool adaptive)
  {
    auto result = provider.ExpandGeometry(checkpoints, delegate, previous, frontiers, adaptive);
    LOG(LINFO, (name, "expanded geometry", result.m_complete, result.m_bounds, result.m_triangles.size()));
    return result;
  });
  routing::CheckpointsGeometry points{mercator::FromLatLon(start)};
  for (auto const & stop : stops)
    points.push_back(mercator::FromLatLon(stop));
  points.push_back(mercator::FromLatLon(finish));
  routing::Checkpoints checkpoints(std::move(points));
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
  TEST_EQUAL(routing::RoutingOptions::LoadFromSettings(routing::VehicleType::Pedestrian).GetOptions(), savedOptions,
             ());
  LOG(LINFO, (name, "result", static_cast<int>(code), result.IsValid()));
  TEST(code == routing::RouterResultCode::NoError, (name));
  TEST(result.IsValid(), (name));
  routing::Route const route(result.GetActive());
  TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(route.GetPoly().GetPoints().front(), checkpoints.GetStart()), 0.1,
                     (name));
  TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(route.GetPoly().GetPoints().back(), checkpoints.GetFinish()), 0.1,
                     (name));
  TEST_EQUAL(route.GetSubrouteCount(), stops.size() + 1, (name));
  for (size_t i = 0; i < route.GetSubrouteCount(); ++i)
  {
    auto const & subroute = route.GetSubrouteAttrs(i);
    TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(subroute.GetStart().GetPoint(), checkpoints.GetPoints()[i]), 0.1,
                       (name, i));
    TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(subroute.GetFinish().GetPoint(), checkpoints.GetPoints()[i + 1]), 0.1,
                       (name, i));
  }
  auto const distances = route.GetWaterDistances();
  TEST(distances.has_value(), (name));
  LOG(LINFO, (name, "distances", distances->m_waterMeters, distances->m_landMeters, distances->m_shortestConfirmed));
  TEST_LESS_OR_EQUAL(distances->m_landMeters, maxWalking, (name));
  TEST_GREATER_OR_EQUAL(distances->m_waterMeters, minWater, (name));
  TEST_GREATER(distances->m_waterMeters, 0.0, (name));
  TEST_LESS_OR_EQUAL(distances->m_waterMeters, maxWater, (name));
  if (waterOnly)
    TEST_EQUAL(distances->m_landMeters, 0.0, (name));
  return route.GetPoly().GetPoints();
}

void CheckAlmere(std::string const & name, ms::LatLon const & finish)
{
  CheckRoute(name, {52.35483470, 5.17963790}, finish, true,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}

UNIT_TEST(WaterReported_NerisNemencia)
{
  // PR #13968: ordinary-map river fragments meet at 54.8530534438, 25.4651406815.
  // Missing connectivity caused a 920 m walking detour instead of following Nemenčia.
  CheckRoute("NerisNemencia", {54.85352075179435, 25.45188769698143}, {54.856528849207365, 25.472672134637833}, false,
             {"Lithuania_East"}, true);
}

void TestArroyoDeRocha()
{
  auto const points =
      CheckRoute("ArroyoDeRocha", {-34.513887, -54.348557}, {-34.521986, -54.346818}, false, {"Uruguay"}, true);
  auto const bridge = mercator::RectByCenterXYAndSizeInMeters(mercator::FromLatLon(-34.5163784, -54.3506677), 15.0);
  auto const crossing = std::adjacent_find(
      points.begin(), points.end(), [&](m2::PointD from, m2::PointD to) { return m2::Intersect(bridge, from, to); });
  TEST(crossing != points.end(), ("The water route must pass under the railway bridge"));
}

// Run with OMIM_TEST_ARROYO_DE_ROCHA=1, --filter=WaterManual_ArroyoDeRocha and Uruguay.mwm in --writable_path.
// Keep manual until the test maps include the change that removed layer=-1 under the railway bridge.
// https://github.com/organicmaps/organicmaps/pull/13968#issuecomment-6873735555
// https://www.openstreetmap.org/changeset/191925170
static auto const g_arroyoDeRochaTest =
    std::getenv("OMIM_TEST_ARROYO_DE_ROCHA")
        ? std::optional<TestRegister>(std::in_place, "WaterManual_ArroyoDeRocha", __FILE__, &TestArroyoDeRocha)
        : std::nullopt;

UNIT_TEST(WaterReported_LithuaniaBalticSea)
{
  CheckRoute("LithuaniaBalticSea", {55.58482353, 20.96625629}, {55.81964426, 20.96674091}, false, {"Lithuania_West"},
             true);
}

UNIT_TEST(WaterReported_LithuaniaBalticSeaReverse)
{
  CheckRoute("LithuaniaBalticSeaReverse", {55.81964426, 20.96674091}, {55.58482353, 20.96625629}, false,
             {"Lithuania_West"}, true);
}

UNIT_TEST(WaterReported_NerisBorderStart)
{
  CheckRoute("NerisBorderStart", {54.82479565, 25.74941456}, {55.07294426, 24.29086921}, false, {"Lithuania_East"},
             true);
}

UNIT_TEST(WaterReported_NemunasDeltaNoFerry)
{
  // PR #13968: two ferries via Nida were counted as 35.68 km of walking.
  // The stored lagoon surface permits a wholly water route instead.
  CheckRoute("NemunasDeltaNoFerry", {55.316761, 21.385615}, {55.336162, 21.219596}, false, {"Lithuania_West"}, true);
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
UNIT_TEST(WaterReported_AlmereLelystad)
{
  CheckRoute("AlmereLelystad", {52.35482820, 5.17957070}, {52.45889844, 5.41338676}, true,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_NorthNearKeersluisplas)
{
  CheckRoute("NorthNearKeersluisplas", {52.40677214, 5.2176028}, {52.45889844, 5.41338676}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_NorthNearKeersluisplasReverse)
{
  CheckRoute("NorthNearKeersluisplasReverse", {52.45889844, 5.41338676}, {52.40677214, 5.2176028}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_NorthMiddleKeersluisplas)
{
  CheckRoute("NorthMiddleKeersluisplas", {52.41585159, 5.232504}, {52.45889844, 5.41338676}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_NorthMiddleKeersluisplasReverse)
{
  CheckRoute("NorthMiddleKeersluisplasReverse", {52.45889844, 5.41338676}, {52.41585159, 5.232504}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_GooimeerKeersluisplas)
{
  CheckRoute("GooimeerKeersluisplas", {52.31592888, 5.18930914}, {52.45889844, 5.41338676}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_GooimeerKeersluisplasReverse)
{
  CheckRoute("GooimeerKeersluisplasReverse", {52.45889844, 5.41338676}, {52.31592888, 5.18930914}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_NorthFarKeersluisplas)
{
  CheckRoute("NorthFarKeersluisplas", {52.42719742, 5.21305839}, {52.45889844, 5.41338676}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_NorthFarKeersluisplasReverse)
{
  CheckRoute("NorthFarKeersluisplasReverse", {52.45889844, 5.41338676}, {52.42719742, 5.21305839}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_EastGooimeer)
{
  CheckRoute("EastGooimeer", {52.38056521, 5.26966545}, {52.31592888, 5.18930914}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_EastGooimeerReverse)
{
  CheckRoute("EastGooimeerReverse", {52.31592888, 5.18930914}, {52.38056521, 5.26966545}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"});
}
UNIT_TEST(WaterLong_WaterwayVistula)
{
  CheckRoute("WaterwayVistula", {53.9749579, 18.8647097}, {54.198981, 18.8999379}, false,
             {"Poland_Pomeranian Voivodeship"}, true);
}
UNIT_TEST(WaterLong_WaterwayVistulaReverse)
{
  CheckRoute("WaterwayVistulaReverse", {54.198981, 18.8999379}, {53.9749579, 18.8647097}, false,
             {"Poland_Pomeranian Voivodeship"}, true);
}
UNIT_TEST(WaterLong_WaterwaySlupia)
{
  CheckRoute("WaterwaySlupia", {54.3583798, 17.1117788}, {54.5063062, 17.0072827}, false,
             {"Poland_Pomeranian Voivodeship"}, true);
}
UNIT_TEST(WaterLong_WaterwaySlupiaReverse)
{
  CheckRoute("WaterwaySlupiaReverse", {54.5063062, 17.0072827}, {54.3583798, 17.1117788}, false,
             {"Poland_Pomeranian Voivodeship"}, true);
}
UNIT_TEST(WaterLong_WaterwayWda)
{
  CheckRoute("WaterwayWda", {53.8525277, 18.3609399}, {53.7366259, 18.3953419}, false,
             {"Poland_Pomeranian Voivodeship"}, true);
}
UNIT_TEST(WaterLong_WaterwayWdaReverse)
{
  CheckRoute("WaterwayWdaReverse", {53.7366259, 18.3953419}, {53.8525277, 18.3609399}, false,
             {"Poland_Pomeranian Voivodeship"}, true);
}
UNIT_TEST(WaterLong_WaterwayLupawa)
{
  CheckRoute("WaterwayLupawa", {54.3786698, 17.6154789}, {54.4685703, 17.4065777}, false,
             {"Poland_Pomeranian Voivodeship"}, true);
}
UNIT_TEST(WaterLong_WaterwayLupawaReverse)
{
  CheckRoute("WaterwayLupawaReverse", {54.4685703, 17.4065777}, {54.3786698, 17.6154789}, false,
             {"Poland_Pomeranian Voivodeship"}, true);
}
UNIT_TEST(WaterLong_WaterwayLageVaart)
{
  CheckRoute("WaterwayLageVaart", {52.4135251, 5.36596705}, {52.5227259, 5.68635155}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"}, true);
}
UNIT_TEST(WaterLong_WaterwayLageVaartReverse)
{
  CheckRoute("WaterwayLageVaartReverse", {52.5227259, 5.68635155}, {52.4135251, 5.36596705}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"}, true);
}
UNIT_TEST(WaterLong_WaterwayHogeVaart)
{
  CheckRoute("WaterwayHogeVaart", {52.425761, 5.62879403}, {52.5342217, 5.77420731}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"}, true);
}
UNIT_TEST(WaterLong_WaterwayHogeVaartReverse)
{
  CheckRoute("WaterwayHogeVaartReverse", {52.5342217, 5.77420731}, {52.425761, 5.62879403}, false,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"}, true);
}
UNIT_TEST(WaterLong_CheckpointsChain)
{
  CheckRoute("CheckpointsChain", {52.35482820, 5.17957070}, {52.31592888, 5.18930914}, true,
             {"Netherlands_Flevoland", "Netherlands_North Holland_Amsterdam", "Netherlands_Utrecht_Amersfoort"}, false,
             {{52.40677214, 5.21760280}, {52.45889844, 5.41338676}});
}
// Reported on Android 16, map catalog 2026-07-14, both maps downloaded, no intermediate points.
UNIT_TEST(WaterReported_KuyavianPomeranianLong)
{
  CheckRoute("KuyavianPomeranianLong", {53.09396078, 18.16890286}, {54.30863264, 18.93380513}, false,
             {"Poland_Kuyavian-Pomeranian Voivodeship", "Poland_Pomeranian Voivodeship"}, true);
}

// Reported with code 6 on Android 16, catalog 2026-07-14, both maps downloaded.
UNIT_TEST(WaterReported_VistulaMouth)
{
  CheckRoute("VistulaMouth", {53.09858890, 18.15192177}, {54.33892355, 18.94185916}, false,
             {"Poland_Kuyavian-Pomeranian Voivodeship", "Poland_Pomeranian Voivodeship"}, true);
}

UNIT_TEST(WaterLong_VistulaMouthReverse)
{
  CheckRoute("VistulaMouthReverse", {54.33892355, 18.94185916}, {53.09858890, 18.15192177}, false,
             {"Poland_Kuyavian-Pomeranian Voivodeship", "Poland_Pomeranian Voivodeship"}, true);
}

UNIT_TEST(WaterLong_KuyavianPomeranianReverse)
{
  CheckRoute("KuyavianPomeranianReverse", {54.30863264, 18.93380513}, {53.09396078, 18.16890286}, false,
             {"Poland_Kuyavian-Pomeranian Voivodeship", "Poland_Pomeranian Voivodeship"}, true);
}

UNIT_TEST(WaterReported_PomeranianLongPortage)
{
  // This reported-route regression also verifies that graph reuse preserves the entire path.
  auto const time = std::time(nullptr);
  auto const fresh = CheckRoute("PomeranianFreshGraph", {54.12300877, 17.75593190}, {53.84449881, 18.35696959}, false,
                                {"Poland_Pomeranian Voivodeship"}, false, {}, false, time);
  auto const cached = CheckRoute("PomeranianCachedGraph", {54.12300877, 17.75593190}, {53.84449881, 18.35696959}, false,
                                 {"Poland_Pomeranian Voivodeship"}, false, {}, true, time);
  TEST_EQUAL(cached, fresh, ());
}
UNIT_TEST(WaterLong_PomeranianPortageReverse)
{
  CheckRoute("PomeranianPortageReverse", {53.84449881, 18.35696959}, {54.12300877, 17.75593190}, false,
             {"Poland_Pomeranian Voivodeship"});
}
UNIT_TEST(WaterReported_NerisKaunasReservoir)
{
  CheckRoute("NerisKaunasReservoir", {54.82539913135153, 25.748180598020554}, {54.900349439028986, 24.087091237306595},
             false, {"Lithuania_East", "Lithuania_West"}, false, {}, true, 0, 5000.0, 200000.0);
}

UNIT_TEST(WaterLong_NerisKaunasReservoirReverse)
{
  CheckRoute("NerisKaunasReservoirReverse", {54.900349439028986, 24.087091237306595},
             {54.82539913135153, 25.748180598020554}, false, {"Lithuania_East", "Lithuania_West"}, false, {}, true, 0,
             5000.0, 200000.0);
}

UNIT_TEST(WaterLong_NerisConfluence)
{
  CheckRoute("NerisConfluence", {54.82539913135153, 25.748180598020554}, {54.9005461209, 23.8703662445}, false,
             {"Lithuania_East", "Lithuania_West"}, true);
}
}  // namespace water_reported_routes_test

UNIT_TEST(WaterReported_MissingLatvia)
{
  using namespace routing;
  auto info = storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform());
  AbsentRegionsFinder finder([&](auto const & point) { return info->GetRegionCountryId(point); },
                             [](auto const & country) { return country == "Lithuania_East"; });
  Checkpoints checkpoints(mercator::FromLatLon(54.71577130, 25.27821850),
                          mercator::FromLatLon(57.24266941, 24.16273648));
  checkpoints.SetStartIsMyPosition(true);
  RouterDelegate delegate;
  TEST(finder.GenerateAbsentRegions(checkpoints, delegate) == RouterResultCode::NeedMoreMaps, ());
  TEST_EQUAL(finder.GetAbsentRegions(), (std::set<std::string>{"Latvia"}), ());
}

UNIT_TEST(WaterReported_NerisSlowRoute)
{
  water_reported_routes_test::CheckRoute(
      "NerisSlowRoute", {54.82494, 25.749111}, {54.880858, 24.040882}, false,
      {"Lithuania_East", "Lithuania_West", "Belarus_Hrodna Region", "Belarus_Minsk Region", "Belarus_Vitebsk Region"});
}

UNIT_TEST(WaterReported_LithuaniaLatviaSea)
{
  water_reported_routes_test::CheckRoute("LithuaniaLatviaSea", {55.31438121, 19.60647214}, {57.56200190, 23.84290724},
                                         false, {"Lithuania_West", "Latvia"}, true, {}, true, 0, 0.0, 0.0, 529143.0);
}

UNIT_TEST(WaterReported_LithuaniaLatviaSeaReverse)
{
  water_reported_routes_test::CheckRoute("LithuaniaLatviaSeaReverse", {57.56200190, 23.84290724},
                                         {55.31438121, 19.60647214}, false, {"Lithuania_West", "Latvia"}, true, {},
                                         true, 0, 0.0, 0.0, 529143.0);
}

UNIT_TEST(WaterReported_LithuaniaLatviaSeaWithNeighboringMaps)
{
  water_reported_routes_test::CheckRoute("LithuaniaLatviaSeaWithNeighboringMaps", {55.31438121, 19.60647214},
                                         {57.56200190, 23.84290724}, false,
                                         {"Lithuania_East", "Lithuania_West", "Latvia", "Belarus_Hrodna Region",
                                          "Belarus_Minsk Region", "Belarus_Vitebsk Region"},
                                         true, {}, true, 0, 0.0, 0.0, 529143.0);
}

UNIT_TEST(WaterReported_LatviaOpenSea)
{
  water_reported_routes_test::CheckRoute("LatviaOpenSea", {55.72627956, 20.24194466}, {57.28488619, 23.71587488}, false,
                                         {"Lithuania_West", "Latvia"}, true);
}

UNIT_TEST(WaterReported_LatviaOpenSeaReverse)
{
  water_reported_routes_test::CheckRoute("LatviaOpenSeaReverse", {57.28488619, 23.71587488}, {55.72627956, 20.24194466},
                                         false, {"Lithuania_West", "Latvia"}, true);
}

UNIT_TEST(WaterReported_NerisNemunas)
{
  // Reported via coordinates describe the incorrect path, not intermediate checkpoints.
  water_reported_routes_test::CheckRoute("NerisNemunas", {54.857431, 25.746642}, {55.043729, 21.101253}, false,
                                         {"Lithuania_East", "Lithuania_West", "Latvia", "Russia_Kaliningrad Oblast",
                                          "Belarus_Hrodna Region", "Belarus_Minsk Region", "Belarus_Vitebsk Region"},
                                         true);
}

UNIT_TEST(WaterReported_NerisNemunasReverse)
{
  water_reported_routes_test::CheckRoute("NerisNemunasReverse", {55.043729, 21.101253}, {54.857431, 25.746642}, false,
                                         {"Lithuania_East", "Lithuania_West", "Latvia", "Russia_Kaliningrad Oblast",
                                          "Belarus_Hrodna Region", "Belarus_Minsk Region", "Belarus_Vitebsk Region"},
                                         true);
}
