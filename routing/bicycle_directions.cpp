#include "routing/bicycle_directions.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/road_point.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/turns_generator.hpp"

#include "traffic/traffic_info.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"

namespace
{
using namespace routing;
using namespace routing::turns;
using namespace traffic;

class RoutingResult : public IRoutingResult
{
public:
  RoutingResult(IRoadGraph::TEdgeVector const & routeEdges,
                BicycleDirectionsEngine::AdjacentEdgesMap const & adjacentEdges,
                TUnpackedPathSegments const & pathSegments)
    : m_routeEdges(routeEdges)
    , m_adjacentEdges(adjacentEdges)
    , m_pathSegments(pathSegments)
    , m_routeLength(0)
  {
    for (auto const & edge : routeEdges)
    {
      m_routeLength += MercatorBounds::DistanceOnEarth(edge.GetStartJunction().GetPoint(),
                                                       edge.GetEndJunction().GetPoint());
    }
  }

  // turns::IRoutingResult overrides:
  TUnpackedPathSegments const & GetSegments() const override { return m_pathSegments; }
  void GetPossibleTurns(UniNodeId const & node, m2::PointD const & /* ingoingPoint */,
                        m2::PointD const & /* junctionPoint */, size_t & ingoingCount,
                        TurnCandidates & outgoingTurns) const override
  {
    ingoingCount = 0;
    outgoingTurns.candidates.clear();

    auto const adjacentEdges = m_adjacentEdges.find(node);
    if (adjacentEdges == m_adjacentEdges.cend())
    {
      ASSERT(false, ());
      return;
    }

    ingoingCount = adjacentEdges->second.m_ingoingTurnsCount;
    outgoingTurns = adjacentEdges->second.m_outgoingTurns;
  }

  double GetPathLength() const override { return m_routeLength; }

  Junction GetStartPoint() const override
  {
    CHECK(!m_routeEdges.empty(), ());
    return m_routeEdges.front().GetStartJunction();
  }

  Junction GetEndPoint() const override
  {
    CHECK(!m_routeEdges.empty(), ());
    return m_routeEdges.back().GetEndJunction();
  }

private:
  IRoadGraph::TEdgeVector const & m_routeEdges;
  BicycleDirectionsEngine::AdjacentEdgesMap const & m_adjacentEdges;
  TUnpackedPathSegments const & m_pathSegments;
  double m_routeLength;
};
}  // namespace

namespace routing
{
BicycleDirectionsEngine::BicycleDirectionsEngine(Index const & index,
                                                 shared_ptr<NumMwmIds> numMwmIds)
  : m_index(index), m_numMwmIds(numMwmIds)
{
}

void BicycleDirectionsEngine::Generate(RoadGraphBase const & graph, vector<Junction> const & path,
                                       my::Cancellable const & cancellable, Route::TTimes & times,
                                       Route::TTurns & turns, vector<Junction> & routeGeometry,
                                       vector<Segment> & trafficSegs)
{
  times.clear();
  turns.clear();
  routeGeometry.clear();
  m_adjacentEdges.clear();
  m_pathSegments.clear();
  trafficSegs.clear();

  size_t const pathSize = path.size();
  if (pathSize == 0)
    return;

  auto emptyPathWorkaround = [&]()
  {
    turns.emplace_back(pathSize - 1, turns::TurnDirection::ReachedYourDestination);
    // There's one ingoing edge to the finish.
    this->m_adjacentEdges[UniNodeId(UniNodeId::Type::Mwm)] = AdjacentEdges(1);
  };

  if (pathSize == 1)
  {
    emptyPathWorkaround();
    return;
  }

  CalculateTimes(graph, path, times);

  IRoadGraph::TEdgeVector routeEdges;
  if (!ReconstructPath(graph, path, routeEdges, cancellable))
  {
    LOG(LDEBUG, ("Couldn't reconstruct path."));
    emptyPathWorkaround();
    return;
  }
  if (routeEdges.empty())
  {
    emptyPathWorkaround();
    return;
  }

  // Filling |m_adjacentEdges|.
  m_adjacentEdges.insert(make_pair(UniNodeId(UniNodeId::Type::Mwm), AdjacentEdges(0)));
  for (size_t i = 1; i < pathSize; ++i)
  {
    if (cancellable.IsCancelled())
      return;

    Junction const & prevJunction = path[i - 1];
    Junction const & currJunction = path[i];
    IRoadGraph::TEdgeVector outgoingEdges, ingoingEdges;
    graph.GetOutgoingEdges(currJunction, outgoingEdges);
    graph.GetIngoingEdges(currJunction, ingoingEdges);

    AdjacentEdges adjacentEdges = AdjacentEdges(ingoingEdges.size());
    // Outgoing edge angle is not used for bicyle routing.
    adjacentEdges.m_outgoingTurns.isCandidatesAngleValid = false;
    adjacentEdges.m_outgoingTurns.candidates.reserve(outgoingEdges.size());
    ASSERT_EQUAL(routeEdges.size(), pathSize - 1, ());
    FeatureID const inFeatureId = routeEdges[i - 1].GetFeatureId();
    uint32_t const inSegId = routeEdges[i - 1].GetSegId();
    bool const inIsForward = routeEdges[i - 1].IsForward();
    UniNodeId const uniNodeId(inFeatureId, inSegId, inIsForward);

    for (auto const & edge : outgoingEdges)
    {
      auto const & outFeatureId = edge.GetFeatureId();
      // Checking for if |edge| is a fake edge.
      if (!outFeatureId.IsValid())
        continue;

      FeatureType ft;
      if (!GetLoader(outFeatureId.m_mwmId).GetFeatureByIndex(outFeatureId.m_index, ft))
        continue;

      auto const highwayClass = ftypes::GetHighwayClass(ft);
      ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
      ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());
      adjacentEdges.m_outgoingTurns.candidates.emplace_back(0.0 /* angle */, uniNodeId,
                                                            highwayClass);
    }

    LoadedPathSegment pathSegment(UniNodeId::Type::Mwm);
    // @TODO(bykoianko) This place should be fixed. Putting |prevJunction| and |currJunction|
    // for every route edge leads that all route points are duplicated. It's because
    // prevJunction == path[i - 1] and currJunction == path[i].
    if (inFeatureId.IsValid())
      LoadPathGeometry(uniNodeId, {prevJunction, currJunction}, pathSegment);

    if (m_numMwmIds)
    {
      ASSERT(inFeatureId.m_mwmId.IsAlive(), ());
      NumMwmId const numMwmId =
          m_numMwmIds->GetId(inFeatureId.m_mwmId.GetInfo()->GetLocalFile().GetCountryFile());
      pathSegment.m_trafficSegs = {{numMwmId, inFeatureId.m_index, inSegId, inIsForward}};
    }

    auto const it = m_adjacentEdges.insert(make_pair(uniNodeId, move(adjacentEdges)));
    ASSERT(it.second, ());
    UNUSED_VALUE(it);
    m_pathSegments.push_back(move(pathSegment));
  }

  RoutingResult resultGraph(routeEdges, m_adjacentEdges, m_pathSegments);
  RouterDelegate delegate;
  Route::TTimes turnAnnotationTimes;
  Route::TStreets streetNames;

  MakeTurnAnnotation(resultGraph, delegate, routeGeometry, turns, turnAnnotationTimes,
                     streetNames, trafficSegs);

  // @TODO(bykoianko) The invariant below it's an issue but now it's so and it should be checked.
  // The problem is every edge is added as a pair of points to route geometry.
  // So all the points except for beginning and ending are duplicated. It should
  // be fixed in the future.

  // Note 1. Accoding to current implementation all internal points are duplicated for
  // all A* routes.
  // Note 2. Number of |trafficSegs| should be equal to number of |routeEdges|.
  ASSERT_EQUAL(routeGeometry.size(), 2 * (pathSize - 2) + 2, ());
  ASSERT_EQUAL(trafficSegs.size(), pathSize - 1, ());
}

Index::FeaturesLoaderGuard & BicycleDirectionsEngine::GetLoader(MwmSet::MwmId const & id)
{
  if (!m_loader || id != m_loader->GetId())
    m_loader = make_unique<Index::FeaturesLoaderGuard>(m_index, id);
  return *m_loader;
}

void BicycleDirectionsEngine::LoadPathGeometry(UniNodeId const & uniNodeId,
                                               vector<Junction> const & path,
                                               LoadedPathSegment & pathSegment)
{
  pathSegment.Clear();

  if (!uniNodeId.GetFeature().IsValid())
  {
    ASSERT(false, ());
    return;
  }

  FeatureType ft;
  if (!GetLoader(uniNodeId.GetFeature().m_mwmId)
           .GetFeatureByIndex(uniNodeId.GetFeature().m_index, ft))
  {
    // The feature can't be read, therefore path geometry can't be
    // loaded.
    return;
  }

  auto const highwayClass = ftypes::GetHighwayClass(ft);
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Error, ());
  ASSERT_NOT_EQUAL(highwayClass, ftypes::HighwayClass::Undefined, ());

  pathSegment.m_highwayClass = highwayClass;
  pathSegment.m_isLink = ftypes::IsLinkChecker::Instance()(ft);

  ft.GetName(FeatureType::DEFAULT_LANG, pathSegment.m_name);

  pathSegment.m_nodeId = uniNodeId;
  pathSegment.m_onRoundabout = ftypes::IsRoundAboutChecker::Instance()(ft);
  pathSegment.m_path = path;
  // @TODO(bykoianko) It's better to fill pathSegment.m_weight.
}
}  // namespace routing
