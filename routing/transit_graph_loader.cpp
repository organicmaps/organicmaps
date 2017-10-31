#include "routing/transit_graph_loader.hpp"

#include "routing/routing_exceptions.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_types.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"

#include "coding/file_container.hpp"

#include "base/timer.hpp"

#include "std/unique_ptr.hpp"

#include <vector>

using namespace std;

namespace routing
{
TransitGraphLoader::TransitGraphLoader(shared_ptr<NumMwmIds> numMwmIds, Index & index,
                                       shared_ptr<EdgeEstimator> estimator)
  : m_index(index), m_numMwmIds(numMwmIds), m_estimator(estimator)
{
}

void TransitGraphLoader::Clear() { m_graphs.clear(); }

TransitGraph & TransitGraphLoader::GetTransitGraph(NumMwmId numMwmId, IndexGraph & indexGraph)
{
  auto const it = m_graphs.find(numMwmId);
  if (it != m_graphs.cend())
    return *it->second;

  auto const emplaceRes = m_graphs.emplace(numMwmId, CreateTransitGraph(numMwmId, indexGraph));
  ASSERT(emplaceRes.second, ("Failed to add TransitGraph for", numMwmId, "to TransitGraphLoader."));
  return *(emplaceRes.first)->second;
}

unique_ptr<TransitGraph> TransitGraphLoader::CreateTransitGraph(NumMwmId numMwmId,
                                                                IndexGraph & indexGraph) const
{
  platform::CountryFile const & file = m_numMwmIds->GetFile(numMwmId);
  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  my::Timer timer;
  auto graphPtr = make_unique<TransitGraph>();
  MwmValue const & mwmValue = *handle.GetValue<MwmValue>();
  if (!mwmValue.m_cont.IsExist(TRANSIT_FILE_TAG))
    return graphPtr;

  try
  {
    FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(TRANSIT_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(reader);
    transit::Deserializer<ReaderSource<FilesContainerR::TReader>> deserializer(src);
    transit::FixSizeNumberDeserializer<ReaderSource<FilesContainerR::TReader>> numberDeserializer(src);

    transit::TransitHeader header;
    header.Visit(numberDeserializer);

    vector<transit::Stop> stops;
    deserializer(stops);

    CHECK_EQUAL(src.Pos(), header.m_gatesOffset,("Wrong section format."));
    vector<transit::Gate> gates;
    deserializer(gates);

    CHECK_EQUAL(src.Pos(), header.m_edgesOffset,("Wrong section format."));
    vector<transit::Edge> edges;
    deserializer(edges);

    graphPtr->Fill(stops, gates, edges, *m_estimator, numMwmId, indexGraph);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("Error while reading", TRANSIT_FILE_TAG, "section.", e.Msg()));
    throw;
  }

  LOG(LINFO, (TRANSIT_FILE_TAG, "section for", file.GetName(), "loaded in",
              timer.ElapsedSeconds(), "seconds"));
  return graphPtr;
}
}  // namespace routing
