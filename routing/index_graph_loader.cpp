#include "routing/index_graph_loader.hpp"

#include "routing/city_roads.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/restriction_loader.hpp"
#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"
#include "routing/route.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/speed_camera_ser_des.hpp"

#include "indexer/data_source.hpp"

#include "platform/country_defines.hpp"

#include "coding/files_container.hpp"

#include "base/assert.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <utility>

namespace
{
using namespace routing;
using namespace std;

class IndexGraphLoaderImpl final : public IndexGraphLoader
{
public:
  IndexGraphLoaderImpl(VehicleType vehicleType, bool loadAltitudes, shared_ptr<NumMwmIds> numMwmIds,
                       shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
                       shared_ptr<EdgeEstimator> estimator, DataSource & dataSource,
                       RoutingOptions routingOptions = RoutingOptions());

  // IndexGraphLoader overrides:
  IndexGraph & GetIndexGraph(NumMwmId numMwmId) override;
  vector<RouteSegment::SpeedCamera> GetSpeedCameraInfo(Segment const & segment) override;
  void Clear() override;

private:
  using GraphPtrT = unique_ptr<IndexGraph>;
  GraphPtrT CreateIndexGraph(NumMwmId numMwmId);

  VehicleType m_vehicleType;
  bool m_loadAltitudes;
  DataSource & m_dataSource;
  shared_ptr<NumMwmIds> m_numMwmIds;
  shared_ptr<VehicleModelFactoryInterface> m_vehicleModelFactory;
  shared_ptr<EdgeEstimator> m_estimator;

  unordered_map<NumMwmId, GraphPtrT> m_graphs;

  using CamerasMapT = map<SegmentCoord, vector<RouteSegment::SpeedCamera>>;
  unordered_map<NumMwmId, CamerasMapT> m_cachedCameras;
  CamerasMapT const & ReceiveSpeedCamsFromMwm(NumMwmId numMwmId);

  RoutingOptions m_avoidRoutingOptions;
  std::function<time_t()> m_currentTimeGetter = [time = GetCurrentTimestamp()]() {
    return time;
  };
};

IndexGraphLoaderImpl::IndexGraphLoaderImpl(
    VehicleType vehicleType, bool loadAltitudes, shared_ptr<NumMwmIds> numMwmIds,
    shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
    shared_ptr<EdgeEstimator> estimator, DataSource & dataSource,
    RoutingOptions routingOptions)
  : m_vehicleType(vehicleType)
  , m_loadAltitudes(loadAltitudes)
  , m_dataSource(dataSource)
  , m_numMwmIds(move(numMwmIds))
  , m_vehicleModelFactory(move(vehicleModelFactory))
  , m_estimator(move(estimator))
  , m_avoidRoutingOptions(routingOptions)
{
  CHECK(m_numMwmIds, ());
  CHECK(m_vehicleModelFactory, ());
  CHECK(m_estimator, ());
}

IndexGraph & IndexGraphLoaderImpl::GetIndexGraph(NumMwmId numMwmId)
{
  auto res = m_graphs.try_emplace(numMwmId, GraphPtrT{});
  if (res.second)
    res.first->second = CreateIndexGraph(numMwmId);
  return *(res.first->second);
}

IndexGraphLoaderImpl::CamerasMapT const & IndexGraphLoaderImpl::ReceiveSpeedCamsFromMwm(NumMwmId numMwmId)
{
  auto res = m_cachedCameras.try_emplace(numMwmId, CamerasMapT{});
  if (res.second)
  {
    auto const & file = m_numMwmIds->GetFile(numMwmId);
    auto handle = m_dataSource.GetMwmHandleByCountryFile(file);
    if (!handle.IsAlive())
      MYTHROW(RoutingException, ("Can't get mwm handle for", file));

    MwmValue const & mwmValue = *handle.GetValue();
    try
    {
      FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(CAMERAS_INFO_FILE_TAG));
      ReaderSource<FilesContainerR::TReader> src(reader);
      DeserializeSpeedCamsFromMwm(src, res.first->second);
    }
    catch (Reader::OpenException const &)
    {
      LOG(LWARNING, (CAMERAS_INFO_FILE_TAG "section not found"));
    }
    catch (Reader::Exception const & e)
    {
      LOG(LERROR, ("Error while reading", CAMERAS_INFO_FILE_TAG, "section.", e.Msg()));
    }
  }

  return res.first->second;
}

vector<RouteSegment::SpeedCamera> IndexGraphLoaderImpl::GetSpeedCameraInfo(Segment const & segment)
{
  CamerasMapT const & camerasMap = ReceiveSpeedCamsFromMwm(segment.GetMwmId());
  auto it = camerasMap.find({segment.GetFeatureId(), segment.GetSegmentIdx()});
  if (it == camerasMap.end())
    return {};

  auto camerasTmp = it->second;
  std::sort(camerasTmp.begin(), camerasTmp.end());

  // TODO (@gmoryes) do this in generator.
  // This code matches cameras with equal coefficients and among them
  // only the camera with minimal maxSpeed is left.
  static constexpr auto kInvalidCoef = 2.0;
  camerasTmp.emplace_back(kInvalidCoef, 0.0 /* maxSpeedKmPH */);
  vector<RouteSegment::SpeedCamera> cameras;
  for (size_t i = 1; i < camerasTmp.size(); ++i)
  {
    static constexpr auto kEps = 1e-5;
    if (!base::AlmostEqualAbs(camerasTmp[i - 1].m_coef, camerasTmp[i].m_coef, kEps))
      cameras.emplace_back(camerasTmp[i - 1]);
  }
  // Cameras stored from beginning to ending of segment. So if we go at segment in backward direction,
  // we should read cameras in reverse sequence too.
  if (!segment.IsForward())
    std::reverse(cameras.begin(), cameras.end());

  return cameras;
}

unique_ptr<IndexGraph> IndexGraphLoaderImpl::CreateIndexGraph(NumMwmId numMwmId)
{
  platform::CountryFile const & file = m_numMwmIds->GetFile(numMwmId);
  MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  shared_ptr<VehicleModelInterface> vehicleModel =
      m_vehicleModelFactory->GetVehicleModelForCountry(file.GetName());

  auto geometry = make_shared<Geometry>(GeometryLoader::Create(
      m_dataSource, handle, vehicleModel, m_loadAltitudes));

  auto graph = make_unique<IndexGraph>(std::move(geometry), m_estimator, m_avoidRoutingOptions);
  graph->SetCurrentTimeGetter(m_currentTimeGetter);

  base::Timer timer;
  MwmValue const & mwmValue = *handle.GetValue();
  DeserializeIndexGraph(mwmValue, m_vehicleType, *graph);
  LOG(LINFO, (ROUTING_FILE_TAG, "section for", file.GetName(), "loaded in", timer.ElapsedSeconds(),
      "seconds"));
  return graph;
}

void IndexGraphLoaderImpl::Clear() { m_graphs.clear(); }

bool ReadRoadAccessFromMwm(MwmValue const & mwmValue, VehicleType vehicleType,
                           RoadAccess & roadAccess)
{
  try
  {
    auto const reader = mwmValue.m_cont.GetReader(ROAD_ACCESS_FILE_TAG);
    ReaderSource<FilesContainerR::TReader> src(reader);
    RoadAccessSerializer::Deserialize(src, vehicleType, roadAccess);
  }
  catch (Reader::OpenException const &)
  {
    LOG(LWARNING, (ROAD_ACCESS_FILE_TAG, "section not found"));
    return false;
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while reading", ROAD_ACCESS_FILE_TAG, "section.", e.Msg()));
    return false;
  }
  return true;
}
}  // namespace

namespace routing
{
// static
unique_ptr<IndexGraphLoader> IndexGraphLoader::Create(
    VehicleType vehicleType, bool loadAltitudes, shared_ptr<NumMwmIds> numMwmIds,
    shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
    shared_ptr<EdgeEstimator> estimator, DataSource & dataSource,
    RoutingOptions routingOptions)
{
  return make_unique<IndexGraphLoaderImpl>(vehicleType, loadAltitudes, numMwmIds, vehicleModelFactory,
                                           estimator, dataSource, routingOptions);
}

void DeserializeIndexGraph(MwmValue const & mwmValue, VehicleType vehicleType, IndexGraph & graph)
{
  FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(ROUTING_FILE_TAG));
  ReaderSource<FilesContainerR::TReader> src(reader);
  IndexGraphSerializer::Deserialize(graph, src, GetVehicleMask(vehicleType));
  RestrictionLoader restrictionLoader(mwmValue, graph);
  if (restrictionLoader.HasRestrictions())
  {
    graph.SetRestrictions(restrictionLoader.StealRestrictions());
    graph.SetUTurnRestrictions(restrictionLoader.StealNoUTurnRestrictions());
  }

  RoadAccess roadAccess;
  if (ReadRoadAccessFromMwm(mwmValue, vehicleType, roadAccess))
    graph.SetRoadAccess(move(roadAccess));
}

uint32_t DeserializeIndexGraphNumRoads(MwmValue const & mwmValue, VehicleType vehicleType)
{
  FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(ROUTING_FILE_TAG));
  ReaderSource<FilesContainerR::TReader> src(reader);
  return IndexGraphSerializer::DeserializeNumRoads(src, GetVehicleMask(vehicleType));
}
}  // namespace routing
