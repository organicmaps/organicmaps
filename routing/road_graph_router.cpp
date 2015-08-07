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
#include "indexer/mwm_version.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

#include "geometry/distance.hpp"

#include "base/assert.hpp"

namespace routing
{

namespace
{
// TODO (@gorshenin, @pimenov, @ldragunov): kMaxRoadCandidates == 1
// means that only two closest feature will be examined when searching
// for features in the vicinity of start and final points.
// It is an oversimplification that is not as easily
// solved as tuning up this constant because if you set it too high
// you risk to find a feature that you cannot in fact reach because of
// an obstacle.  Using only the closest feature minimizes (but not
// eliminates) this risk.
size_t constexpr kMaxRoadCandidates = 1;
uint64_t constexpr kMinPedestrianMwmVersion = 150713;

IRouter::ResultCode Convert(IRoutingAlgorithm::Result value)
{
  switch (value)
  {
  case IRoutingAlgorithm::Result::OK: return IRouter::ResultCode::NoError;
  case IRoutingAlgorithm::Result::NoPath: return IRouter::ResultCode::RouteNotFound;
  case IRoutingAlgorithm::Result::Cancelled: return IRouter::ResultCode::Cancelled;
  }
  ASSERT(false, ("Unexpected IRoutingAlgorithm::Result value:", value));
  return IRouter::ResultCode::RouteNotFound;
}

void Convert(vector<Junction> const & path, vector<m2::PointD> & geometry)
{
  geometry.clear();
  geometry.reserve(path.size());
  for (auto const & pos : path)
    geometry.emplace_back(pos.GetPoint());
}

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
    if (version.timestamp < kMinPedestrianMwmVersion)
      mwmNames.push_back(mwmInfo->GetCountryName());
  }
  return !mwmNames.empty();
}
}  // namespace

RoadGraphRouter::~RoadGraphRouter() {}

RoadGraphRouter::RoadGraphRouter(string const & name, Index & index,
                                 unique_ptr<IVehicleModelFactory> && vehicleModelFactory,
                                 unique_ptr<IRoutingAlgorithm> && algorithm,
                                 unique_ptr<IDirectionsEngine> && directionsEngine)
    : m_name(name)
    , m_algorithm(move(algorithm))
    , m_roadGraph(make_unique<FeaturesRoadGraph>(index, move(vehicleModelFactory)))
    , m_directionsEngine(move(directionsEngine))
{
}

void RoadGraphRouter::ClearState()
{
  m_roadGraph->ClearState();
}

IRouter::ResultCode RoadGraphRouter::CalculateRoute(m2::PointD const & startPoint,
                                                    m2::PointD const & /* startDirection */,
                                                    m2::PointD const & finalPoint,
                                                    RouterDelegate const & delegate, Route & route)
{
  vector<pair<Edge, m2::PointD>> finalVicinity;
  m_roadGraph->FindClosestEdges(finalPoint, kMaxRoadCandidates, finalVicinity);
  
  if (finalVicinity.empty())
    return EndPointNotFound;

  //TODO (ldragunov) Remove this check after several releases. (Estimated in november)
  vector<string> mwmNames;
  if (CheckMwmVersion(finalVicinity, mwmNames))
  {
    for (auto const & name : mwmNames)
      route.AddAbsentCountry(name);
  }

  vector<pair<Edge, m2::PointD>> startVicinity;
  m_roadGraph->FindClosestEdges(startPoint, kMaxRoadCandidates, startVicinity);

  if (startVicinity.empty())
    return StartPointNotFound;

  //TODO (ldragunov) Remove this check after several releases. (Estimated in november)
  if (CheckMwmVersion(startVicinity, mwmNames))
  {
    for (auto const & name : mwmNames)
      route.AddAbsentCountry(name);
  }

  if (!route.GetAbsentCountries().empty())
    return FileTooOld;

  Junction const startPos(startPoint);
  Junction const finalPos(finalPoint);

  m_roadGraph->ResetFakes();
  m_roadGraph->AddFakeEdges(startPos, startVicinity);
  m_roadGraph->AddFakeEdges(finalPos, finalVicinity);

  vector<Junction> path;
  IRoutingAlgorithm::Result const resultCode =
      m_algorithm->CalculateRoute(*m_roadGraph, startPos, finalPos, delegate, path);

  if (resultCode == IRoutingAlgorithm::Result::OK)
  {
    ASSERT(!path.empty(), ());
    ASSERT_EQUAL(path.front(), startPos, ());
    ASSERT_EQUAL(path.back(), finalPos, ());
    ReconstructRoute(move(path), route, delegate);
  }

  m_roadGraph->ResetFakes();

  if (delegate.IsCancelled())
    return IRouter::Cancelled;

  return Convert(resultCode);
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

  vector<m2::PointD> geometry;
  Convert(path, geometry);

  Route::TTimes times;
  Route::TTurns turnsDir;
  turns::TTurnsGeom turnsGeom;
  if (m_directionsEngine)
    m_directionsEngine->Generate(*m_roadGraph, path, times, turnsDir, turnsGeom, cancellable);

  route.SetGeometry(geometry.begin(), geometry.end());
  route.SetSectionTimes(times);
  route.SetTurnInstructions(turnsDir);
  route.SetTurnInstructionsGeometry(turnsGeom);
}

unique_ptr<IRouter> CreatePedestrianAStarRouter(Index & index)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarRoutingAlgorithm());
  unique_ptr<IDirectionsEngine> directionsEngine(new PedestrianDirectionsEngine());
  unique_ptr<IRouter> router(new RoadGraphRouter("astar-pedestrian", index, move(vehicleModelFactory), move(algorithm), move(directionsEngine)));
  return router;
}

unique_ptr<IRouter> CreatePedestrianAStarBidirectionalRouter(Index & index)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarBidirectionalRoutingAlgorithm());
  unique_ptr<IDirectionsEngine> directionsEngine(new PedestrianDirectionsEngine());
  unique_ptr<IRouter> router(new RoadGraphRouter("astar-bidirectional-pedestrian", index, move(vehicleModelFactory), move(algorithm), move(directionsEngine)));
  return router;
}

}  // namespace routing
