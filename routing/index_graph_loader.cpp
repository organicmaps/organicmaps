#include "routing/index_graph_loader.hpp"

#include "routing/index_graph_serialization.hpp"
#include "routing/restriction_loader.hpp"
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
  IndexGraphLoaderImpl(shared_ptr<NumMwmIds> numMwmIds,
                       shared_ptr<VehicleModelFactory> vehicleModelFactory,
                       shared_ptr<EdgeEstimator> estimator, Index & index);

  // IndexGraphLoader overrides:
  virtual IndexGraph & GetIndexGraph(NumMwmId numMwmId) override;
  virtual void Clear() override;

private:
  IndexGraph & Load(NumMwmId mwmId);

  Index & m_index;
  shared_ptr<NumMwmIds> m_numMwmIds;
  shared_ptr<VehicleModelFactory> m_vehicleModelFactory;
  shared_ptr<EdgeEstimator> m_estimator;
  unordered_map<NumMwmId, unique_ptr<IndexGraph>> m_graphs;
};

IndexGraphLoaderImpl::IndexGraphLoaderImpl(shared_ptr<NumMwmIds> numMwmIds,
                                           shared_ptr<VehicleModelFactory> vehicleModelFactory,
                                           shared_ptr<EdgeEstimator> estimator, Index & index)
  : m_index(index)
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

  shared_ptr<IVehicleModel> vehicleModel =
      m_vehicleModelFactory->GetVehicleModelForCountry(file.GetName());

  auto const mwmId = MwmSet::MwmId(handle.GetInfo());
  auto graphPtr =
      make_unique<IndexGraph>(GeometryLoader::Create(m_index, mwmId, vehicleModel), m_estimator);
  auto & graph = *graphPtr;

  MwmValue const & mwmValue = *handle.GetValue<MwmValue>();

  my::Timer timer;
  FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(ROUTING_FILE_TAG));
  ReaderSource<FilesContainerR::TReader> src(reader);
  IndexGraphSerializer::Deserialize(graph, src, kCarMask);
  RestrictionLoader restrictionLoader(mwmValue, graph);
  if (restrictionLoader.HasRestrictions())
    graph.SetRestrictions(restrictionLoader.StealRestrictions());

  m_graphs[numMwmId] = move(graphPtr);
  LOG(LINFO, (ROUTING_FILE_TAG, "section for", file.GetName(), "loaded in", timer.ElapsedSeconds(),
              "seconds"));
  return graph;
}

void IndexGraphLoaderImpl::Clear() { m_graphs.clear(); }
}  // namespace

namespace routing
{
// static
unique_ptr<IndexGraphLoader> IndexGraphLoader::Create(
    shared_ptr<NumMwmIds> numMwmIds, shared_ptr<VehicleModelFactory> vehicleModelFactory,
    shared_ptr<EdgeEstimator> estimator, Index & index)
{
  return make_unique<IndexGraphLoaderImpl>(numMwmIds, vehicleModelFactory, estimator, index);
}
}  // namespace routing
