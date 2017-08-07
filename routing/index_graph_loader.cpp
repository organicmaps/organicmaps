#include "routing/index_graph_loader.hpp"

#include "routing/index_graph_serialization.hpp"
#include "routing/restriction_loader.hpp"
#include "routing/road_access_serialization.hpp"
#include "routing/routing_exceptions.hpp"

#include "coding/file_container.hpp"

#include "base/assert.hpp"
#include "base/timer.hpp"

namespace
{
using namespace routing;
using namespace std;

class IndexGraphLoaderImpl final : public IndexGraphLoader
{
public:
  IndexGraphLoaderImpl(VehicleType vehicleType, shared_ptr<NumMwmIds> numMwmIds,
                       shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
                       shared_ptr<EdgeEstimator> estimator, Index & index);

  // IndexGraphLoader overrides:
  virtual IndexGraph & GetIndexGraph(NumMwmId numMwmId) override;
  virtual void Clear() override;

private:
  IndexGraph & Load(NumMwmId mwmId);

  VehicleMask m_vehicleMask;
  Index & m_index;
  shared_ptr<NumMwmIds> m_numMwmIds;
  shared_ptr<VehicleModelFactoryInterface> m_vehicleModelFactory;
  shared_ptr<EdgeEstimator> m_estimator;
  unordered_map<NumMwmId, unique_ptr<IndexGraph>> m_graphs;
};

IndexGraphLoaderImpl::IndexGraphLoaderImpl(VehicleType vehicleType, shared_ptr<NumMwmIds> numMwmIds,
                                           shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
                                           shared_ptr<EdgeEstimator> estimator, Index & index)
  : m_vehicleMask(GetVehicleMask(vehicleType))
  , m_index(index)
  , m_numMwmIds(numMwmIds)
  , m_vehicleModelFactory(vehicleModelFactory)
  , m_estimator(estimator)
{
  CHECK(m_numMwmIds, ());
  CHECK(m_vehicleModelFactory, ());
  CHECK(m_estimator, ());
}

IndexGraph & IndexGraphLoaderImpl::GetIndexGraph(NumMwmId numMwmId)
{
  auto it = m_graphs.find(numMwmId);
  if (it != m_graphs.end())
    return *it->second;

  return Load(numMwmId);
}

IndexGraph & IndexGraphLoaderImpl::Load(NumMwmId numMwmId)
{
  platform::CountryFile const & file = m_numMwmIds->GetFile(numMwmId);
  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  shared_ptr<VehicleModelInterface> vehicleModel =
      m_vehicleModelFactory->GetVehicleModelForCountry(file.GetName());

  auto graphPtr = make_unique<IndexGraph>(
      GeometryLoader::Create(m_index, handle, vehicleModel, m_vehicleMask != kCarMask),
      m_estimator);
  IndexGraph & graph = *graphPtr;

  my::Timer timer;
  MwmValue const & mwmValue = *handle.GetValue<MwmValue>();
  DeserializeIndexGraph(mwmValue, m_vehicleMask, graph);
  m_graphs[numMwmId] = move(graphPtr);
  LOG(LINFO, (ROUTING_FILE_TAG, "section for", file.GetName(), "loaded in", timer.ElapsedSeconds(),
              "seconds"));
  return graph;
}

void IndexGraphLoaderImpl::Clear() { m_graphs.clear(); }

bool ReadRoadAccessFromMwm(MwmValue const & mwmValue, RoadAccess & roadAccess)
{
  if (!mwmValue.m_cont.IsExist(ROAD_ACCESS_FILE_TAG))
    return false;

  try
  {
    auto const reader = mwmValue.m_cont.GetReader(ROAD_ACCESS_FILE_TAG);
    ReaderSource<FilesContainerR::TReader> src(reader);

    RoadAccessSerializer::Deserialize(src, VehicleType::Car, roadAccess);
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
    VehicleType vehicleType, shared_ptr<NumMwmIds> numMwmIds,
    shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory, shared_ptr<EdgeEstimator> estimator,
    Index & index)
{
  return make_unique<IndexGraphLoaderImpl>(vehicleType, numMwmIds, vehicleModelFactory, estimator,
                                           index);
}

void DeserializeIndexGraph(MwmValue const & mwmValue, VehicleMask vehicleMask, IndexGraph & graph)
{
  FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(ROUTING_FILE_TAG));
  ReaderSource<FilesContainerR::TReader> src(reader);
  IndexGraphSerializer::Deserialize(graph, src, vehicleMask);
  RestrictionLoader restrictionLoader(mwmValue, graph);
  if (restrictionLoader.HasRestrictions())
    graph.SetRestrictions(restrictionLoader.StealRestrictions());

  RoadAccess roadAccess;
  if (ReadRoadAccessFromMwm(mwmValue, roadAccess))
    graph.SetRoadAccess(move(roadAccess));
}
}  // namespace routing
