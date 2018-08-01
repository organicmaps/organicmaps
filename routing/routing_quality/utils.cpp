#include "routing/routing_quality/utils.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "routing/index_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"

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

#include <limits>
#include <memory>
#include <utility>

using namespace std;

namespace
{
class PolylineGetter
{
public:
  PolylineGetter()
  {
    classificator::Load();
    vector<platform::LocalCountryFile> localFiles;
    platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max(), localFiles);
    m_numMwmIds = make_shared<routing::NumMwmIds>();

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

  routing::FollowedPolyline operator()(routing_quality::RouteParams && params)
  {
    using namespace routing;

    unique_ptr<storage::CountryInfoGetter> cig =
        storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
    CHECK(cig, ());

    auto const & infoGetter = *cig.get();

    auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) {
      return infoGetter.GetRegionCountryId(pt);
    };

    auto const getMwmRectByName = [&infoGetter](string const & countryId) {
      return infoGetter.GetLimitRectForLeaf(countryId);
    };

    auto countryParentGetter = make_unique<storage::CountryParentGetter>();
    CHECK(countryParentGetter, ());
    traffic::TrafficCache cache;
    IndexRouter indexRouter(params.m_type, false /* load altitudes */, *countryParentGetter,
                            countryFileGetter, getMwmRectByName, m_numMwmIds,
                            MakeNumMwmTree(*m_numMwmIds, infoGetter), cache, m_dataSource);

    RouterDelegate delegate;
    Route route("" /* router */, 0 /* routeId */);
    auto const result = indexRouter.CalculateRoute(
        Checkpoints(routing_quality::FromLatLon(params.m_waypoints)), m2::PointD::Zero(),
        false /* adjustToPrevRoute */, delegate, route);

    CHECK_EQUAL(result, RouterResultCode::NoError, ());
    CHECK(route.IsValid(), ());

    return route.GetFollowedPolyline();
  }

private:
  FrozenDataSource m_dataSource;
  shared_ptr<routing::NumMwmIds> m_numMwmIds;
};
}  // namespace

namespace routing_quality
{
routing::FollowedPolyline GetRouteFollowedPolyline(RouteParams && params)
{
  CHECK_GREATER_OR_EQUAL(params.m_waypoints.size(), 2, ());
  PolylineGetter get;
  return get(move(params));
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
