#pragma once

#include "routing/routes_builder/data_source_storage.hpp"

#include "routing/checkpoints.hpp"
#include "routing/index_router.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "traffic/traffic_cache.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/country_parent_getter.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "routing/base/followed_polyline.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"

#include "geometry/latlon.hpp"

#include "base/macros.hpp"
#include "base/thread_pool_computational.hpp"

#include <cstddef>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace routing
{
namespace routes_builder
{
// TODO (@gmoryes)
//  Reuse this class for routing_integration_tests
class RoutesBuilder
{
public:
  explicit RoutesBuilder(size_t threadsNumber);
  DISALLOW_COPY(RoutesBuilder);

  static RoutesBuilder & GetSimpleRoutesBuilder();

  struct Params
  {
    static void Dump(Params const & params, FileWriter & writer);
    static Params Load(ReaderSource<FileReader> & src);

    Params() = default;
    Params(VehicleType type, ms::LatLon const & start, ms::LatLon const & finish);
    Params(VehicleType type, std::vector<m2::PointD> && checkpoints);

    VehicleType m_type = VehicleType::Car;
    Checkpoints m_checkpoints;
    uint32_t m_timeoutSeconds = RouterDelegate::kNoTimeout;
    uint32_t m_launchesNumber = 1;
  };

  struct Route
  {
    static void Dump(Route const & route, FileWriter & writer);
    static Route Load(ReaderSource<FileReader> & src);

    std::vector<ms::LatLon> GetWaypoints() const;
    double GetETA() const { return m_eta; }

    double m_eta = 0.0;
    double m_distance = 0.0;
    FollowedPolyline m_followedPolyline;
  };

  struct Result
  {
    static std::string const kDumpExtension;
    static void Dump(Result const & result, std::string const & filePath);
    static Result Load(std::string const & filePath);

    VehicleType GetVehicleType() const { return m_params.m_type; }
    m2::PointD const & GetStartPoint() const { return m_params.m_checkpoints.GetPointFrom(); }
    m2::PointD const & GetFinishPoint() const { return m_params.m_checkpoints.GetPointTo(); }
    bool IsCodeOK() const { return m_code == RouterResultCode::NoError; }
    std::vector<Route> const & GetRoutes() const { return m_routes; }

    RouterResultCode m_code = RouterResultCode::RouteNotFound;
    Params m_params;
    std::vector<Route> m_routes;
    double m_buildTimeSeconds = 0.0;
  };

  Result ProcessTask(Params const & params);
  std::future<Result> ProcessTaskAsync(Params const & params);

private:
  class Processor
  {
  public:
    Processor(std::shared_ptr<NumMwmIds> numMwmIds, DataSourceStorage & dataSourceStorage,
              std::weak_ptr<storage::CountryParentGetter> cpg, std::weak_ptr<storage::CountryInfoGetter> cig);

    Processor(Processor && rhs) noexcept;

    Result operator()(Params const & params);

  private:
    void InitRouter(VehicleType type);

    ms::LatLon m_start;
    ms::LatLon m_finish;

    std::unique_ptr<IndexRouter> m_router;
    std::shared_ptr<RouterDelegate> m_delegate = std::make_shared<RouterDelegate>();

    std::shared_ptr<NumMwmIds> m_numMwmIds;
    std::shared_ptr<traffic::TrafficCache> m_trafficCache = std::make_shared<traffic::TrafficCache>();
    DataSourceStorage & m_dataSourceStorage;
    std::weak_ptr<storage::CountryParentGetter> m_cpg;
    std::weak_ptr<storage::CountryInfoGetter> m_cig;
    std::unique_ptr<FrozenDataSource> m_dataSource;
  };

  base::ComputationalThreadPool m_threadPool;

  std::shared_ptr<storage::CountryParentGetter> m_cpg = std::make_shared<storage::CountryParentGetter>();

  std::shared_ptr<storage::CountryInfoGetter> m_cig =
      storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform());

  std::shared_ptr<NumMwmIds> m_numMwmIds = std::make_shared<NumMwmIds>();

  DataSourceStorage m_dataSourcesStorage;
};
}  // namespace routes_builder
}  // namespace routing
