#include "routing/regions_router.hpp"

#include "routing/dummy_world_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/junction_visitor.hpp"
#include "routing/regions_sparse_graph.hpp"
#include "routing/routing_helpers.hpp"

#include "base/scope_guard.hpp"

namespace routing
{
RegionsRouter::RegionsRouter(CountryFileGetterFn const & countryFileGetter, std::shared_ptr<NumMwmIds> numMwmIds,
                             DataSource & dataSource, RouterDelegate const & delegate, Checkpoints const & checkpoints)
  : m_countryFileGetterFn(countryFileGetter)
  , m_numMwmIds(std::move(numMwmIds))
  , m_dataSource(dataSource)
  , m_checkpoints(checkpoints)
  , m_delegate(delegate)
{
  CHECK(m_countryFileGetterFn, ());
}

template <typename Vertex, typename Edge, typename Weight>
RouterResultCode RegionsRouter::ConvertResult(typename AStarAlgorithm<Vertex, Edge, Weight>::Result result) const
{
  switch (result)
  {
  case AStarAlgorithm<Vertex, Edge, Weight>::Result::NoPath: return RouterResultCode::RouteNotFound;
  case AStarAlgorithm<Vertex, Edge, Weight>::Result::Cancelled: return RouterResultCode::Cancelled;
  case AStarAlgorithm<Vertex, Edge, Weight>::Result::OK: return RouterResultCode::NoError;
  }
  UNREACHABLE();
}

RouterResultCode RegionsRouter::CalculateSubrouteNoLeapsMode(IndexGraphStarter & starter,
                                                             std::vector<Segment> & subroute,
                                                             m2::PointD const & startCheckpoint,
                                                             m2::PointD const & finishCheckpoint)
{
  using Vertex = IndexGraphStarter::Vertex;
  using Edge = IndexGraphStarter::Edge;
  using Weight = IndexGraphStarter::Weight;

  double constexpr almostZeroContribution = 1e-7;
  auto progress = std::make_shared<AStarProgress>();
  AStarSubProgress subProgress(mercator::ToLatLon(startCheckpoint), mercator::ToLatLon(finishCheckpoint),
                               almostZeroContribution);

  progress->AppendSubProgress(subProgress);
  SCOPE_GUARD(eraseProgress, [&progress]() { progress->PushAndDropLastSubProgress(); });

  using Visitor = JunctionVisitor<IndexGraphStarter>;
  uint32_t constexpr kVisitPeriod = 40;
  Visitor visitor(starter, m_delegate, kVisitPeriod, progress);

  AStarAlgorithm<Vertex, Edge, Weight>::Params<Visitor, AStarLengthChecker> params(
      starter, starter.GetStartSegment(), starter.GetFinishSegment(), m_delegate.GetCancellable(), std::move(visitor),
      AStarLengthChecker(starter));

  params.m_badReducedWeight = [](Weight const & reduced, Weight const & current)
  {
    // https://github.com/organicmaps/organicmaps/issues/333
    // Better to check relative error in cross-mwm regions graph, because it stores weights
    // in rounded meters, and we observe accumulated error like 5m per ~3500km.
    return fabs(reduced.GetWeight() / current.GetWeight()) > 1.0E-5;
  };

  RoutingResult<Vertex, Weight> routingResult;

  AStarAlgorithm<Vertex, Edge, Weight> algorithm;
  RouterResultCode const result =
      ConvertResult<Vertex, Edge, Weight>(algorithm.FindPathBidirectional(params, routingResult));

  if (result != RouterResultCode::NoError)
    return result;

  subroute = std::move(routingResult.m_path);
  return RouterResultCode::NoError;
}

void RegionsRouter::Do()
{
  m_mwmNames.clear();

  auto sparseGraph = std::make_shared<RegionsSparseGraph>(m_countryFileGetterFn, m_numMwmIds, m_dataSource);
  sparseGraph->LoadRegionsSparseGraph();

  std::unique_ptr<WorldGraph> graph = std::make_unique<DummyWorldGraph>();

  for (size_t i = 0; i < m_checkpoints.GetNumSubroutes(); ++i)
  {
    // equal mwm ids
    if (GetCheckpointRegion(i).second == GetCheckpointRegion(i + 1).second)
      continue;

    std::optional<FakeEnding> const startFakeEnding = sparseGraph->GetFakeEnding(m_checkpoints.GetPoint(i));
    if (!startFakeEnding)
      return;

    std::optional<FakeEnding> const finishFakeEnding = sparseGraph->GetFakeEnding(m_checkpoints.GetPoint(i + 1));
    if (!finishFakeEnding)
      return;

    IndexGraphStarter subrouteStarter(*startFakeEnding, *finishFakeEnding, 0 /* fakeNumerationStart */,
                                      false /* isStartSegmentStrictForward */, *graph);

    subrouteStarter.GetGraph().SetMode(WorldGraphMode::NoLeaps);

    subrouteStarter.SetRegionsGraphMode(sparseGraph);

    std::vector<Segment> subroute;

    auto const result = CalculateSubrouteNoLeapsMode(subrouteStarter, subroute, m_checkpoints.GetPoint(i),
                                                     m_checkpoints.GetPoint(i + 1));

    if (result != RouterResultCode::NoError)
      return;

    for (auto const & s : subroute)
    {
      for (bool front : {false, true})
      {
        auto const & ll = subrouteStarter.GetJunction(s, front).GetLatLon();

        std::string name = m_countryFileGetterFn(mercator::FromLatLon(ll));
        if (name.empty() && !IndexGraphStarter::IsFakeSegment(s))
          name = m_numMwmIds->GetFile(s.GetMwmId()).GetName();

        m_mwmNames.emplace(name);
      }
    }
  }
}

std::unordered_set<std::string> const & RegionsRouter::GetMwmNames() const
{
  return m_mwmNames;
}

std::pair<m2::PointD, std::string> RegionsRouter::GetCheckpointRegion(size_t index) const
{
  m2::PointD const & point = m_checkpoints.GetPoint(index);
  std::string const mwm = m_countryFileGetterFn(point);
  return {point, mwm};
}
}  // namespace routing
