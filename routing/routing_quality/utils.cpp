#include "routing/routing_quality/utils.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "routing/index_router.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/country_parent_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <array>
#include <limits>
#include <memory>
#include <utility>

using namespace std;
using namespace routing;

namespace
{
class RouteGetter
{
public:
  static RouteGetter & Instance()
  {
    static RouteGetter instance;
    return instance;
  }

  routing_quality::RouteResult operator()(routing_quality::RoutePoints && waypoints, VehicleType type)
  {
    CHECK_LESS(type, VehicleType::Count, ());
    auto const & infoGetter = *m_cig.get();

    auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) {
      return infoGetter.GetRegionCountryId(pt);
    };

    auto const getMwmRectByName = [&infoGetter](string const & countryId) {
      return infoGetter.GetLimitRectForLeaf(countryId);
    };

    auto const index = my::Key(type);
    if (!m_routers[index])
    {
      m_routers[index] = make_unique<IndexRouter>(type, false /* load altitudes */, *m_cpg,
                            countryFileGetter, getMwmRectByName, m_numMwmIds,
                            MakeNumMwmTree(*m_numMwmIds, infoGetter), m_trafficCache, m_dataSource);
    }

    routing_quality::RouteResult result;
    result.m_code = m_routers[index]->CalculateRoute(
        Checkpoints(move(waypoints)), m2::PointD::Zero(),
        false /* adjustToPrevRoute */, m_delegate, result.m_route);
    return result;
  }

private:
  RouteGetter()
  {
    CHECK(m_cig, ());
    CHECK(m_cpg, ());

    classificator::Load();
    vector<platform::LocalCountryFile> localFiles;
    platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max(), localFiles);

    for (auto const & localFile : localFiles)
    {
      UNUSED_VALUE(m_dataSource.RegisterMap(localFile));
      auto const & countryFile = localFile.GetCountryFile();
      auto const mwmId = m_dataSource.GetMwmIdByCountryFile(countryFile);
      CHECK(mwmId.IsAlive(), ());
      // We have to exclude minsk-pass because we can't register mwm which is not from
      // countries.txt.
      if (mwmId.GetInfo()->GetType() == MwmInfo::COUNTRY && countryFile.GetName() != "minsk-pass")
        m_numMwmIds->RegisterFile(countryFile);
    }
  }

  DISALLOW_COPY_AND_MOVE(RouteGetter);

  FrozenDataSource m_dataSource;
  shared_ptr<NumMwmIds> m_numMwmIds = make_shared<NumMwmIds>();
  array<unique_ptr<IndexRouter>, my::Key(VehicleType::Count)> m_routers{};
  unique_ptr<storage::CountryParentGetter> m_cpg = make_unique<storage::CountryParentGetter>();
  unique_ptr<storage::CountryInfoGetter> m_cig = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  traffic::TrafficCache m_trafficCache;
  RouterDelegate m_delegate;
};
}  // namespace

namespace routing_quality
{
FollowedPolyline GetRouteFollowedPolyline(RouteParams && params)
{
  CHECK_GREATER_OR_EQUAL(params.m_waypoints.size(), 2, ());
  auto points = FromLatLon(params.m_waypoints);
  auto const result = RouteGetter::Instance()(move(points), params.m_type);
  CHECK_EQUAL(result.m_code, RouterResultCode::NoError, ());
  CHECK(result.m_route.IsValid(), ());
  return result.m_route.GetFollowedPolyline();
}

RouteResult GetRoute(RoutePoints && waypoints, VehicleType type)
{
  CHECK_GREATER_OR_EQUAL(waypoints.size(), 2, ());
  return RouteGetter::Instance()(move(waypoints), type);
}

RoutePoints FromLatLon(Coordinates const & coords)
{
  CHECK(!coords.empty(), ());
  RoutePoints ret;
  ret.reserve(coords.size());
  for (auto const & ll : coords)
    ret.emplace_back(MercatorBounds::FromLatLon(ll));

  return ret;
}
}  // namespace routing_quality
