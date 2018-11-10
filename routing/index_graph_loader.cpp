#include "routing/index_graph_loader.hpp"

#include "routing/city_roads.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/maxspeeds.hpp"
#include "routing/restriction_loader.hpp"
#include "routing/road_access_serialization.hpp"
#include "routing/route.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/speed_camera_ser_des.hpp"

#include "indexer/data_source.hpp"

#include "coding/file_container.hpp"

#include "base/assert.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <map>
#include <unordered_map>

namespace
{
using namespace routing;
using namespace std;

class IndexGraphLoaderImpl final : public IndexGraphLoader
{
public:
  IndexGraphLoaderImpl(VehicleType vehicleType, bool loadAltitudes, shared_ptr<NumMwmIds> numMwmIds,
                       shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
                       shared_ptr<EdgeEstimator> estimator, DataSource & dataSource);

  // IndexGraphLoader overrides:
  Geometry & GetGeometry(NumMwmId numMwmId) override;
  IndexGraph & GetIndexGraph(NumMwmId numMwmId) override;
  vector<RouteSegment::SpeedCamera> GetSpeedCameraInfo(Segment const & segment) override;
  void Clear() override;

private:
  struct GraphAttrs
  {
    shared_ptr<Geometry> m_geometry;
    unique_ptr<IndexGraph> m_indexGraph;
  };

  GraphAttrs & CreateGeometry(NumMwmId numMwmId);
  GraphAttrs & CreateIndexGraph(NumMwmId numMwmId, GraphAttrs & graph);

  VehicleType m_vehicleType;
  bool m_loadAltitudes;
  DataSource & m_dataSource;
  shared_ptr<NumMwmIds> m_numMwmIds;
  shared_ptr<VehicleModelFactoryInterface> m_vehicleModelFactory;
  shared_ptr<EdgeEstimator> m_estimator;

  unordered_map<NumMwmId, GraphAttrs> m_graphs;

  // TODO (@gmoryes) move this field to |GeometryIndexGraph| after @bykoianko PR
  unordered_map<NumMwmId, map<SegmentCoord, vector<RouteSegment::SpeedCamera>>> m_cachedCameras;
  decltype(m_cachedCameras)::iterator ReceiveSpeedCamsFromMwm(NumMwmId numMwmId);
};

IndexGraphLoaderImpl::IndexGraphLoaderImpl(
    VehicleType vehicleType, bool loadAltitudes, shared_ptr<NumMwmIds> numMwmIds,
    shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
    shared_ptr<EdgeEstimator> estimator, DataSource & dataSource)
  : m_vehicleType(vehicleType)
  , m_loadAltitudes(loadAltitudes)
  , m_dataSource(dataSource)
  , m_numMwmIds(numMwmIds)
  , m_vehicleModelFactory(vehicleModelFactory)
  , m_estimator(estimator)
{
  CHECK(m_numMwmIds, ());
  CHECK(m_vehicleModelFactory, ());
  CHECK(m_estimator, ());
}

Geometry & IndexGraphLoaderImpl::GetGeometry(NumMwmId numMwmId)
{
  auto it = m_graphs.find(numMwmId);
  if (it != m_graphs.end())
    return *it->second.m_geometry;

  return *CreateGeometry(numMwmId).m_geometry;
}

IndexGraph & IndexGraphLoaderImpl::GetIndexGraph(NumMwmId numMwmId)
{
  auto it = m_graphs.find(numMwmId);
  if (it != m_graphs.end())
  {
    return it->second.m_indexGraph ? *it->second.m_indexGraph
                                   : *CreateIndexGraph(numMwmId, it->second).m_indexGraph;
  }

  return *CreateIndexGraph(numMwmId, CreateGeometry(numMwmId)).m_indexGraph;
}

auto IndexGraphLoaderImpl::ReceiveSpeedCamsFromMwm(NumMwmId numMwmId) -> decltype(m_cachedCameras)::iterator
{
  m_cachedCameras.emplace(numMwmId,
                          map<SegmentCoord, vector<RouteSegment::SpeedCamera>>{});
  auto & mapReference = m_cachedCameras.find(numMwmId)->second;

  auto const & file = m_numMwmIds->GetFile(numMwmId);
  auto handle = m_dataSource.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  MwmValue const & mwmValue = *handle.GetValue<MwmValue>();
  if (!mwmValue.m_cont.IsExist(CAMERAS_INFO_FILE_TAG))
  {
    LOG(LINFO, ("No section about speed cameras"));
    return m_cachedCameras.end();
  }

  try
  {
    FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(CAMERAS_INFO_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(reader);
    DeserializeSpeedCamsFromMwm(src, mapReference);
  }
  catch (Reader::OpenException & ex)
  {
    LOG(LINFO, ("No section about speed cameras"));
    return m_cachedCameras.end();
  }

  decltype(m_cachedCameras)::iterator it;
  it = m_cachedCameras.find(numMwmId);
  CHECK(it != m_cachedCameras.end(), ());

  return it;
}

vector<RouteSegment::SpeedCamera> IndexGraphLoaderImpl::GetSpeedCameraInfo(Segment const & segment)
{
  auto const numMwmId = segment.GetMwmId();

  auto it = m_cachedCameras.find(numMwmId);
  if (it == m_cachedCameras.end())
    it = ReceiveSpeedCamsFromMwm(numMwmId);

  if (it == m_cachedCameras.end())
    return {};

  auto result = it->second.find({segment.GetFeatureId(), segment.GetSegmentIdx()});
  if (result == it->second.end())
    return {};

  auto camerasTmp = result->second;
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

IndexGraphLoaderImpl::GraphAttrs & IndexGraphLoaderImpl::CreateGeometry(NumMwmId numMwmId)
{
  platform::CountryFile const & file = m_numMwmIds->GetFile(numMwmId);
  MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  shared_ptr<VehicleModelInterface> vehicleModel =
      m_vehicleModelFactory->GetVehicleModelForCountry(file.GetName());

  auto & graph = m_graphs[numMwmId];
  graph.m_geometry = make_shared<Geometry>(GeometryLoader::Create(
      m_dataSource, handle, vehicleModel, AttrLoader(m_dataSource, handle), m_loadAltitudes));
  return graph;
}

IndexGraphLoaderImpl::GraphAttrs & IndexGraphLoaderImpl::CreateIndexGraph(
    NumMwmId numMwmId, GraphAttrs & graph)
{
  CHECK(graph.m_geometry, ());
  platform::CountryFile const & file = m_numMwmIds->GetFile(numMwmId);
  MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  graph.m_indexGraph = make_unique<IndexGraph>(graph.m_geometry, m_estimator);
  base::Timer timer;
  MwmValue const & mwmValue = *handle.GetValue<MwmValue>();
  DeserializeIndexGraph(mwmValue, m_vehicleType, *graph.m_indexGraph);
  LOG(LINFO, (ROUTING_FILE_TAG, "section for", file.GetName(), "loaded in", timer.ElapsedSeconds(),
      "seconds"));
  return graph;
}

void IndexGraphLoaderImpl::Clear() { m_graphs.clear(); }

bool ReadRoadAccessFromMwm(MwmValue const & mwmValue, VehicleType vehicleType,
                           RoadAccess & roadAccess)
{
  if (!mwmValue.m_cont.IsExist(ROAD_ACCESS_FILE_TAG))
    return false;

  try
  {
    auto const reader = mwmValue.m_cont.GetReader(ROAD_ACCESS_FILE_TAG);
    ReaderSource<FilesContainerR::TReader> src(reader);

    RoadAccessSerializer::Deserialize(src, vehicleType, roadAccess);
  }
  catch (Reader::OpenException const & e)
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
    shared_ptr<EdgeEstimator> estimator, DataSource & dataSource)
{
  return make_unique<IndexGraphLoaderImpl>(vehicleType, loadAltitudes, numMwmIds, vehicleModelFactory,
                                           estimator, dataSource);
}

void DeserializeIndexGraph(MwmValue const & mwmValue, VehicleType vehicleType, IndexGraph & graph)
{
  FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(ROUTING_FILE_TAG));
  ReaderSource<FilesContainerR::TReader> src(reader);
  IndexGraphSerializer::Deserialize(graph, src, GetVehicleMask(vehicleType));
  RestrictionLoader restrictionLoader(mwmValue, graph);
  if (restrictionLoader.HasRestrictions())
    graph.SetRestrictions(restrictionLoader.StealRestrictions());

  RoadAccess roadAccess;
  if (ReadRoadAccessFromMwm(mwmValue, vehicleType, roadAccess))
    graph.SetRoadAccess(move(roadAccess));
}
}  // namespace routing
