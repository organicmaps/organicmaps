#include "routing/road_graph_router.hpp"

#include "routing/bicycle_directions.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/nearest_edge_finder.hpp"
#include "routing/pedestrian_directions.hpp"
#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "coding/reader_wrapper.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"

#include "geometry/distance.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/map.hpp"
#include "std/queue.hpp"
#include "std/set.hpp"

#include <utility>

using platform::CountryFile;
using platform::LocalCountryFile;

namespace routing
{

namespace
{
size_t constexpr kMaxRoadCandidates = 12;

size_t constexpr kTestConnectivityVisitJunctionsLimit = 25;

uint64_t constexpr kMinPedestrianMwmVersion = 150713;

// Check if the found edges lays on mwm with pedestrian routing support.
bool CheckMwmVersion(vector<pair<Edge, Junction>> const & vicinities, vector<string> & mwmNames)
{
  mwmNames.clear();
  for (auto const & vicinity : vicinities)
  {
    auto const mwmInfo = vicinity.first.GetFeatureId().m_mwmId.GetInfo();
    // mwmInfo gets version from a path of the file, so we must read the version header to be sure.
    ModelReaderPtr reader = FilesContainerR(mwmInfo->GetLocalFile().GetPath(MapOptions::Map))
                                            .GetReader(VERSION_FILE_TAG);
    ReaderSrc src(reader.GetPtr());

    version::MwmVersion version;
    version::ReadVersion(src, version);
    if (version.GetVersion() < kMinPedestrianMwmVersion)
      mwmNames.push_back(mwmInfo->GetCountryName());
  }
  return !mwmNames.empty();
}

// Find closest candidates in the world graph
void FindClosestEdges(IRoadGraph const & graph, m2::PointD const & point,
                      vector<pair<Edge, Junction>> & vicinity)
{
  // WARNING: Take only one vicinity, with, maybe, its inverse.  It is
  // an oversimplification that is not as easily solved as tuning up
  // this constant because if you set it too high you risk to find a
  // feature that you cannot in fact reach because of an obstacle.
  // Using only the closest feature minimizes (but not eliminates)
  // this risk.
  vector<pair<Edge, Junction>> candidates;
  graph.FindClosestEdges(point, kMaxRoadCandidates, candidates);

  vicinity.clear();
  for (auto const & candidate : candidates)
  {
    auto const & edge = candidate.first;
    auto const getVertexByEdgeFn = [](Edge const & edge) { return edge.GetEndJunction(); };
    auto const getOutgoingEdgesFn = [](IRoadGraph const & graph, Junction const & u,
                                       vector<Edge> & edges) { graph.GetOutgoingEdges(u, edges); };

    if (CheckGraphConnectivity(edge.GetEndJunction(), kTestConnectivityVisitJunctionsLimit,
                               graph, getVertexByEdgeFn, getOutgoingEdgesFn))
    {
      vicinity.emplace_back(candidate);

      // Need to add a reverse edge, if exists, because fake edges
      // must be added for reverse edge too.
      IRoadGraph::TEdgeVector revEdges;
      graph.GetOutgoingEdges(edge.GetEndJunction(), revEdges);
      for (auto const & revEdge : revEdges)
      {
        if (revEdge.GetFeatureId() == edge.GetFeatureId() &&
            revEdge.GetEndJunction() == edge.GetStartJunction() &&
            revEdge.GetSegId() == edge.GetSegId())
        {
          vicinity.emplace_back(revEdge, candidate.second);
          break;
        }
      }

      break;
    }
  }
}
}  // namespace

RoadGraphRouter::~RoadGraphRouter() {}
RoadGraphRouter::RoadGraphRouter(string const & name, Index const & index,
                                 TCountryFileFn const & countryFileFn, IRoadGraph::Mode mode,
                                 unique_ptr<VehicleModelFactoryInterface> && vehicleModelFactory,
                                 unique_ptr<IRoutingAlgorithm> && algorithm,
                                 unique_ptr<IDirectionsEngine> && directionsEngine)
  : m_name(name)
  , m_countryFileFn(countryFileFn)
  , m_index(index)
  , m_algorithm(move(algorithm))
  , m_roadGraph(make_unique<FeaturesRoadGraph>(index, mode, move(vehicleModelFactory)))
  , m_directionsEngine(move(directionsEngine))
{
}

void RoadGraphRouter::ClearState()
{
  m_roadGraph->ClearState();
}

bool RoadGraphRouter::CheckMapExistence(m2::PointD const & point, Route & route) const
{
  string const fileName = m_countryFileFn(point);
  if (!m_index.GetMwmIdByCountryFile(CountryFile(fileName)).IsAlive())
  {
    route.AddAbsentCountry(fileName);
    return false;
  }
  return true;
}

IRouter::ResultCode RoadGraphRouter::CalculateRoute(Checkpoints const & checkpoints,
                                                    m2::PointD const &, bool,
                                                    RouterDelegate const & delegate, Route & route)
{
  auto const & startPoint = checkpoints.GetStart();
  auto const & finalPoint = checkpoints.GetFinish();

  if (!CheckMapExistence(startPoint, route) || !CheckMapExistence(finalPoint, route))
    return IRouter::RouteFileNotExist;

  vector<pair<Edge, Junction>> finalVicinity;
  FindClosestEdges(*m_roadGraph, finalPoint, finalVicinity);

  if (finalVicinity.empty())
    return IRouter::EndPointNotFound;

  // TODO (ldragunov) Remove this check after several releases. (Estimated in november)
  vector<string> mwmNames;
  if (CheckMwmVersion(finalVicinity, mwmNames))
  {
    for (auto const & name : mwmNames)
      route.AddAbsentCountry(name);
  }

  vector<pair<Edge, Junction>> startVicinity;
  FindClosestEdges(*m_roadGraph, startPoint, startVicinity);

  if (startVicinity.empty())
    return IRouter::StartPointNotFound;

  // TODO (ldragunov) Remove this check after several releases. (Estimated in november)
  if (CheckMwmVersion(startVicinity, mwmNames))
  {
    for (auto const & name : mwmNames)
      route.AddAbsentCountry(name);
  }

  if (!route.GetAbsentCountries().empty())
    return IRouter::FileTooOld;

  // Let us assume that the closest to startPoint/finalPoint feature point has the same altitude
  // with startPoint/finalPoint.
  Junction const startPos(startPoint, startVicinity.front().second.GetAltitude());
  Junction const finalPos(finalPoint, finalVicinity.front().second.GetAltitude());

  m_roadGraph->ResetFakes();
  m_roadGraph->AddFakeEdges(startPos, startVicinity);
  m_roadGraph->AddFakeEdges(finalPos, finalVicinity);

  RoutingResult<Junction, double /* WeightType */> result;
  IRoutingAlgorithm::Result const resultCode =
      m_algorithm->CalculateRoute(*m_roadGraph, startPos, finalPos, delegate, result);

  if (resultCode == IRoutingAlgorithm::Result::OK)
  {
    ASSERT(!result.m_path.empty(), ());
    ASSERT_EQUAL(result.m_path.front(), startPos, ());
    ASSERT_EQUAL(result.m_path.back(), finalPos, ());
    ASSERT_GREATER(result.m_distance, 0., ());

    Route::TTimes times;
    CalculateMaxSpeedTimes(*m_roadGraph, result.m_path, times);

    CHECK(m_directionsEngine, ());
    route.SetSubroteAttrs(vector<Route::SubrouteAttrs>(
        {Route::SubrouteAttrs(startPos, finalPos, 0, result.m_path.size() - 1)}));
    ReconstructRoute(*m_directionsEngine, *m_roadGraph, nullptr /* trafficStash */, delegate,
                     result.m_path, map<Segment, TransitInfo>(), std::move(times), route);
  }

  m_roadGraph->ResetFakes();

  if (delegate.IsCancelled())
    return IRouter::Cancelled;

  if (!route.IsValid())
    return IRouter::RouteNotFound;

  switch (resultCode)
  {
    case IRoutingAlgorithm::Result::OK: return IRouter::NoError;
    case IRoutingAlgorithm::Result::NoPath: return IRouter::RouteNotFound;
    case IRoutingAlgorithm::Result::Cancelled: return IRouter::Cancelled;
  }
  ASSERT(false, ("Unexpected IRoutingAlgorithm::Result code:", resultCode));
  return IRouter::RouteNotFound;
}

unique_ptr<IRouter> CreatePedestrianAStarRouter(Index & index,
                                                TCountryFileFn const & countryFileFn,
                                                shared_ptr<NumMwmIds> numMwmIds)
{
  unique_ptr<VehicleModelFactoryInterface> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarRoutingAlgorithm());
  unique_ptr<IDirectionsEngine> directionsEngine(
      new PedestrianDirectionsEngine(std::move(numMwmIds)));
  unique_ptr<IRouter> router(new RoadGraphRouter(
      "astar-pedestrian", index, countryFileFn, IRoadGraph::Mode::IgnoreOnewayTag,
      move(vehicleModelFactory), move(algorithm), move(directionsEngine)));
  return router;
}

unique_ptr<IRouter> CreatePedestrianAStarBidirectionalRouter(Index & index,
                                                             TCountryFileFn const & countryFileFn,
                                                             shared_ptr<NumMwmIds> numMwmIds)
{
  unique_ptr<VehicleModelFactoryInterface> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarBidirectionalRoutingAlgorithm());
  unique_ptr<IDirectionsEngine> directionsEngine(
      new PedestrianDirectionsEngine(std::move(numMwmIds)));
  unique_ptr<IRouter> router(new RoadGraphRouter(
      "astar-bidirectional-pedestrian", index, countryFileFn, IRoadGraph::Mode::IgnoreOnewayTag,
      move(vehicleModelFactory), move(algorithm), move(directionsEngine)));
  return router;
}

unique_ptr<IRouter> CreateBicycleAStarBidirectionalRouter(Index & index,
                                                          TCountryFileFn const & countryFileFn,
                                                          shared_ptr<NumMwmIds> numMwmIds)
{
  unique_ptr<VehicleModelFactoryInterface> vehicleModelFactory(new BicycleModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarBidirectionalRoutingAlgorithm());
  unique_ptr<IDirectionsEngine> directionsEngine(
    new BicycleDirectionsEngine(index, numMwmIds));
  unique_ptr<IRouter> router(new RoadGraphRouter(
      "astar-bidirectional-bicycle", index, countryFileFn, IRoadGraph::Mode::ObeyOnewayTag,
      move(vehicleModelFactory), move(algorithm), move(directionsEngine)));
  return router;
}
}  // namespace routing
