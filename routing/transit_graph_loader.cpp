#include "routing/transit_graph_loader.hpp"

#include "routing/fake_ending.hpp"
#include "routing/routing_exceptions.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_types.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"

#include "coding/file_container.hpp"

#include "base/stl_add.hpp"
#include "base/timer.hpp"

#include <unordered_map>
#include <vector>

using namespace std;

namespace routing
{
class TransitGraphLoaderImpl : public TransitGraphLoader
{
public:
  TransitGraphLoaderImpl(Index & index, shared_ptr<NumMwmIds> numMwmIds,
                         shared_ptr<EdgeEstimator> estimator);

  // TransitGraphLoader overrides.
  ~TransitGraphLoaderImpl() override = default;

  TransitGraph & GetTransitGraph(NumMwmId mwmId, IndexGraph & indexGraph) override;
  void Clear() override;

private:
  unique_ptr<TransitGraph> CreateTransitGraph(NumMwmId mwmId, IndexGraph & indexGraph) const;

  Index & m_index;
  shared_ptr<NumMwmIds> m_numMwmIds;
  shared_ptr<EdgeEstimator> m_estimator;
  unordered_map<NumMwmId, unique_ptr<TransitGraph>> m_graphs;
};

TransitGraphLoaderImpl::TransitGraphLoaderImpl(Index & index, shared_ptr<NumMwmIds> numMwmIds,
                                               shared_ptr<EdgeEstimator> estimator)
  : m_index(index), m_numMwmIds(numMwmIds), m_estimator(estimator)
{
}

void TransitGraphLoaderImpl::Clear() { m_graphs.clear(); }

TransitGraph & TransitGraphLoaderImpl::GetTransitGraph(NumMwmId numMwmId, IndexGraph & indexGraph)
{
  auto const it = m_graphs.find(numMwmId);
  if (it != m_graphs.cend())
    return *it->second;

  auto const emplaceRes = m_graphs.emplace(numMwmId, CreateTransitGraph(numMwmId, indexGraph));
  ASSERT(emplaceRes.second, ("Failed to add TransitGraph for", numMwmId, "to TransitGraphLoader."));
  return *(emplaceRes.first)->second;
}

unique_ptr<TransitGraph> TransitGraphLoaderImpl::CreateTransitGraph(NumMwmId numMwmId,
                                                                    IndexGraph & indexGraph) const
{
  platform::CountryFile const & file = m_numMwmIds->GetFile(numMwmId);
  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  my::Timer timer;
  auto graph = my::make_unique<TransitGraph>(numMwmId, m_estimator);
  MwmValue const & mwmValue = *handle.GetValue<MwmValue>();
  if (!mwmValue.m_cont.IsExist(TRANSIT_FILE_TAG))
    return graph;

  try
  {
    FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(TRANSIT_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(reader);
    transit::Deserializer<ReaderSource<FilesContainerR::TReader>> deserializer(src);
    transit::FixedSizeDeserializer<ReaderSource<FilesContainerR::TReader>> numberDeserializer(src);

    transit::TransitHeader header;
    numberDeserializer(header);

    TransitGraph::TransitData transitData;

    CHECK_EQUAL(src.Pos(), header.m_stopsOffset, ("Wrong section format."));
    deserializer(transitData.m_stops);

    CHECK_EQUAL(src.Pos(), header.m_gatesOffset, ("Wrong section format."));
    deserializer(transitData.m_gates);

    CHECK_EQUAL(src.Pos(), header.m_edgesOffset, ("Wrong section format."));
    deserializer(transitData.m_edges);

    src.Skip(header.m_linesOffset - src.Pos());
    CHECK_EQUAL(src.Pos(), header.m_linesOffset, ("Wrong section format."));
    deserializer(transitData.m_lines);

    TransitGraph::GateEndings gateEndings;
    MakeGateEndings(transitData.m_gates, numMwmId, indexGraph, gateEndings);

    graph->Fill(transitData, gateEndings);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("Error while reading", TRANSIT_FILE_TAG, "section.", e.Msg()));
    throw;
  }

  LOG(LINFO, (TRANSIT_FILE_TAG, "section for", file.GetName(), "loaded in",
              timer.ElapsedSeconds(), "seconds"));
  return graph;
}

// static
unique_ptr<TransitGraphLoader> TransitGraphLoader::Create(Index & index,
                                                          shared_ptr<NumMwmIds> numMwmIds,
                                                          shared_ptr<EdgeEstimator> estimator)
{
  return my::make_unique<TransitGraphLoaderImpl>(index, numMwmIds, estimator);
}

}  // namespace routing
