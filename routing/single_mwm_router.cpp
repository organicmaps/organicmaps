#include "routing/single_mwm_router.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_progress.hpp"
#include "routing/bicycle_directions.hpp"
#include "routing/bicycle_model.hpp"
#include "routing/car_model.hpp"
#include "routing/index_graph.hpp"
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

#include "std/algorithm.hpp"

using namespace routing;

namespace
{
size_t constexpr kMaxRoadCandidates = 6;
float constexpr kProgressInterval = 2;
uint32_t constexpr kDrawPointsPeriod = 10;
}  // namespace

namespace routing
{
SingleMwmRouter::SingleMwmRouter(string const & name, Index const & index,
                                 traffic::TrafficCache const & trafficCache,
                                 shared_ptr<VehicleModelFactory> vehicleModelFactory,
                                 shared_ptr<EdgeEstimator> estimator,
                                 unique_ptr<IDirectionsEngine> directionsEngine)
  : m_name(name)
  , m_index(index)
  , m_trafficCache(trafficCache)
  , m_roadGraph(index, IRoadGraph::Mode::ObeyOnewayTag, vehicleModelFactory)
  , m_vehicleModelFactory(vehicleModelFactory)
  , m_estimator(estimator)
  , m_directionsEngine(move(directionsEngine))
{
  ASSERT(!m_name.empty(), ());
  ASSERT(m_vehicleModelFactory, ());
  ASSERT(m_estimator, ());
  ASSERT(m_directionsEngine, ());
}

IRouter::ResultCode SingleMwmRouter::CalculateRoute(MwmSet::MwmId const & mwmId,
                                                    m2::PointD const & startPoint,
                                                    m2::PointD const & startDirection,
                                                    m2::PointD const & finalPoint,
                                                    RouterDelegate const & delegate, Route & route)
{
  try
  {
    return DoCalculateRoute(mwmId, startPoint, startDirection, finalPoint, delegate, route);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't find path from", MercatorBounds::ToLatLon(startPoint), "to",
                 MercatorBounds::ToLatLon(finalPoint), ":\n ", e.what()));
    return IRouter::InternalError;
  }
}

IRouter::ResultCode SingleMwmRouter::DoCalculateRoute(MwmSet::MwmId const & mwmId,
                                                      m2::PointD const & startPoint,
                                                      m2::PointD const & /* startDirection */,
                                                      m2::PointD const & finalPoint,
                                                      RouterDelegate const & delegate,
                                                      Route & route)
{
  if (!mwmId.IsAlive())
    return IRouter::RouteFileNotExist;

  string const & country = mwmId.GetInfo()->GetCountryName();

  Edge startEdge;
  if (!FindClosestEdge(mwmId, startPoint, startEdge))
    return IRouter::StartPointNotFound;

  Edge finishEdge;
  if (!FindClosestEdge(mwmId, finalPoint, finishEdge))
    return IRouter::EndPointNotFound;

  IndexGraphStarter::FakeVertex const start(startEdge.GetFeatureId().m_index, startEdge.GetSegId(),
                                            startPoint);
  IndexGraphStarter::FakeVertex const finish(finishEdge.GetFeatureId().m_index,
                                             finishEdge.GetSegId(), finalPoint);

  EstimatorGuard guard(mwmId, *m_estimator);

  shared_ptr<IVehicleModel> vehicleModel =
      m_vehicleModelFactory->GetVehicleModelForCountry(country);
  IndexGraph graph(GeometryLoader::Create(m_index, mwmId, vehicleModel), m_estimator);

  if (!LoadIndex(mwmId, country, graph))
    return IRouter::RouteFileNotExist;

  IndexGraphStarter starter(graph, start, finish);

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
    if (!RedressRoute(mwmId, *vehicleModel, routingResult.path, delegate, starter, route))
      return IRouter::InternalError;
    if (delegate.IsCancelled())
      return IRouter::Cancelled;
    return IRouter::NoError;
  }
}

bool SingleMwmRouter::FindClosestEdge(MwmSet::MwmId const & mwmId, m2::PointD const & point,
                                      Edge & closestEdge) const
{
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

bool SingleMwmRouter::LoadIndex(MwmSet::MwmId const & mwmId, string const & country,
                                IndexGraph & graph)
{
  MwmSet::MwmHandle mwmHandle = m_index.GetMwmHandleById(mwmId);
  if (!mwmHandle.IsAlive())
    return false;

  MwmValue const * mwmValue = mwmHandle.GetValue<MwmValue>();
  try
  {
    my::Timer timer;
    FilesContainerR::TReader reader(mwmValue->m_cont.GetReader(ROUTING_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(reader);
    IndexGraphSerializer::Deserialize(graph, src, kCarMask);
    RestrictionLoader restrictionLoader(*mwmValue, graph);
    if (restrictionLoader.HasRestrictions())
      graph.SetRestrictions(restrictionLoader.StealRestrictions());

    LOG(LINFO,
        (ROUTING_FILE_TAG, "section for", country, "loaded in", timer.ElapsedSeconds(), "seconds"));
    return true;
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("File", mwmValue->GetCountryFileName(), "Error while reading", ROUTING_FILE_TAG,
                 "section:", e.Msg()));
    return false;
  }
}

bool SingleMwmRouter::RedressRoute(MwmSet::MwmId const & mwmId, IVehicleModel const & vehicleModel,
                                   vector<Segment> const & segments,
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

  shared_ptr<traffic::TrafficInfo::Coloring> trafficColoring = m_trafficCache.GetTrafficInfo(mwmId);
  IndexRoadGraph roadGraph(mwmId, m_index, vehicleModel.GetMaxSpeed(), starter, segments,
                           junctions);

  CHECK(m_directionsEngine, ());
  ReconstructRoute(*m_directionsEngine, roadGraph, trafficColoring, delegate, junctions, route);

  if (!route.IsValid())
  {
    LOG(LERROR, ("ReconstructRoute failed. Segments:", segments.size()));
    return false;
  }

  EdgeEstimator const & estimator = starter.GetGraph().GetEstimator();
  Geometry & geometry = starter.GetGraph().GetGeometry();
  Route::TTimes times;
  times.reserve(segments.size());
  double time = 0.0;
  times.emplace_back(static_cast<uint32_t>(0), 0.0);
  // First and last segments are fakes: skip it.
  for (size_t i = 1; i < segments.size() - 1; ++i)
  {
    times.emplace_back(static_cast<uint32_t>(i), time);
    Segment const & segment = segments[i];
    time += estimator.CalcSegmentWeight(segment, geometry.GetRoad(segment.GetFeatureId()));
  }
  route.SetSectionTimes(move(times));

  return true;
}

// static
unique_ptr<SingleMwmRouter> SingleMwmRouter::CreateCarRouter(
    Index const & index, traffic::TrafficCache const & trafficCache)
{
  auto vehicleModelFactory = make_shared<CarModelFactory>();
  // @TODO Bicycle turn generation engine is used now. It's ok for the time being.
  // But later a special car turn generation engine should be implemented.
  auto directionsEngine = make_unique<BicycleDirectionsEngine>(index);
  auto estimator =
      EdgeEstimator::CreateForCar(*vehicleModelFactory->GetVehicleModel(), trafficCache);
  auto router =
      make_unique<SingleMwmRouter>("astar-bidirectional-car", index, trafficCache,
                                   move(vehicleModelFactory), estimator, move(directionsEngine));
  return router;
}
}  // namespace routing
