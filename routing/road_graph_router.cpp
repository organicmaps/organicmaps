#include "routing/bicycle_directions.hpp"
#include "routing/bicycle_model.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/nearest_edge_finder.hpp"
#include "routing/pedestrian_directions.hpp"
#include "routing/pedestrian_model.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"

#include "coding/reader_wrapper.hpp"

#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"

#include "geometry/distance.hpp"

#include "std/queue.hpp"
#include "std/set.hpp"

#include "base/assert.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace routing
{

namespace
{
size_t constexpr kMaxRoadCandidates = 6;

size_t constexpr kTestConnectivityVisitJunctionsLimit = 25;

uint64_t constexpr kMinPedestrianMwmVersion = 150713;

// Check if the found edges lays on mwm with pedestrian routing support.
bool CheckMwmVersion(vector<pair<Edge, m2::PointD>> const & vicinities, vector<string> & mwmNames)
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

// Checks is edge connected with world graph.
// Function does BFS while it finds some number of edges, if graph ends
// before this number is reached then junction is assumed as not connected to the world graph.
bool CheckGraphConnectivity(IRoadGraph const & graph, Junction const & junction, size_t limit)
{
  queue<Junction> q;
  q.push(junction);

  set<Junction> visited;

  vector<Edge> edges;
  while (!q.empty())
  {
    Junction const node = q.front();
    q.pop();

    if (visited.find(node) != visited.end())
      continue;
    visited.insert(node);

    if (limit == visited.size())
      return true;

    edges.clear();
    graph.GetOutgoingEdges(node, edges);
    for (Edge const & e : edges)
      q.push(e.GetEndJunction());
  }

  return false;
}

// Find closest candidates in the world graph
void FindClosestEdges(IRoadGraph const & graph, m2::PointD const & point,
                      vector<pair<Edge, m2::PointD>> & vicinity)
{
  // WARNING: Take only one vicinity
  // It is an oversimplification that is not as easily
  // solved as tuning up this constant because if you set it too high
  // you risk to find a feature that you cannot in fact reach because of
  // an obstacle.  Using only the closest feature minimizes (but not
  // eliminates) this risk.

  vector<pair<Edge, m2::PointD>> candidates;
  graph.FindClosestEdges(point, kMaxRoadCandidates, candidates);

  vicinity.clear();
  for (auto const & candidate : candidates)
  {
    if (CheckGraphConnectivity(graph, candidate.first.GetStartJunction(), kTestConnectivityVisitJunctionsLimit))
    {
      vicinity.emplace_back(candidate);
      break;
    }
  }
}
}  // namespace

RoadGraphRouter::~RoadGraphRouter() {}

RoadGraphRouter::RoadGraphRouter(string const & name, Index const & index,
                                 TCountryFileFn const & countryFileFn, IRoadGraph::Mode mode,
                                 unique_ptr<IVehicleModelFactory> && vehicleModelFactory,
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

IRouter::ResultCode RoadGraphRouter::CalculateRoute(m2::PointD const & startPoint,
                                                    m2::PointD const & /* startDirection */,
                                                    m2::PointD const & finalPoint,
                                                    RouterDelegate const & delegate, Route & route)
{
  if (!CheckMapExistence(startPoint, route) || !CheckMapExistence(finalPoint, route))
    return IRouter::RouteFileNotExist;

  vector<pair<Edge, m2::PointD>> finalVicinity;
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

  vector<pair<Edge, m2::PointD>> startVicinity;
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

  Junction const startPos(startPoint);
  Junction const finalPos(finalPoint);

  m_roadGraph->ResetFakes();
  m_roadGraph->AddFakeEdges(startPos, startVicinity);
  m_roadGraph->AddFakeEdges(finalPos, finalVicinity);

  RoutingResult<Junction> result;
  IRoutingAlgorithm::Result const resultCode =
      m_algorithm->CalculateRoute(*m_roadGraph, startPos, finalPos, delegate, result);

  if (resultCode == IRoutingAlgorithm::Result::OK)
  {
    ASSERT(!result.path.empty(), ());
    ASSERT_EQUAL(result.path.front(), startPos, ());
    ASSERT_EQUAL(result.path.back(), finalPos, ());
    ASSERT_GREATER(result.distance, 0., ());
    ReconstructRoute(move(result.path), route, delegate);
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

void RoadGraphRouter::ReconstructRoute(vector<Junction> && path, Route & route,
                                       my::Cancellable const & cancellable) const
{
  CHECK(!path.empty(), ("Can't reconstruct route from an empty list of positions."));

  // By some reason there're two adjacent positions on a road with
  // the same end-points. This could happen, for example, when
  // direction on a road was changed.  But it doesn't matter since
  // this code reconstructs only geometry of a route.
  path.erase(unique(path.begin(), path.end()), path.end());
  if (path.size() == 1)
    path.emplace_back(path.back());

  Route::TTimes times;
  Route::TTurns turnsDir;
  vector<m2::PointD> geometry;
  // @TODO(bykoianko) streetNames is not filled in Generate(). It should be done.
  Route::TStreets streetNames;
  if (m_directionsEngine)
    m_directionsEngine->Generate(*m_roadGraph, path, times, turnsDir, geometry, cancellable);

  route.SetGeometry(geometry.begin(), geometry.end());
  route.SetSectionTimes(times);
  route.SetTurnInstructions(turnsDir);
  route.SetStreetNames(streetNames);
}

unique_ptr<IRouter> CreatePedestrianAStarRouter(Index & index, TCountryFileFn const & countryFileFn)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarRoutingAlgorithm());
  unique_ptr<IDirectionsEngine> directionsEngine(new PedestrianDirectionsEngine());
  unique_ptr<IRouter> router(new RoadGraphRouter(
      "astar-pedestrian", index, countryFileFn, IRoadGraph::Mode::IgnoreOnewayTag,
      move(vehicleModelFactory), move(algorithm), move(directionsEngine)));
  return router;
}

unique_ptr<IRouter> CreatePedestrianAStarBidirectionalRouter(Index & index, TCountryFileFn const & countryFileFn)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarBidirectionalRoutingAlgorithm());
  unique_ptr<IDirectionsEngine> directionsEngine(new PedestrianDirectionsEngine());
  unique_ptr<IRouter> router(new RoadGraphRouter(
      "astar-bidirectional-pedestrian", index, countryFileFn, IRoadGraph::Mode::IgnoreOnewayTag,
      move(vehicleModelFactory), move(algorithm), move(directionsEngine)));
  return router;
}

unique_ptr<IRouter> CreateBicycleAStarBidirectionalRouter(Index & index, TCountryFileFn const & countryFileFn)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new BicycleModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarBidirectionalRoutingAlgorithm());
  unique_ptr<IDirectionsEngine> directionsEngine(new BicycleDirectionsEngine(index));
  unique_ptr<IRouter> router(new RoadGraphRouter(
      "astar-bidirectional-bicycle", index, countryFileFn, IRoadGraph::Mode::ObeyOnewayTag,
      move(vehicleModelFactory), move(algorithm), move(directionsEngine)));
  return router;
}
}  // namespace routing
