#include "routing/transit_graph_loader.hpp"

#include "routing/routing_exceptions.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"

#include "base/timer.hpp"

#include "std/unique_ptr.hpp"

using namespace std;

namespace routing
{
TransitGraphLoader::TransitGraphLoader(shared_ptr<NumMwmIds> numMwmIds, Index & index)
  : m_index(index), m_numMwmIds(numMwmIds)
{
}

void TransitGraphLoader::Clear() { m_graphs.clear(); }

TransitGraph & TransitGraphLoader::GetTransitGraph(NumMwmId numMwmId)
{
  auto const it = m_graphs.find(numMwmId);
  if (it != m_graphs.cend())
    return *it->second;

  auto const emplaceRes = m_graphs.emplace(numMwmId, CreateTransitGraph(numMwmId));
  ASSERT(emplaceRes.second, ("Failed to add TransitGraph for", numMwmId, "to TransitGraphLoader."));
  return *(emplaceRes.first)->second;
}

std::unique_ptr<TransitGraph> TransitGraphLoader::CreateTransitGraph(NumMwmId numMwmId)
{
  platform::CountryFile const & file = m_numMwmIds->GetFile(numMwmId);
  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  my::Timer timer;
  auto graphPtr = make_unique<TransitGraph>();
  // TODO:
  // MwmValue const & mwmValue = *handle.GetValue<MwmValue>();
  // DeserializeTransitGraph(mwmValue, *graph);
  LOG(LINFO, (TRANSIT_FILE_TAG, "section for", file.GetName(), "loaded in",
              timer.ElapsedSeconds(), "seconds"));
  return graphPtr;
}
}  // namespace routing
