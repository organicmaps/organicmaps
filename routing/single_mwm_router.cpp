#include "routing/single_mwm_router.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_progress.hpp"
#include "routing/bicycle_directions.hpp"
#include "routing/bicycle_model.hpp"
#include "routing/car_model.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/pedestrian_model.hpp"
#include "routing/restriction_loader.hpp"
#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/turns_generator.hpp"
#include "routing/vehicle_mask.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/distance.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/exception.hpp"

#include <algorithm>

using namespace routing;

namespace
{
size_t constexpr kMaxRoadCandidates = 6;
float constexpr kProgressInterval = 2;
uint32_t constexpr kDrawPointsPeriod = 10;
}  // namespace

namespace routing
{
SingleMwmRouter::SingleMwmRouter(string const & name, TCountryFileFn const & countryFileFn,
                                 shared_ptr<NumMwmIds> numMwmIds,
                                 shared_ptr<TrafficStash> trafficStash,
                                 shared_ptr<VehicleModelFactory> vehicleModelFactory,
                                 shared_ptr<EdgeEstimator> estimator,
                                 unique_ptr<IDirectionsEngine> directionsEngine, Index & index)
  : m_name(name)
  , m_index(index)
  , m_countryFileFn(countryFileFn)
  , m_numMwmIds(numMwmIds)
  , m_trafficStash(trafficStash)
  , m_indexManager(countryFileFn, m_index)
  , m_roadGraph(index, IRoadGraph::Mode::ObeyOnewayTag, vehicleModelFactory)
  , m_vehicleModelFactory(vehicleModelFactory)
  , m_estimator(estimator)
  , m_directionsEngine(move(directionsEngine))
{
  CHECK(!m_name.empty(), ());
  CHECK(m_numMwmIds, ());
  CHECK(m_trafficStash, ());
  CHECK(m_vehicleModelFactory, ());
  CHECK(m_estimator, ());
  CHECK(m_directionsEngine, ());
}

IRouter::ResultCode SingleMwmRouter::CalculateRoute(m2::PointD const & startPoint,
                                                    m2::PointD const & startDirection,
                                                    m2::PointD const & finalPoint,
                                                    RouterDelegate const & delegate, Route & route)
{
  try
  {
    return DoCalculateRoute(startPoint, startDirection, finalPoint, delegate, route);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't find path from", MercatorBounds::ToLatLon(startPoint), "to",
                 MercatorBounds::ToLatLon(finalPoint), ":\n ", e.what()));
    return IRouter::InternalError;
  }
}

IRouter::ResultCode SingleMwmRouter::DoCalculateRoute(m2::PointD const & startPoint,
                                                      m2::PointD const & /* startDirection */,
                                                      m2::PointD const & finalPoint,
                                                      RouterDelegate const & delegate,
                                                      Route & route)
{
  // TODO: remove field m_country.
  // Use m_countryFileFn(startPoint), m_countryFileFn(finalPoint) here.
  auto const startCountry = platform::CountryFile(m_country);
  auto const finishCountry = platform::CountryFile(m_country);

  Edge startEdge;
  if (!FindClosestEdge(startCountry, startPoint, startEdge))
    return IRouter::StartPointNotFound;

  Edge finishEdge;
  if (!FindClosestEdge(finishCountry, finalPoint, finishEdge))
    return IRouter::EndPointNotFound;

  IndexGraphStarter::FakeVertex const start(m_numMwmIds->GetId(startCountry),
                                            startEdge.GetFeatureId().m_index, startEdge.GetSegId(),
                                            startPoint);
  IndexGraphStarter::FakeVertex const finish(m_numMwmIds->GetId(finishCountry),
                                             finishEdge.GetFeatureId().m_index,
                                             finishEdge.GetSegId(), finalPoint);

  TrafficStash::Guard guard(*m_trafficStash);
  WorldGraph graph(make_unique<CrossMwmIndexGraph>(m_numMwmIds, m_indexManager),
                   IndexGraphLoader::Create(m_numMwmIds, m_vehicleModelFactory, m_estimator,
                                            m_trafficStash, m_index),
                   m_estimator);

  // TODO remove to activate CrossMwmGraph.
  graph.BlockMwmBorders();

  IndexGraphStarter starter(start, finish, graph);

  AStarProgress progress(0, 100);
  progress.Initialize(startPoint, finalPoint);

  uint32_t drawPointsStep = 0;
  auto onVisitJunction = [&](Segment const & from, Segment const & to) {
    m2::PointD const & pointFrom = starter.GetPoint(from, true /* front */);
    m2::PointD const & pointTo = starter.GetPoint(to, true /* front */);
    auto const lastValue = progress.GetLastValue();
    auto const newValue = progress.GetProgressForBidirectedAlgo(pointFrom, pointTo);
    if (newValue - lastValue > kProgressInterval)
      delegate.OnProgress(newValue);
    if (drawPointsStep % kDrawPointsPeriod == 0)
      delegate.OnPointCheck(pointFrom);
    ++drawPointsStep;
  };

  AStarAlgorithm<IndexGraphStarter> algorithm;

  RoutingResult<Segment> routingResult;
  auto const resultCode = algorithm.FindPathBidirectional(
      starter, starter.GetStart(), starter.GetFinish(), routingResult, delegate, onVisitJunction);

  switch (resultCode)
  {
  case AStarAlgorithm<IndexGraphStarter>::Result::NoPath: return IRouter::RouteNotFound;
  case AStarAlgorithm<IndexGraphStarter>::Result::Cancelled: return IRouter::Cancelled;
  case AStarAlgorithm<IndexGraphStarter>::Result::OK:
    vector<Segment> segments;
    IRouter::ResultCode const leapsResult =
        ProcessLeaps(routingResult.path, delegate, starter, segments);
    if (leapsResult != IRouter::NoError)
      return leapsResult;

    CHECK_GREATER_OR_EQUAL(segments.size(), routingResult.path.size(), ());

    if (!RedressRoute(segments, delegate, starter, route))
      return IRouter::InternalError;
    if (delegate.IsCancelled())
      return IRouter::Cancelled;
    return IRouter::NoError;
  }
}

bool SingleMwmRouter::FindClosestEdge(platform::CountryFile const & file, m2::PointD const & point,
                                      Edge & closestEdge) const
{
  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  auto const mwmId = MwmSet::MwmId(handle.GetInfo());

  vector<pair<Edge, Junction>> candidates;
  m_roadGraph.FindClosestEdges(point, kMaxRoadCandidates, candidates);

  double minDistance = numeric_limits<double>::max();
  size_t minIndex = candidates.size();

  for (size_t i = 0; i < candidates.size(); ++i)
  {
    Edge const & edge = candidates[i].first;
    if (edge.GetFeatureId().m_mwmId != mwmId)
      continue;

    m2::DistanceToLineSquare<m2::PointD> squaredDistance;
    squaredDistance.SetBounds(edge.GetStartJunction().GetPoint(), edge.GetEndJunction().GetPoint());
    double const distance = squaredDistance(point);
    if (distance < minDistance)
    {
      minDistance = distance;
      minIndex = i;
    }
  }

  if (minIndex == candidates.size())
    return false;

  closestEdge = candidates[minIndex].first;
  return true;
}

IRouter::ResultCode SingleMwmRouter::ProcessLeaps(vector<Segment> const & input,
                                                  RouterDelegate const & delegate,
                                                  IndexGraphStarter & starter,
                                                  vector<Segment> & output)
{
  starter.GetGraph().BlockMwmBorders();

  for (size_t i = 0; i < input.size(); ++i)
  {
    Segment const & current = input[i];
    if (starter.IsLeap(current.GetMwmId()))
    {
      ++i;
      CHECK_LESS(i, input.size(), ());
      Segment const & next = input[i];
      CHECK_EQUAL(current.GetMwmId(), next.GetMwmId(), ("i:", i));

      IndexGraphStarter::FakeVertex const start(current,
                                                starter.GetPoint(current, true /* front */));
      IndexGraphStarter::FakeVertex const finish(next, starter.GetPoint(next, true /* front */));

      IndexGraphStarter leapStarter(start, finish, starter.GetGraph());

      AStarAlgorithm<IndexGraphStarter> algorithm;
      RoutingResult<Segment> routingResult;
      auto const resultCode =
          algorithm.FindPathBidirectional(leapStarter, leapStarter.GetStart(),
                                          leapStarter.GetFinish(), routingResult, delegate, {});

      switch (resultCode)
      {
      case AStarAlgorithm<IndexGraphStarter>::Result::NoPath: return IRouter::RouteNotFound;
      case AStarAlgorithm<IndexGraphStarter>::Result::Cancelled: return IRouter::Cancelled;
      case AStarAlgorithm<IndexGraphStarter>::Result::OK:
        output.insert(output.end(), routingResult.path.begin(), routingResult.path.end());
      }
    }
    else
      output.push_back(current);
  }

  return IRouter::NoError;
}

bool SingleMwmRouter::RedressRoute(vector<Segment> const & segments,
                                   RouterDelegate const & delegate, IndexGraphStarter & starter,
                                   Route & route) const
{
  vector<Junction> junctions;
  size_t const numPoints = IndexGraphStarter::GetRouteNumPoints(segments);
  junctions.reserve(numPoints);

  for (size_t i = 0; i < numPoints; ++i)
  {
    // TODO: Use real altitudes for pedestrian and bicycle routing.
    junctions.emplace_back(starter.GetRoutePoint(segments, i), feature::kDefaultAltitudeMeters);
  }

  IndexRoadGraph roadGraph(m_numMwmIds, starter, segments, junctions, m_index);

  CHECK(m_directionsEngine, ());
  ReconstructRoute(*m_directionsEngine, roadGraph, m_trafficStash, delegate, junctions, route);

  if (!route.IsValid())
  {
    LOG(LERROR, ("ReconstructRoute failed. Segments:", segments.size()));
    return false;
  }

  Route::TTimes times;
  times.reserve(segments.size());
  double time = 0.0;
  times.emplace_back(static_cast<uint32_t>(0), 0.0);
  // First and last segments are fakes: skip it.
  for (size_t i = 1; i < segments.size() - 1; ++i)
  {
    times.emplace_back(static_cast<uint32_t>(i), time);
    time += starter.CalcSegmentWeight(segments[i]);
  }
  route.SetSectionTimes(move(times));

  return true;
}

// static
unique_ptr<SingleMwmRouter> SingleMwmRouter::CreateCarRouter(
    TCountryFileFn const & countryFileFn, shared_ptr<NumMwmIds> numMwmIds,
    traffic::TrafficCache const & trafficCache, Index & index)
{
  auto vehicleModelFactory = make_shared<CarModelFactory>();
  // @TODO Bicycle turn generation engine is used now. It's ok for the time being.
  // But later a special car turn generation engine should be implemented.
  auto directionsEngine = make_unique<BicycleDirectionsEngine>(index, numMwmIds);

  double maxSpeed = 0.0;
  numMwmIds->ForEachId([&](NumMwmId id) {
    string const & country = numMwmIds->GetFile(id).GetName();
    double const mwmMaxSpeed =
        vehicleModelFactory->GetVehicleModelForCountry(country)->GetMaxSpeed();
    maxSpeed = max(maxSpeed, mwmMaxSpeed);
  });

  auto trafficStash = make_shared<TrafficStash>(trafficCache);

  auto estimator = EdgeEstimator::CreateForCar(trafficStash, maxSpeed);
  auto router = make_unique<SingleMwmRouter>("astar-bidirectional-car", countryFileFn, numMwmIds,
                                             trafficStash, vehicleModelFactory, estimator,
                                             move(directionsEngine), index);
  return router;
}
}  // namespace routing
