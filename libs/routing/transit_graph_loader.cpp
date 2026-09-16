#include "routing/transit_graph_loader.hpp"

#include "routing/data_source.hpp"
#include "routing/fake_ending.hpp"
#include "routing/routing_exceptions.hpp"

#include "transit/transit_graph_data.hpp"

#include "platform/country_file.hpp"

#include "coding/files_container.hpp"

#include "base/timer.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace routing
{
class TransitGraphLoaderImpl : public TransitGraphLoader
{
public:
  TransitGraphLoaderImpl(MwmDataSource & dataSource, std::shared_ptr<EdgeEstimator> estimator)
    : m_dataSource(dataSource)
    , m_estimator(estimator)
  {}

  TransitGraph & GetTransitGraph(NumMwmId numMwmId, IndexGraph & indexGraph) override
  {
    auto const it = m_graphs.find(numMwmId);
    if (it != m_graphs.cend())
      return *it->second;

    auto const emplaceRes = m_graphs.emplace(numMwmId, CreateTransitGraph(numMwmId, indexGraph));
    ASSERT(emplaceRes.second, ("Failed to add TransitGraph for", numMwmId, "to TransitGraphLoader."));
    return *(emplaceRes.first)->second;
  }

  void Clear() override { m_graphs.clear(); }

private:
  std::unique_ptr<TransitGraph> CreateTransitGraph(NumMwmId numMwmId, IndexGraph & indexGraph) const
  {
    base::Timer timer;

    MwmValue const & mwmValue = m_dataSource.GetMwmValue(numMwmId);

    // By default return an empty transit graph.
    if (!mwmValue.m_cont.IsExist(TRANSIT_FILE_TAG))
      return std::make_unique<TransitGraph>(numMwmId, m_estimator);

    try
    {
      FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(TRANSIT_FILE_TAG));
      auto graph = std::make_unique<TransitGraph>(numMwmId, m_estimator);

      transit::GraphData transitData;
      transitData.DeserializeForRouting(*reader.GetPtr());

      TransitGraph::Endings gateEndings;
      MakeGateEndings(transitData.GetGates(), numMwmId, indexGraph, gateEndings);

      graph->Fill(transitData, gateEndings);

      LOG(LINFO, (TRANSIT_FILE_TAG, "section for", mwmValue.GetCountryFileName(), "loaded in", timer.ElapsedSeconds(),
                  "seconds"));

      return graph;
    }
    catch (Reader::OpenException const & e)
    {
      LOG(LERROR, ("Error while reading", TRANSIT_FILE_TAG, "section.", e.Msg()));
      throw;
    }

    UNREACHABLE();
  }

  MwmDataSource & m_dataSource;
  std::shared_ptr<EdgeEstimator> m_estimator;
  std::unordered_map<NumMwmId, std::unique_ptr<TransitGraph>> m_graphs;
};

// static
std::unique_ptr<TransitGraphLoader> TransitGraphLoader::Create(MwmDataSource & dataSource,
                                                               std::shared_ptr<EdgeEstimator> estimator)
{
  return std::make_unique<TransitGraphLoaderImpl>(dataSource, estimator);
}

}  // namespace routing
