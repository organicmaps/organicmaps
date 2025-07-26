#include "routing/routes_builder/routes_builder.hpp"

#include "routing/vehicle_mask.hpp"

#include "storage/routing_helpers.hpp"

#include "indexer/classificator_loader.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"

#include "coding/write_to_sink.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <limits>

namespace
{
void DumpPointDVector(std::vector<m2::PointD> const & points, FileWriter & writer)
{
  WriteToSink(writer, points.size());
  for (auto const & point : points)
  {
    writer.Write(&point.x, sizeof(point.x));
    writer.Write(&point.y, sizeof(point.y));
  }
}

std::vector<m2::PointD> LoadPointDVector(ReaderSource<FileReader> & src)
{
  std::vector<m2::PointD> points;
  auto const n = ReadPrimitiveFromSource<size_t>(src);
  points.reserve(n);
  for (size_t i = 0; i < n; ++i)
  {
    auto const x = ReadPrimitiveFromSource<double>(src);
    auto const y = ReadPrimitiveFromSource<double>(src);

    points.emplace_back(x, y);
  }

  return points;
}
}  // namespace

namespace routing
{
namespace routes_builder
{
// RoutesBuilder::Params ---------------------------------------------------------------------------

RoutesBuilder::Params::Params(VehicleType type, ms::LatLon const & start, ms::LatLon const & finish)
  : Params(type, {mercator::FromLatLon(start), mercator::FromLatLon(finish)})
{}

RoutesBuilder::Params::Params(VehicleType type, std::vector<m2::PointD> && checkpoints)
  : m_type(type)
  , m_checkpoints(std::move(checkpoints))
{}

// static
void RoutesBuilder::Params::Dump(Params const & params, FileWriter & writer)
{
  WriteToSink(writer, static_cast<int>(params.m_type));
  DumpPointDVector(params.m_checkpoints.GetPoints(), writer);
}

// static
RoutesBuilder::Params RoutesBuilder::Params::Load(ReaderSource<FileReader> & src)
{
  Params params;

  auto const type = ReadPrimitiveFromSource<int>(src);
  CHECK_LESS(type, static_cast<int>(VehicleType::Count), ());

  params.m_type = static_cast<VehicleType>(type);
  params.m_checkpoints = Checkpoints(LoadPointDVector(src));

  return params;
}

// RoutesBuilder -----------------------------------------------------------------------------------

// static
RoutesBuilder & RoutesBuilder::GetSimpleRoutesBuilder()
{
  static RoutesBuilder routesBuilder(1 /* threadsNumber */);
  return routesBuilder;
}
RoutesBuilder::RoutesBuilder(size_t threadsNumber) : m_threadPool(threadsNumber)
{
  CHECK_GREATER(threadsNumber, 0, ());
  LOG(LINFO, ("Threads number:", threadsNumber));
  CHECK(m_cig, ());
  CHECK(m_cpg, ());

  classificator::Load();
  std::vector<platform::LocalCountryFile> localFiles;
  platform::FindAllLocalMapsAndCleanup(std::numeric_limits<int64_t>::max(), localFiles);

  std::vector<std::unique_ptr<FrozenDataSource>> dataSources;
  for (size_t i = 0; i < threadsNumber; ++i)
    dataSources.emplace_back(std::make_unique<FrozenDataSource>());

  for (auto const & localFile : localFiles)
  {
    auto const & countryFile = localFile.GetCountryFile();
    // Only maps from countries.txt should be used.
    if (!m_cpg->GetStorageForTesting().IsLeaf(countryFile.GetName()))
      continue;

    m_numMwmIds->RegisterFile(countryFile);

    for (auto & dataSource : dataSources)
    {
      auto const result = dataSource->RegisterMap(localFile);
      CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register mwm:", localFile));

      auto const mwmId = dataSource->GetMwmIdByCountryFile(countryFile);
      if (!mwmId.IsAlive())
        continue;
    }
  }

  for (auto & dataSource : dataSources)
    m_dataSourcesStorage.PushDataSource(std::move(dataSource));
}

RoutesBuilder::Result RoutesBuilder::ProcessTask(Params const & params)
{
  Processor processor(m_numMwmIds, m_dataSourcesStorage, m_cpg, m_cig);
  return processor(params);
}

std::future<RoutesBuilder::Result> RoutesBuilder::ProcessTaskAsync(Params const & params)
{
  // Should be copyable to workaround MSVC bug (https://developercommunity.visualstudio.com/t/108672)
  auto task = [processor = std::make_shared<Processor>(m_numMwmIds, m_dataSourcesStorage, m_cpg, m_cig)](
                  Params const & params) -> Result { return (*processor)(params); };
  return m_threadPool.Submit(std::move(task), params);
}

// RoutesBuilder::Result ---------------------------------------------------------------------------

// static
std::string const RoutesBuilder::Result::kDumpExtension = ".mapsme.dump";

// static
void RoutesBuilder::Result::Dump(Result const & result, std::string const & filePath)
{
  FileWriter writer(filePath);
  WriteToSink(writer, static_cast<int>(result.m_code));

  writer.Write(&result.m_buildTimeSeconds, sizeof(m_buildTimeSeconds));

  RoutesBuilder::Params::Dump(result.m_params, writer);

  size_t const routesNumber = result.m_routes.size();
  writer.Write(&routesNumber, sizeof(routesNumber));
  for (auto const & route : result.m_routes)
    RoutesBuilder::Route::Dump(route, writer);
}

// static
RoutesBuilder::Result RoutesBuilder::Result::Load(std::string const & filePath)
{
  Result result;
  FileReader reader(filePath);
  ReaderSource<FileReader> src(reader);

  auto const code = ReadPrimitiveFromSource<int>(src);
  result.m_code = static_cast<RouterResultCode>(code);
  result.m_buildTimeSeconds = ReadPrimitiveFromSource<double>(src);
  result.m_params = RoutesBuilder::Params::Load(src);

  auto const routesNumber = ReadPrimitiveFromSource<size_t>(src);
  result.m_routes.resize(routesNumber);
  for (size_t i = 0; i < routesNumber; ++i)
    result.m_routes[i] = RoutesBuilder::Route::Load(src);

  return result;
}

// RoutesBuilder::Route ----------------------------------------------------------------------------

// static
void RoutesBuilder::Route::Dump(Route const & route, FileWriter & writer)
{
  writer.Write(&route.m_eta, sizeof(route.m_eta));
  writer.Write(&route.m_distance, sizeof(route.m_distance));

  DumpPointDVector(route.m_followedPolyline.GetPolyline().GetPoints(), writer);
}

// static
RoutesBuilder::Route RoutesBuilder::Route::Load(ReaderSource<FileReader> & src)
{
  Route route;

  route.m_eta = ReadPrimitiveFromSource<double>(src);
  route.m_distance = ReadPrimitiveFromSource<double>(src);

  std::vector<m2::PointD> points = LoadPointDVector(src);
  if (points.empty())
    return route;

  FollowedPolyline poly(points.begin(), points.end());
  route.m_followedPolyline.Swap(poly);

  return route;
}

std::vector<ms::LatLon> RoutesBuilder::Route::GetWaypoints() const
{
  auto const & points = m_followedPolyline.GetPolyline().GetPoints();
  std::vector<ms::LatLon> latlonPoints;
  latlonPoints.reserve(points.size());
  for (auto const & point : points)
    latlonPoints.emplace_back(mercator::ToLatLon(point));

  return latlonPoints;
}

// RoutesBuilder::Processor ------------------------------------------------------------------------

RoutesBuilder::Processor::Processor(std::shared_ptr<NumMwmIds> numMwmIds, DataSourceStorage & dataSourceStorage,
                                    std::weak_ptr<storage::CountryParentGetter> cpg,
                                    std::weak_ptr<storage::CountryInfoGetter> cig)
  : m_numMwmIds(std::move(numMwmIds))
  , m_dataSourceStorage(dataSourceStorage)
  , m_cpg(std::move(cpg))
  , m_cig(std::move(cig))
{}

RoutesBuilder::Processor::Processor(Processor && rhs) noexcept : m_dataSourceStorage(rhs.m_dataSourceStorage)
{
  m_start = rhs.m_start;
  m_finish = rhs.m_finish;

  m_router = std::move(rhs.m_router);
  m_delegate = std::move(rhs.m_delegate);
  m_numMwmIds = std::move(rhs.m_numMwmIds);
  m_trafficCache = std::move(rhs.m_trafficCache);
  m_cpg = std::move(rhs.m_cpg);
  m_cig = std::move(rhs.m_cig);
  m_dataSource = std::move(rhs.m_dataSource);
}

void RoutesBuilder::Processor::InitRouter(VehicleType type)
{
  if (m_router && m_router->GetVehicleType() == type)
    return;

  auto const & cig = m_cig;
  auto const countryFileGetter = [cig](m2::PointD const & pt)
  {
    auto const cigSharedPtr = cig.lock();
    return cigSharedPtr->GetRegionCountryId(pt);
  };

  auto const getMwmRectByName = [cig](std::string const & countryId)
  {
    auto const cigSharedPtr = cig.lock();
    return cigSharedPtr->GetLimitRectForLeaf(countryId);
  };

  bool const loadAltitudes = type != VehicleType::Car;
  if (!m_dataSource)
    m_dataSource = m_dataSourceStorage.GetDataSource();

  m_router = std::make_unique<IndexRouter>(type, loadAltitudes, *m_cpg.lock(), countryFileGetter, getMwmRectByName,
                                           m_numMwmIds, MakeNumMwmTree(*m_numMwmIds, *m_cig.lock()), *m_trafficCache,
                                           *m_dataSource);
}

RoutesBuilder::Result RoutesBuilder::Processor::operator()(Params const & params)
{
  InitRouter(params.m_type);
  SCOPE_GUARD(returnDataSource, [&]() { m_dataSourceStorage.PushDataSource(std::move(m_dataSource)); });

  LOG(LINFO, ("Start building route, checkpoints:", params.m_checkpoints));

  RouterResultCode resultCode = RouterResultCode::RouteNotFound;
  routing::Route route("" /* router */, 0 /* routeId */);

  CHECK(m_dataSource, ());

  double timeSum = 0.0;
  for (size_t i = 0; i < params.m_launchesNumber; ++i)
  {
    m_delegate->SetTimeout(params.m_timeoutSeconds);
    base::Timer timer;
    resultCode = m_router->CalculateRoute(params.m_checkpoints, m2::PointD::Zero(), false /* adjustToPrevRoute */,
                                          *m_delegate, route);

    if (resultCode != RouterResultCode::NoError)
      break;

    timeSum += timer.ElapsedSeconds();
  }

  Result result;
  result.m_params.m_checkpoints = params.m_checkpoints;
  result.m_code = resultCode;
  result.m_buildTimeSeconds = timeSum / static_cast<double>(params.m_launchesNumber);

  RoutesBuilder::Route routeResult;
  routeResult.m_distance = route.GetTotalDistanceMeters();
  routeResult.m_eta = route.GetTotalTimeSec();

  routeResult.m_followedPolyline = route.GetFollowedPolyline();

  result.m_routes.emplace_back(std::move(routeResult));

  return result;
}
}  // namespace routes_builder
}  // namespace routing
