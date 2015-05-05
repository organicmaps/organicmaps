#include "routing/online_cross_fetcher.hpp"
#include "routing/osrm_router.hpp"
#include "routing/turns_generator.hpp"
#include "routing/vehicle_model.hpp"

#include "platform/platform.hpp"

#include "geometry/angles.hpp"
#include "geometry/distance.hpp"
#include "geometry/distance_on_sphere.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/mercator.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_string_utils.hpp"

#include "coding/reader_wrapper.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"

#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"
#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"
#include "3party/osrm/osrm-backend/data_structures/search_engine_data.hpp"
#include "3party/osrm/osrm-backend/descriptors/description_factory.hpp"
#include "3party/osrm/osrm-backend/routing_algorithms/shortest_path.hpp"
#include "3party/osrm/osrm-backend/routing_algorithms/n_to_m_many_to_many.hpp"

#define INTERRUPT_WHEN_CANCELLED() \
  do                               \
  {                                \
    if (IsCancelled())             \
      return Cancelled;            \
  } while (false)

namespace routing
{

size_t const MAX_NODE_CANDIDATES = 10;
double const FEATURE_BY_POINT_RADIUS_M = 1000.0;
double const TIME_OVERHEAD = 1.4;
double const FEATURES_NEAR_TURN_M = 3.0;

// TODO (ldragunov) Switch all RawRouteData and incapsulate to own omim types.
using RawRouteData = InternalRouteResult;

struct RawRoutingResultT
{
  RawRouteData m_routePath;
  FeatureGraphNode m_sourceEdge;
  FeatureGraphNode m_targetEdge;
};

namespace
{
class AbsentCountryChecker
{
public:
  AbsentCountryChecker(m2::PointD const & startPoint, m2::PointD const & finalPoint,
                       RoutingIndexManager & indexManager, Index const * index, Route & route)
      : m_route(route),
        m_index(index),
        m_indexManager(indexManager),
        m_fetcher(OSRM_ONLINE_SERVER_URL,
                  {MercatorBounds::XToLon(startPoint.x), MercatorBounds::YToLat(startPoint.y)},
                  {MercatorBounds::XToLon(finalPoint.x), MercatorBounds::YToLat(finalPoint.y)})
  {
  }

  ~AbsentCountryChecker()
  {
    vector<m2::PointD> const & points = m_fetcher.GetMwmPoints();
    for (m2::PointD const & point : points)
    {
      RoutingMappingPtrT mapping = m_indexManager.GetMappingByPoint(point, m_index);
      if (!mapping->IsValid())
      {
        LOG(LINFO, ("Online recomends to download: ", mapping->GetName()));
        m_route.AddAbsentCountry(mapping->GetName());
      }
    }
  }

private:
  Route & m_route;
  Index const * m_index;
  RoutingIndexManager & m_indexManager;
  OnlineCrossFetcher m_fetcher;
};

class Point2Geometry : private noncopyable
{
  m2::PointD m_p, m_p1;
  OsrmRouter::GeomTurnCandidateT & m_candidates;
public:
  Point2Geometry(m2::PointD const & p, m2::PointD const & p1,
                 OsrmRouter::GeomTurnCandidateT & candidates)
    : m_p(p), m_p1(p1), m_candidates(candidates)
  {
  }

  void operator() (FeatureType const & ft)
  {
    static CarModel const carModel;
    if (ft.GetFeatureType() != feature::GEOM_LINE || !carModel.IsRoad(ft))
      return;
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const count = ft.GetPointsCount();
    ASSERT_GREATER(count, 1, ());

    auto addAngle = [&](m2::PointD const & p, m2::PointD const & p1, m2::PointD const & p2)
    {
      double const a = my::RadToDeg(ang::TwoVectorsAngle(p, p1, p2));
      if (!my::AlmostEqual(a, 0.))
        m_candidates.push_back(a);
    };

    for (size_t i = 0; i < count; ++i)
    {
      if (MercatorBounds::DistanceOnEarth(m_p, ft.GetPoint(i)) < FEATURES_NEAR_TURN_M)
      {
        if (i > 0)
          addAngle(m_p, m_p1, ft.GetPoint(i - 1));
        if (i < count - 1)
          addAngle(m_p, m_p1, ft.GetPoint(i + 1));
        return;
      }
    }
  }
};

class Point2PhantomNode : private noncopyable
{
  struct Candidate
  {
    double m_dist;
    uint32_t m_segIdx;
    uint32_t m_fid;
    m2::PointD m_point;

    Candidate() : m_dist(numeric_limits<double>::max()), m_fid(OsrmMappingTypes::FtSeg::INVALID_FID) {}
  };

  m2::PointD m_point;
  m2::PointD const m_direction;
  OsrmFtSegMapping const & m_mapping;
  buffer_vector<Candidate, 128> m_candidates;
  MwmSet::MwmId m_mwmId;
  Index const * m_pIndex;

public:
  Point2PhantomNode(OsrmFtSegMapping const & mapping, Index const * pIndex,
                    m2::PointD const & direction)
      : m_direction(direction), m_mapping(mapping), m_pIndex(pIndex)
  {
  }

  void SetPoint(m2::PointD const & pt)
  {
    m_point = pt;
  }

  bool HasCandidates() const
  {
    return !m_candidates.empty();
  }

  void operator() (FeatureType const & ft)
  {
    static CarModel const carModel;
    if (ft.GetFeatureType() != feature::GEOM_LINE || !carModel.IsRoad(ft))
      return;

    Candidate res;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t const count = ft.GetPointsCount();
    ASSERT_GREATER(count, 1, ());
    for (size_t i = 1; i < count; ++i)
    {
      /// @todo Probably, we need to get exact projection distance in meters.
      m2::ProjectionToSection<m2::PointD> segProj;
      segProj.SetBounds(ft.GetPoint(i - 1), ft.GetPoint(i));

      m2::PointD const pt = segProj(m_point);
      double const d = m_point.SquareLength(pt);
      if (d < res.m_dist)
      {
        res.m_dist = d;
        res.m_fid = ft.GetID().m_offset;
        res.m_segIdx = i - 1;
        res.m_point = pt;

        if (!m_mwmId.IsAlive())
          m_mwmId = ft.GetID().m_mwmId;
        ASSERT_EQUAL(m_mwmId, ft.GetID().m_mwmId, ());
      }
    }

    if (res.m_fid != OsrmMappingTypes::FtSeg::INVALID_FID)
      m_candidates.push_back(res);
  }

  double CalculateDistance(OsrmMappingTypes::FtSeg const & s) const
  {
    ASSERT_NOT_EQUAL(s.m_pointStart, s.m_pointEnd, ());

    Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmId);
    FeatureType ft;
    loader.GetFeature(s.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    double dist = 0.0;
    size_t n = max(s.m_pointEnd, s.m_pointStart);
    size_t i = min(s.m_pointStart, s.m_pointEnd) + 1;
    do
    {
      dist += MercatorBounds::DistanceOnEarth(ft.GetPoint(i - 1), ft.GetPoint(i));
      ++i;
    } while (i <= n);

    return dist;
  }

  void CalculateOffset(OsrmMappingTypes::FtSeg const & seg, m2::PointD const & segPt, NodeID & nodeId, int & offset, bool forward) const
  {
    if (nodeId == INVALID_NODE_ID)
      return;

    double distance = 0;
    auto const range = m_mapping.GetSegmentsRange(nodeId);
    OsrmMappingTypes::FtSeg s, cSeg;

    int si = forward ? range.second - 1 : range.first;
    int ei = forward ? range.first - 1 : range.second;
    int di = forward ? -1 : 1;

    for (int i = si; i != ei; i += di)
    {
      m_mapping.GetSegmentByIndex(i, s);
      if (!s.IsValid())
        continue;

      auto s1 = min(s.m_pointStart, s.m_pointEnd);
      auto e1 = max(s.m_pointEnd, s.m_pointStart);

      // seg.m_pointEnd - seg.m_pointStart == 1, so check
      // just a case, when seg is inside s
      if ((seg.m_pointStart != s1 || seg.m_pointEnd != e1) &&
          (s1 <= seg.m_pointStart && e1 >= seg.m_pointEnd))
      {
        cSeg.m_fid = s.m_fid;

        if (s.m_pointStart < s.m_pointEnd)
        {
          if (forward)
          {
            cSeg.m_pointEnd = seg.m_pointEnd;
            cSeg.m_pointStart = s.m_pointStart;
          }
          else
          {
            cSeg.m_pointStart = seg.m_pointStart;
            cSeg.m_pointEnd = s.m_pointEnd;
          }
        }
        else
        {
          if (forward)
          {
            cSeg.m_pointStart = s.m_pointEnd;
            cSeg.m_pointEnd = seg.m_pointEnd;
          }
          else
          {
            cSeg.m_pointEnd = seg.m_pointStart;
            cSeg.m_pointStart = s.m_pointStart;
          }
        }

        distance += CalculateDistance(cSeg);
        break;
      }
      else
        distance += CalculateDistance(s);
    }

    Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmId);
    FeatureType ft;
    loader.GetFeature(seg.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    // node.m_seg always forward ordered (m_pointStart < m_pointEnd)
    distance -= MercatorBounds::DistanceOnEarth(ft.GetPoint(forward ? seg.m_pointEnd : seg.m_pointStart), segPt);

    offset = max(static_cast<int>(distance), 1);
  }

  void CalculateOffsets(FeatureGraphNode & node) const
  {
    CalculateOffset(node.m_seg, node.m_segPt, node.m_node.forward_node_id, node.m_node.forward_offset, true);
    CalculateOffset(node.m_seg, node.m_segPt, node.m_node.reverse_node_id, node.m_node.reverse_offset, false);

    // need to initialize weights for correct work of PhantomNode::GetForwardWeightPlusOffset
    // and PhantomNode::GetReverseWeightPlusOffset
    node.m_node.forward_weight = 0;
    node.m_node.reverse_weight = 0;
  }

  void MakeResult(FeatureGraphNodeVecT & res, size_t maxCount)
  {
    if (!m_mwmId.IsAlive())
      return;

    vector<OsrmMappingTypes::FtSeg> segments;

    segments.resize(maxCount);

    OsrmFtSegMapping::FtSegSetT segmentSet;
    sort(m_candidates.begin(), m_candidates.end(), [] (Candidate const & r1, Candidate const & r2)
    {
      return (r1.m_dist < r2.m_dist);
    });

    size_t const n = min(m_candidates.size(), maxCount);
    for (size_t j = 0; j < n; ++j)
    {
      OsrmMappingTypes::FtSeg & seg = segments[j];
      Candidate const & c = m_candidates[j];

      seg.m_fid = c.m_fid;
      seg.m_pointStart = c.m_segIdx;
      seg.m_pointEnd = c.m_segIdx + 1;

      segmentSet.insert(&seg);
    }

    OsrmFtSegMapping::OsrmNodesT nodes;
    m_mapping.GetOsrmNodes(segmentSet, nodes);

    res.clear();
    res.resize(maxCount);

    for (size_t j = 0; j < maxCount; ++j)
    {
      size_t const idx = j;

      if (!segments[idx].IsValid())
        continue;

      auto it = nodes.find(segments[idx].Store());
      if (it == nodes.end())
        continue;

      FeatureGraphNode & node = res[idx];

      if (!m_direction.IsAlmostZero())
      {
        // Filter income nodes by direction mode
        OsrmMappingTypes::FtSeg const & node_seg = segments[idx];
        FeatureType feature;
        Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmId);
        loader.GetFeature(node_seg.m_fid, feature);
        feature.ParseGeometry(FeatureType::BEST_GEOMETRY);
        m2::PointD const featureDirection = feature.GetPoint(node_seg.m_pointEnd) - feature.GetPoint(node_seg.m_pointStart);
        bool const sameDirection = (m2::DotProduct(featureDirection, m_direction) / (featureDirection.Length() * m_direction.Length()) > 0);
        if (sameDirection)
        {
          node.m_node.forward_node_id = it->second.first;
          node.m_node.reverse_node_id = INVALID_NODE_ID;
        }
        else
        {
          node.m_node.forward_node_id = INVALID_NODE_ID;
          node.m_node.reverse_node_id = it->second.second;
        }
      }
      else
      {
        node.m_node.forward_node_id = it->second.first;
        node.m_node.reverse_node_id = it->second.second;
      }

      node.m_seg = segments[idx];
      node.m_segPt = m_candidates[j].m_point;

      CalculateOffsets(node);
    }

  }
};

size_t GetLastSegmentPointIndex(pair<size_t, size_t> const & p)
{
  ASSERT_GREATER(p.second, 0, ());
  return p.second - 1;
}
} // namespace

RoutingMappingPtrT RoutingIndexManager::GetMappingByPoint(m2::PointD const & point, Index const * pIndex)
{
  return GetMappingByName(m_countryFn(point), pIndex);
}

RoutingMappingPtrT RoutingIndexManager::GetMappingByName(string const & fName, Index const * pIndex)
{
  // Check if we have already load this file
  auto mapIter = m_mapping.find(fName);
  if (mapIter != m_mapping.end())
    return mapIter->second;

  // Or load and check file
  RoutingMappingPtrT new_mapping = make_shared<RoutingMapping>(fName, pIndex);
  m_mapping.insert(make_pair(fName, new_mapping));
  return new_mapping;
}

OsrmRouter::OsrmRouter(Index const * index, CountryFileFnT const & fn, RoutingVisualizerFn /* routingVisualization */)
    : m_pIndex(index), m_indexManager(fn)
{
}

string OsrmRouter::GetName() const
{
  return "vehicle";
}

void OsrmRouter::ClearState()
{
  m_cachedFinalNodes.clear();
  m_indexManager.Clear();
}

namespace
{

bool IsRouteExist(RawRouteData const & r)
{
  return !(INVALID_EDGE_WEIGHT == r.shortest_path_length ||
          r.segment_end_coordinates.empty() ||
          r.source_traversed_in_reverse.empty());
}

bool IsValidEdgeWeight(EdgeWeight const & w) { return w != INVALID_EDGE_WEIGHT; }

}

void OsrmRouter::FindWeightsMatrix(MultiroutingTaskPointT const & sources, MultiroutingTaskPointT const & targets,
                                    RawDataFacadeT &facade, vector<EdgeWeight> &result)
{
  SearchEngineData engineData;
  NMManyToManyRouting<RawDataFacadeT> pathFinder(&facade, engineData);

  PhantomNodeArray sourcesTaskVector(sources.size());
  PhantomNodeArray targetsTaskVector(targets.size());
  for (int i = 0; i < sources.size(); ++i)
  {
    sourcesTaskVector[i].push_back(sources[i].m_node);
  }
  for (int i = 0; i < targets.size(); ++i)
  {
    targetsTaskVector[i].push_back(targets[i].m_node);
  }

  // Calculate time consumption of a NtoM path finding.
  my::HighResTimer timer(true);
  shared_ptr<vector<EdgeWeight>> resultTable = pathFinder(sourcesTaskVector, targetsTaskVector);
  LOG(LINFO, ("Duration of a single one-to-many routing call", timer.ElapsedNano(), "ns"));
  timer.Reset();
  ASSERT_EQUAL(resultTable->size(), sources.size() * targets.size(), ());
  result.swap(*resultTable);
}

bool OsrmRouter::FindSingleRoute(FeatureGraphNodeVecT const & source, FeatureGraphNodeVecT const & target, DataFacadeT & facade,
                                 RawRoutingResultT & rawRoutingResult)
{
  /// @todo: make more complex nearest edge turnaround
  SearchEngineData engineData;
  ShortestPathRouting<DataFacadeT> pathFinder(&facade, engineData);

  for (auto targetEdge = target.cbegin(); targetEdge != target.cend(); ++targetEdge)
  {
    for (auto sourceEdge = source.cbegin(); sourceEdge != source.cend(); ++sourceEdge)
    {
      PhantomNodes nodes;
      nodes.source_phantom = sourceEdge->m_node;
      nodes.target_phantom = targetEdge->m_node;

      rawRoutingResult.m_routePath = RawRouteData();

      if ((nodes.source_phantom.forward_node_id != INVALID_NODE_ID ||
           nodes.source_phantom.reverse_node_id != INVALID_NODE_ID) &&
          (nodes.target_phantom.forward_node_id != INVALID_NODE_ID ||
           nodes.target_phantom.reverse_node_id != INVALID_NODE_ID))
      {
        rawRoutingResult.m_routePath.segment_end_coordinates.push_back(nodes);

        pathFinder({nodes}, {}, rawRoutingResult.m_routePath);
      }


      if (IsRouteExist(rawRoutingResult.m_routePath))
      {
        rawRoutingResult.m_sourceEdge = *sourceEdge;
        rawRoutingResult.m_targetEdge = *targetEdge;
        return true;
      }
    }
  }
  return false;
}

void OsrmRouter::GenerateRoutingTaskFromNodeId(NodeID const nodeId, bool const isStartNode,
                                               FeatureGraphNode & taskNode)
{
  taskNode.m_node.forward_node_id = isStartNode ? nodeId : INVALID_NODE_ID;
  taskNode.m_node.reverse_node_id = isStartNode ? INVALID_NODE_ID : nodeId;
  taskNode.m_node.forward_weight = 0;
  taskNode.m_node.reverse_weight = 0;
  taskNode.m_node.forward_offset = 0;
  taskNode.m_node.reverse_offset = 0;
  taskNode.m_node.name_id = 1;
  taskNode.m_seg.m_fid = OsrmMappingTypes::FtSeg::INVALID_FID;
}

size_t OsrmRouter::FindNextMwmNode(OutgoingCrossNode const & startNode, RoutingMappingPtrT const & targetMapping)
{
  m2::PointD startPoint = startNode.m_point;

  auto income_iters = targetMapping->m_crossContext.GetIngoingIterators();
  for (auto i = income_iters.first; i < income_iters.second; ++i)
  {
    m2::PointD targetPoint = i->m_point;
    if (ms::DistanceOnEarth(startPoint.y, startPoint.x, targetPoint.y, targetPoint.x) < FEATURE_BY_POINT_RADIUS_M)
      return i->m_nodeId;
  }
  return INVALID_NODE_ID;
}

OsrmRouter::ResultCode OsrmRouter::MakeRouteFromCrossesPath(CheckedPathT const & path,
                                                            Route & route)
{
  Route::TurnsT TurnsDir;
  Route::TimesT Times;
  vector<m2::PointD> Points;
  turns::TurnsGeomT TurnsGeom;
  for (RoutePathCross const & cross: path)
  {
    RawRoutingResultT routingResult;
    // TODO (Dragunov) refactor whole routing to single OSRM node in task instead vector.
    FeatureGraphNodeVecT startTask(1), targetTask(1);

    startTask[0] = cross.startNode;
    if (!cross.startNode.m_seg.IsValid())
      startTask[0].m_node.reverse_node_id = INVALID_NODE_ID;

    targetTask[0] = cross.targetNode;
    if (!cross.targetNode.m_seg.IsValid())
      targetTask[0].m_node.forward_node_id = INVALID_NODE_ID;
    RoutingMappingPtrT mwmMapping;
    mwmMapping = m_indexManager.GetMappingByName(cross.mwmName, m_pIndex);
    ASSERT(mwmMapping->IsValid(), ());
    MappingGuard mwmMappingGuard(mwmMapping);
    UNUSED_VALUE(mwmMappingGuard);
    if (!FindSingleRoute(startTask, targetTask, mwmMapping->m_dataFacade, routingResult))
    {
      return OsrmRouter::RouteNotFound;
    }

    // Get annotated route
    Route::TurnsT mwmTurnsDir;
    Route::TimesT mwmTimes;
    vector<m2::PointD> mwmPoints;
    turns::TurnsGeomT mwmTurnsGeom;
    MakeTurnAnnotation(routingResult, mwmMapping, mwmPoints, mwmTurnsDir, mwmTimes, mwmTurnsGeom);

    // And connect it to result route
    for (auto turn : mwmTurnsDir)
    {
      turn.m_index += Points.size() - 1;
      TurnsDir.push_back(turn);
    }


    double const estimationTime = Times.size() ? Times.back().second : 0.0;
    for (auto time : mwmTimes)
    {
      time.first += Points.size() - 1;
      time.second += estimationTime;
      Times.push_back(time);
    }


    if (!Points.empty())
    {
      // We're at the end point
      Points.pop_back();
      // -1 because --mwmPoints.begin()
      const size_t psize = Points.size() - 1;
      for (auto & turnGeom : mwmTurnsGeom)
        turnGeom.m_indexInRoute += psize;
    }
    Points.insert(Points.end(), ++mwmPoints.begin(), mwmPoints.end());
    TurnsGeom.insert(TurnsGeom.end(), mwmTurnsGeom.begin(), mwmTurnsGeom.end());
  }

  route.SetGeometry(Points.begin(), Points.end());
  route.SetTurnInstructions(TurnsDir);
  route.SetSectionTimes(Times);
  route.SetTurnInstructionsGeometry(TurnsGeom);
  return OsrmRouter::NoError;
}

class OsrmRouter::LastCrossFinder
{
  CrossRoutingContextReader const & m_targetContext;
  string const m_mwmName;
  MultiroutingTaskPointT m_sources;
  FeatureGraphNode m_target;
  vector<EdgeWeight> m_weights;

public:
  LastCrossFinder(RoutingMappingPtrT & mapping, FeatureGraphNodeVecT const & targetTask)
    : m_targetContext(mapping->m_crossContext),
      m_mwmName(mapping->GetName())
  {
    auto income_iterators = m_targetContext.GetIngoingIterators();
    MultiroutingTaskPointT targets(1);
    m_sources.resize(distance(income_iterators.first, income_iterators.second));

    size_t index = 0;
    for (auto i = income_iterators.first; i < income_iterators.second; ++i, ++index)
      OsrmRouter::GenerateRoutingTaskFromNodeId(i->m_nodeId, true /*isStartNode*/, m_sources[index]);

    vector<EdgeWeight> weights;
    for (auto const & t : targetTask)
    {
      targets[0] = t;
      OsrmRouter::FindWeightsMatrix(m_sources, targets, mapping->m_dataFacade, weights);
      if (find_if(weights.begin(), weights.end(), &IsValidEdgeWeight) != weights.end())
      {
        ASSERT_EQUAL(weights.size(), m_sources.size(), ());
        m_target = t;
        m_weights.swap(weights);
        break;
      }
    }
  }

  OsrmRouter::ResultCode GetError() const
  {
    if (m_sources.empty())
      return OsrmRouter::RouteFileNotExist;
    if (m_weights.empty())
      return OsrmRouter::EndPointNotFound;
    return OsrmRouter::NoError;
  }

  bool MakeLastCrossSegment(size_t const incomeNodeId, OsrmRouter::RoutePathCross & outCrossTask)
  {
    auto const ingoing = m_targetContext.GetIngoingIterators();
    auto const it = find_if(ingoing.first, ingoing.second, [&incomeNodeId](IngoingCrossNode const & node)
                      {
      return node.m_nodeId == incomeNodeId;
    });
    ASSERT(it != ingoing.second, ());
    size_t const targetNumber = distance(ingoing.first, it);
    EdgeWeight const & targetWeight = m_weights[targetNumber];
    outCrossTask = {m_mwmName, m_sources[targetNumber], m_target, targetWeight};
    return targetWeight != INVALID_EDGE_WEIGHT;
  }
};

OsrmRouter::ResultCode OsrmRouter::CalculateRoute(m2::PointD const & startPoint,
                                                  m2::PointD const & startDirection,
                                                  m2::PointD const & finalPoint, Route & route)
{
// Experimental feature
#if defined(DEBUG)
  AbsentCountryChecker checker(startPoint, finalPoint, m_indexManager, m_pIndex, route);
  UNUSED_VALUE(checker);
#endif
  my::HighResTimer timer(true);
  RoutingMappingPtrT startMapping = m_indexManager.GetMappingByPoint(startPoint, m_pIndex);
  RoutingMappingPtrT targetMapping = m_indexManager.GetMappingByPoint(finalPoint, m_pIndex);

  m_indexManager.Clear();  // TODO (Dragunov) make proper index manager cleaning

  if (!startMapping->IsValid())
  {
    route.AddAbsentCountry(startMapping->GetName());
    return startMapping->GetError();
  }
  if (!targetMapping->IsValid())
  {
    // Check if target is a neighbour
    startMapping->LoadCrossContext();
    auto out_iterators = startMapping->m_crossContext.GetOutgoingIterators();
    for (auto i = out_iterators.first; i != out_iterators.second; ++i)
      if (startMapping->m_crossContext.getOutgoingMwmName(i->m_outgoingIndex) == targetMapping->GetName())
      {
        route.AddAbsentCountry(targetMapping->GetName());
        return targetMapping->GetError();
      }
    return targetMapping->GetError();
  }

  MappingGuard startMappingGuard(startMapping);
  MappingGuard finalMappingGuard(targetMapping);
  UNUSED_VALUE(startMappingGuard);
  UNUSED_VALUE(finalMappingGuard);
  LOG(LINFO, ("Duration of the MWM loading", timer.ElapsedNano()));
  timer.Reset();

  // 3. Find start/end nodes.
  FeatureGraphNodeVecT startTask;

  {
    ResultCode const code = FindPhantomNodes(startMapping->GetName(), startPoint, startDirection,
                                             startTask, MAX_NODE_CANDIDATES, startMapping);
    if (code != NoError)
      return code;
  }
  {
    if (finalPoint != m_CachedTargetPoint)
    {
      ResultCode const code =
          FindPhantomNodes(targetMapping->GetName(), finalPoint, m2::PointD::Zero(),
                           m_CachedTargetTask, MAX_NODE_CANDIDATES, targetMapping);
      if (code != NoError)
        return code;
      m_CachedTargetPoint = finalPoint;
    }
  }
  INTERRUPT_WHEN_CANCELLED();

  LOG(LINFO, ("Duration of the start/stop points lookup", timer.ElapsedNano()));
  timer.Reset();

  // 4. Find route.
  RawRoutingResultT routingResult;

  // 4.1 Single mwm case
  if (startMapping->GetName() == targetMapping->GetName())
  {
    LOG(LINFO, ("Single mwm routing case"));
    m_indexManager.ForEachMapping([](pair<string, RoutingMappingPtrT> const & indexPair)
                                  {
                                    indexPair.second->FreeCrossContext();
                                  });
    if (!FindSingleRoute(startTask, m_CachedTargetTask, startMapping->m_dataFacade, routingResult))
    {
      return RouteNotFound;
    }
    INTERRUPT_WHEN_CANCELLED();

    // 5. Restore route.

    Route::TurnsT turnsDir;
    Route::TimesT times;
    vector<m2::PointD> points;
    turns::TurnsGeomT turnsGeom;

    MakeTurnAnnotation(routingResult, startMapping, points, turnsDir, times, turnsGeom);

    route.SetGeometry(points.begin(), points.end());
    route.SetTurnInstructions(turnsDir);
    route.SetSectionTimes(times);
    route.SetTurnInstructionsGeometry(turnsGeom);

    return NoError;
  }
  else //4.2 Multiple mwm case
  {
    LOG(LINFO, ("Multiple mwm routing case"));
    startMapping->LoadCrossContext();
    targetMapping->LoadCrossContext();

    // Check if mwms are neighbours
    /*
    bool left_neighbour = false, right_neighbour = false;
    auto start_out_iterators = startMapping->m_crossContext.GetOutgoingIterators();
    auto target_out_iterators = targetMapping->m_crossContext.GetOutgoingIterators();
    for (auto i = start_out_iterators.first; i != start_out_iterators.second; ++i)
      if (startMapping->m_crossContext.getOutgoingMwmName(i->m_outgoingIndex) == targetMapping->GetName())
      {
        right_neighbour = true;
        break;
      }
    for (auto i = target_out_iterators.first; i != target_out_iterators.second; ++i)
      if (targetMapping->m_crossContext.getOutgoingMwmName(i->m_outgoingIndex) == startMapping->GetName())
      {
        left_neighbour = true;
        break;
      }

    if (!left_neighbour && !right_neighbour)
    {
      LOG(LWARNING, ("MWMs not a neighbours!"));
      return RouteNotFound;
    }
    else if (right_neighbour && !left_neighbour)
    {
      route.AddAbsentCountry(targetMapping->GetName());
      return RouteNotFound;
    }
    else if (!right_neighbour && left_neighbour)
    {
      route.AddAbsentCountry(startMapping->GetName());
      return RouteNotFound;
    }

    LOG(LINFO, ("Mwms are neighbours"));
    */

    // Load source data
    auto const mwmOutsIter = startMapping->m_crossContext.GetOutgoingIterators();
    MultiroutingTaskPointT sources(1), targets(distance(mwmOutsIter.first, mwmOutsIter.second));

    if (targets.empty())
    {
      route.AddAbsentCountry(startMapping->GetName());
      return RouteFileNotExist;
    }

    size_t index = 0;
    for (auto j = mwmOutsIter.first; j < mwmOutsIter.second; ++j, ++index)
      OsrmRouter::GenerateRoutingTaskFromNodeId(j->m_nodeId, false /*isStartNode*/, targets[index]);
    vector<EdgeWeight> weights;
    for (auto const & t : startTask)
    {
      sources[0] = t;
      LOG(LINFO, ("Start case"));
      FindWeightsMatrix(sources, targets, startMapping->m_dataFacade, weights);
      if (find_if(weights.begin(), weights.end(), &IsValidEdgeWeight) != weights.end())
        break;
      weights.clear();
    }

    if (weights.empty())
      return StartPointNotFound;
    INTERRUPT_WHEN_CANCELLED();

    // Load target data
    LastCrossFinder targetFinder(targetMapping, m_CachedTargetTask);

    ResultCode const targetResult = targetFinder.GetError();
    if (targetResult != NoError)
    {
      if (targetResult == RouteFileNotExist)
        route.AddAbsentCountry(targetMapping->GetName());
      return targetResult;
    }

    EdgeWeight finalWeight = INVALID_EDGE_WEIGHT;
    CheckedPathT finalPath;

    RoutingTaskQueueT crossTasks;
    CheckedOutsT checkedOuts;

    LOG(LINFO, ("Duration of a one-to-many routing", timer.ElapsedNano()));
    timer.Reset();

    //Submit tasks from source
    for (size_t j = 0; j < targets.size(); ++j)
    {
      if (weights[j] == INVALID_EDGE_WEIGHT)
        continue;
      INTERRUPT_WHEN_CANCELLED();
      string const & nextMwm =
          startMapping->m_crossContext.getOutgoingMwmName((mwmOutsIter.first + j)->m_outgoingIndex);
      RoutingMappingPtrT nextMapping;
      nextMapping = m_indexManager.GetMappingByName(nextMwm, m_pIndex);
      // If we don't this routing file, we skip this path
      if (!nextMapping->IsValid())
        continue;
      nextMapping->LoadCrossContext();
      size_t tNodeId = (mwmOutsIter.first + j)->m_nodeId;
      size_t nextNodeId = FindNextMwmNode(*(mwmOutsIter.first + j), nextMapping);
      if (nextNodeId == INVALID_NODE_ID)
        continue;
      checkedOuts.insert(make_pair(tNodeId, startMapping->GetName()));
      CheckedPathT tmpPath;
      tmpPath.push_back({startMapping->GetName(), sources[0], targets[j], weights[j]});
      ASSERT(nextMwm != startMapping->GetName(), ("single round error!"));
      if (nextMwm != targetMapping->GetName())
      {
        FeatureGraphNode tmpNode;
        OsrmRouter::GenerateRoutingTaskFromNodeId(nextNodeId, true /*isStartNode*/, tmpNode);
        tmpPath.push_back({nextMapping->GetName(), tmpNode, tmpNode, 0});
        crossTasks.push(tmpPath);
      }
      else
      {
        OsrmRouter::RoutePathCross targetCross;
        if (targetFinder.MakeLastCrossSegment(nextNodeId, targetCross))
        {
          if (weights[j] + targetCross.weight < finalWeight)
          {
            tmpPath.push_back(targetCross);
            finalWeight = weights[j] + targetCross.weight;
            finalPath.swap(tmpPath);
          }
        }
      }
    }

    // Process tasks from tasks queue
    if (!crossTasks.empty())
    {
      while (getPathWeight(crossTasks.top())<finalWeight)
      {
        INTERRUPT_WHEN_CANCELLED();
        CheckedPathT const topTask = crossTasks.top();
        crossTasks.pop();
        RoutePathCross const cross = topTask.back();
        RoutingMappingPtrT currentMapping;

        currentMapping = m_indexManager.GetMappingByName(cross.mwmName, m_pIndex);
        ASSERT(currentMapping->IsValid(), ());

        currentMapping->LoadCrossContext();
        CrossRoutingContextReader const & currentContext = currentMapping->m_crossContext;
        auto current_in_iterators = currentContext.GetIngoingIterators();
        auto current_out_iterators = currentContext.GetOutgoingIterators();

        // find income number
        auto iit = current_in_iterators.first;
        while (iit < current_in_iterators.second)
        {
          if (iit->m_nodeId == cross.startNode.m_node.forward_node_id)
            break;
          ++iit;
        }
        ASSERT(iit != current_in_iterators.second, ());

        // find outs
        for (auto oit = current_out_iterators.first; oit != current_out_iterators.second; ++oit)
        {
          EdgeWeight const outWeight = currentContext.getAdjacencyCost(iit, oit);
          if (outWeight != INVALID_CONTEXT_EDGE_WEIGHT && outWeight != 0)
          {
            ASSERT(outWeight > 0, ("Looks' like .routing file is corrupted!"));

            if (getPathWeight(topTask)+outWeight >= finalWeight)
              continue;
            INTERRUPT_WHEN_CANCELLED();

            string const & nextMwm = currentContext.getOutgoingMwmName(oit->m_outgoingIndex);
            RoutingMappingPtrT nextMapping;

            nextMapping = m_indexManager.GetMappingByName(nextMwm, m_pIndex);
            if (!nextMapping->IsValid())
              continue;

            size_t const tNodeId = oit->m_nodeId;
            auto const outNode = make_pair(tNodeId, currentMapping->GetName());
            if (checkedOuts.find(outNode)!=checkedOuts.end())
              continue;
            checkedOuts.insert(outNode);

            nextMapping->LoadCrossContext();
            size_t nextNodeId = FindNextMwmNode(*oit, nextMapping);
            if(nextNodeId == INVALID_NODE_ID)
              continue;
            CheckedPathT tmpPath(topTask);
            FeatureGraphNode startNode, stopNode;
            OsrmRouter::GenerateRoutingTaskFromNodeId(iit->m_nodeId, true /*isStartNode*/,
                                                      startNode);
            OsrmRouter::GenerateRoutingTaskFromNodeId(tNodeId, false /*isStartNode*/, stopNode);
            tmpPath.back() = {currentMapping->GetName(), startNode, stopNode, outWeight};
            if (nextMwm != targetMapping->GetName())
            {
              FeatureGraphNode tmpNode;
              OsrmRouter::GenerateRoutingTaskFromNodeId(nextNodeId, true /*isStartNode*/,
                                                        tmpNode);
              tmpPath.push_back({nextMapping->GetName(), tmpNode, tmpNode, 0});
              crossTasks.emplace(move(tmpPath));
            }
            else
            {
              OsrmRouter::RoutePathCross targetCross;
              if(targetFinder.MakeLastCrossSegment(nextNodeId, targetCross))
              {
                EdgeWeight const newWeight = getPathWeight(tmpPath) + targetCross.weight;
                if (newWeight < finalWeight)
                {
                  tmpPath.push_back(targetCross);
                  finalWeight = newWeight;
                  finalPath.swap(tmpPath);
                }
              }
            }
          }
        }
        if (crossTasks.empty())
          break;
      }
    }

    LOG(LINFO, ("Duration of a cross path finding", timer.ElapsedNano()));
    timer.Reset();

    // 5. Make generate answer
    if (finalWeight < INVALID_EDGE_WEIGHT)
    {
      // Manually free all cross context allocations before geometry unpacking
      m_indexManager.ForEachMapping([](pair<string, RoutingMappingPtrT> const & indexPair)
                                    {
                                      indexPair.second->FreeCrossContext();
                                    });
      auto code = MakeRouteFromCrossesPath(finalPath, route);
      LOG(LINFO, ("Make final route", timer.ElapsedNano()));
      timer.Reset();
      return code;
    }
    return OsrmRouter::RouteNotFound;
  }
}

m2::PointD OsrmRouter::GetPointForTurnAngle(OsrmMappingTypes::FtSeg const & seg,
                                            FeatureType const & ft, m2::PointD const & turnPnt,
                                            size_t (*GetPndInd)(const size_t, const size_t, const size_t)) const
{
  const size_t maxPntsNum = 7;
  const double maxDistMeter = 300.f;
  double curDist = 0.f;
  m2::PointD pnt = turnPnt, nextPnt;

  const size_t segDist = abs(seg.m_pointEnd - seg.m_pointStart);
  ASSERT_LESS(segDist, ft.GetPointsCount(), ());
  const size_t usedFtPntNum = min(maxPntsNum, segDist);

  for (size_t i = 1; i <= usedFtPntNum; ++i)
  {
    nextPnt = ft.GetPoint(GetPndInd(seg.m_pointStart, seg.m_pointEnd, i));
    curDist += MercatorBounds::DistanceOnEarth(pnt, nextPnt);
    if (curDist > maxDistMeter)
    {
      return nextPnt;
    }
    pnt = nextPnt;
  }
  return nextPnt;
}

OsrmRouter::ResultCode OsrmRouter::MakeTurnAnnotation(RawRoutingResultT const & routingResult,
                                                      RoutingMappingPtrT const & mapping,
                                                      vector<m2::PointD> & points,
                                                      Route::TurnsT & turnsDir,
                                                      Route::TimesT & times,
                                                      turns::TurnsGeomT & turnsGeom)
{
  typedef OsrmMappingTypes::FtSeg SegT;
  SegT const & segBegin = routingResult.m_sourceEdge.m_seg;
  SegT const & segEnd = routingResult.m_targetEdge.m_seg;

  double estimateTime = 0;

  LOG(LDEBUG, ("Shortest path length:", routingResult.m_routePath.shortest_path_length));

  //! @todo: Improve last segment time calculation
  CarModel carModel;
#ifdef _DEBUG
  size_t lastIdx = 0;
#endif
  for (auto i : osrm::irange<size_t>(0, routingResult.m_routePath.unpacked_path_segments.size()))
  {
    INTERRUPT_WHEN_CANCELLED();

    // Get all the coordinates for the computed route
    size_t const n = routingResult.m_routePath.unpacked_path_segments[i].size();
    for (size_t j = 0; j < n; ++j)
    {
      PathData const & path_data = routingResult.m_routePath.unpacked_path_segments[i][j];

      if (j > 0 && !points.empty())
      {
        TurnItem t;
        t.m_index = points.size() - 1;

        GetTurnDirection(routingResult.m_routePath.unpacked_path_segments[i][j - 1],
                         routingResult.m_routePath.unpacked_path_segments[i][j],
                         mapping, t);
        if (t.m_turn != turns::TurnDirection::NoTurn)
        {
          // adding lane info
          t.m_lanes =
              turns::GetLanesInfo(routingResult.m_routePath.unpacked_path_segments[i][j - 1],
                                  *mapping, GetLastSegmentPointIndex, *m_pIndex);
          turnsDir.push_back(move(t));
        }

        // osrm multiple seconds to 10, so we need to divide it back
        double const sTime = TIME_OVERHEAD * path_data.segment_duration / 10.0;
#ifdef _DEBUG
        double dist = 0.0;
        for (size_t l = lastIdx + 1; l < points.size(); ++l)
          dist += MercatorBounds::DistanceOnEarth(points[l - 1], points[l]);
        LOG(LDEBUG, ("Speed:", 3.6 * dist / sTime, "kmph; Dist:", dist, "Time:", sTime, "s", lastIdx, "e", points.size()));
        lastIdx = points.size();
#endif
        estimateTime += sTime;
        times.push_back(Route::TimeItemT(points.size(), estimateTime));
      }

      buffer_vector<SegT, 8> buffer;
      mapping->m_segMapping.ForEachFtSeg(path_data.node, MakeBackInsertFunctor(buffer));

      auto FindIntersectingSeg = [&buffer] (SegT const & seg) -> size_t
      {
        ASSERT(seg.IsValid(), ());
        auto const it = find_if(buffer.begin(), buffer.end(), [&seg] (OsrmMappingTypes::FtSeg const & s)
        {
          return s.IsIntersect(seg);
        });

        ASSERT(it != buffer.end(), ());
        return distance(buffer.begin(), it);
      };

      //m_mapping.DumpSegmentByNode(path_data.node);

      //Do not put out node geometry (we do not have it)!
      size_t startK = 0, endK = buffer.size();
      if (j == 0)
      {
        if (!segBegin.IsValid())
          continue;
        startK = FindIntersectingSeg(segBegin);
      }
      if (j == n - 1)
      {
        if  (!segEnd.IsValid())
          continue;
        endK = FindIntersectingSeg(segEnd) + 1;
      }

      for (size_t k = startK; k < endK; ++k)
      {
        SegT const & seg = buffer[k];

        FeatureType ft;
        Index::FeaturesLoaderGuard loader(*m_pIndex, mapping->GetMwmId());
        loader.GetFeature(seg.m_fid, ft);
        ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

        auto startIdx = seg.m_pointStart;
        auto endIdx = seg.m_pointEnd;
        bool const needTime = (j == 0) || (j == n - 1);

        if (j == 0 && k == startK && segBegin.IsValid())
          startIdx = (seg.m_pointEnd > seg.m_pointStart) ? segBegin.m_pointStart : segBegin.m_pointEnd;
        if (j == n - 1 && k == endK - 1 && segEnd.IsValid())
          endIdx = (seg.m_pointEnd > seg.m_pointStart) ? segEnd.m_pointEnd : segEnd.m_pointStart;

        if (seg.m_pointEnd > seg.m_pointStart)
        {
          for (auto idx = startIdx; idx <= endIdx; ++idx)
          {
            points.push_back(ft.GetPoint(idx));
            if (needTime && idx > startIdx)
              estimateTime += MercatorBounds::DistanceOnEarth(ft.GetPoint(idx - 1), ft.GetPoint(idx)) / carModel.GetSpeed(ft);
          }
        }
        else
        {
          for (auto idx = startIdx; idx > endIdx; --idx)
          {
            if (needTime)
              estimateTime += MercatorBounds::DistanceOnEarth(ft.GetPoint(idx - 1), ft.GetPoint(idx)) / carModel.GetSpeed(ft);
            points.push_back(ft.GetPoint(idx));
          }
          points.push_back(ft.GetPoint(endIdx));
        }
      }
    }
  }

  if (points.size() < 2)
    return RouteNotFound;

  if (routingResult.m_sourceEdge.m_seg.IsValid())
    points.front() = routingResult.m_sourceEdge.m_segPt;
  if (routingResult.m_targetEdge.m_seg.IsValid())
    points.back() = routingResult.m_targetEdge.m_segPt;

  times.push_back(Route::TimeItemT(points.size() - 1, estimateTime));
  if (routingResult.m_targetEdge.m_seg.IsValid())
    turnsDir.push_back(TurnItem(points.size() - 1, turns::TurnDirection::ReachedYourDestination));
  turns::FixupTurns(points, turnsDir);

  turns::CalculateTurnGeometry(points, turnsDir, turnsGeom);

#ifdef _DEBUG
  for (auto t : turnsDir)
  {
    LOG(LDEBUG, (turns::GetTurnString(t.m_turn), ":", t.m_index, t.m_sourceName, "-", t.m_targetName, "exit:", t.m_exitNum));
  }

  size_t last = 0;
  double lastTime = 0;
  for (Route::TimeItemT t : times)
  {
    double dist = 0;
    for (size_t i = last + 1; i <= t.first; ++i)
      dist += MercatorBounds::DistanceOnEarth(points[i - 1], points[i]);

    double time = t.second - lastTime;

    LOG(LDEBUG, ("distance:", dist, "start:", last, "end:", t.first, "Time:", time, "Speed:", 3.6 * dist / time));
    last = t.first;
    lastTime = t.second;
  }
#endif
  LOG(LDEBUG, ("Estimate time:", estimateTime, "s"));
  return OsrmRouter::NoError;
}

NodeID OsrmRouter::GetTurnTargetNode(NodeID src, NodeID trg, QueryEdge::EdgeData const & edgeData, RoutingMappingPtrT const & routingMapping)
{
  ASSERT_NOT_EQUAL(src, SPECIAL_NODEID, ());
  ASSERT_NOT_EQUAL(trg, SPECIAL_NODEID, ());
  if (!edgeData.shortcut)
    return trg;

  ASSERT_LESS(edgeData.id, routingMapping->m_dataFacade.GetNumberOfNodes(), ());
  EdgeID edge = SPECIAL_EDGEID;
  QueryEdge::EdgeData d;
  for (EdgeID e : routingMapping->m_dataFacade.GetAdjacentEdgeRange(edgeData.id))
  {
    if (routingMapping->m_dataFacade.GetTarget(e) == src )
    {
      d = routingMapping->m_dataFacade.GetEdgeData(e, edgeData.id);
      if (d.backward)
      {
        edge = e;
        break;
      }
    }
  }

  if (edge == SPECIAL_EDGEID)
  {
    for (EdgeID e : routingMapping->m_dataFacade.GetAdjacentEdgeRange(src))
    {
      if (routingMapping->m_dataFacade.GetTarget(e) == edgeData.id)
      {
        d = routingMapping->m_dataFacade.GetEdgeData(e, src);
        if (d.forward)
        {
          edge = e;
          break;
        }
      }
    }
  }
  ASSERT_NOT_EQUAL(edge, SPECIAL_EDGEID, ());

  if (d.shortcut)
    return GetTurnTargetNode(src, edgeData.id, d, routingMapping);

  return edgeData.id;
}

void OsrmRouter::GetPossibleTurns(NodeID node,
                                  m2::PointD const & p1,
                                  m2::PointD const & p,
                                  RoutingMappingPtrT const & routingMapping,
                                  OsrmRouter::TurnCandidatesT & candidates)
{
  ASSERT(routingMapping.get(), ());
  for (EdgeID e : routingMapping->m_dataFacade.GetAdjacentEdgeRange(node))
  {
    QueryEdge::EdgeData const data = routingMapping->m_dataFacade.GetEdgeData(e, node);
    if (!data.forward)
      continue;

    NodeID trg = GetTurnTargetNode(node, routingMapping->m_dataFacade.GetTarget(e), data, routingMapping);
    ASSERT_NOT_EQUAL(trg, SPECIAL_NODEID, ());

    auto const range = routingMapping->m_segMapping.GetSegmentsRange(trg);
    OsrmMappingTypes::FtSeg seg;
    routingMapping->m_segMapping.GetSegmentByIndex(range.first, seg);
    if (!seg.IsValid())
      continue;

    FeatureType ft;
    Index::FeaturesLoaderGuard loader(*m_pIndex, routingMapping->GetMwmId());
    loader.GetFeature(seg.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    m2::PointD const p2 = ft.GetPoint(seg.m_pointStart < seg.m_pointEnd ? seg.m_pointStart + 1 : seg.m_pointStart - 1);
    ASSERT_LESS(MercatorBounds::DistanceOnEarth(p, ft.GetPoint(seg.m_pointStart)), FEATURES_NEAR_TURN_M, ());

    double const a = my::RadToDeg(ang::TwoVectorsAngle(p, p1, p2));

    candidates.emplace_back(a, trg);
  }

  sort(candidates.begin(), candidates.end(), [](TurnCandidate const & t1, TurnCandidate const & t2)
  {
    return t1.m_node < t2.m_node;
  });

  auto last = unique(candidates.begin(), candidates.end(), [](TurnCandidate const & t1, TurnCandidate const & t2)
  {
    return t1.m_node == t2.m_node;
  });
  candidates.erase(last, candidates.end());

  sort(candidates.begin(), candidates.end(), [](TurnCandidate const & t1, TurnCandidate const & t2)
  {
    return t1.m_angle < t2.m_angle;
  });
}

void OsrmRouter::GetTurnGeometry(m2::PointD const & p, m2::PointD const & p1,
                                 OsrmRouter::GeomTurnCandidateT & candidates, RoutingMappingPtrT const & mapping) const
{
  ASSERT(mapping.get(), ());
  Point2Geometry getter(p, p1, candidates);
  m_pIndex->ForEachInRectForMWM(getter, MercatorBounds::RectByCenterXYAndSizeInMeters(p, FEATURES_NEAR_TURN_M),
                                scales::GetUpperScale(), mapping->GetMwmId());
}

turns::TurnDirection OsrmRouter::InvertDirection(turns::TurnDirection dir) const
{
  switch (dir)
  {
  case turns::TurnDirection::TurnSharpRight:
    return turns::TurnDirection::TurnSharpLeft;
  case turns::TurnDirection::TurnRight:
    return turns::TurnDirection::TurnLeft;
  case turns::TurnDirection::TurnSlightRight:
    return turns::TurnDirection::TurnSlightLeft;
  case turns::TurnDirection::TurnSlightLeft:
    return turns::TurnDirection::TurnSlightRight;
  case turns::TurnDirection::TurnLeft:
    return turns::TurnDirection::TurnRight;
  case turns::TurnDirection::TurnSharpLeft:
    return turns::TurnDirection::TurnSharpRight;
  default:
    return dir;
  };
}

turns::TurnDirection OsrmRouter::MostRightDirection(const double angle) const
{
  double const lowerSharpRightBound = 23.;
  double const upperSharpRightBound = 67.;
  double const upperRightBound = 140.;
  double const upperSlightRight = 195.;
  double const upperGoStraitBound = 205.;
  double const upperSlightLeftBound = 240.;
  double const upperLeftBound = 336.;

  if (angle >= lowerSharpRightBound && angle < upperSharpRightBound)
    return turns::TurnDirection::TurnSharpRight;
  else if (angle >= upperSharpRightBound && angle < upperRightBound)
    return  turns::TurnDirection::TurnRight;
  else if (angle >= upperRightBound && angle < upperSlightRight)
    return turns::TurnDirection::TurnSlightRight;
  else if (angle >= upperSlightRight && angle < upperGoStraitBound)
    return  turns::TurnDirection::GoStraight;
  else if (angle >= upperGoStraitBound && angle < upperSlightLeftBound)
    return  turns::TurnDirection::TurnSlightLeft;
  else if (angle >= upperSlightLeftBound && angle < upperLeftBound)
    return  turns::TurnDirection::TurnLeft;
  return turns::TurnDirection::NoTurn;
}

turns::TurnDirection OsrmRouter::MostLeftDirection(const double angle) const
{
  return InvertDirection(MostRightDirection(360 - angle));
}

turns::TurnDirection OsrmRouter::IntermediateDirection(const double angle) const
{
  double const lowerSharpRightBound = 23.;
  double const upperSharpRightBound = 67.;
  double const upperRightBound = 130.;
  double const upperSlightRight = 170.;
  double const upperGoStraitBound = 190.;
  double const upperSlightLeftBound = 230.;
  double const upperLeftBound = 292.;
  double const upperSharpLeftBound = 336.;

  if (angle >= lowerSharpRightBound && angle < upperSharpRightBound)
    return turns::TurnDirection::TurnSharpRight;
  else if (angle >= upperSharpRightBound && angle < upperRightBound)
    return  turns::TurnDirection::TurnRight;
  else if (angle >= upperRightBound && angle < upperSlightRight)
    return turns::TurnDirection::TurnSlightRight;
  else if (angle >= upperSlightRight && angle < upperGoStraitBound)
    return  turns::TurnDirection::GoStraight;
  else if (angle >= upperGoStraitBound && angle < upperSlightLeftBound)
    return  turns::TurnDirection::TurnSlightLeft;
  else if (angle >= upperSlightLeftBound && angle < upperLeftBound)
    return  turns::TurnDirection::TurnLeft;
  else if (angle >= upperLeftBound && angle < upperSharpLeftBound)
    return turns::TurnDirection::TurnSharpLeft;
  return turns::TurnDirection::NoTurn;
}

bool OsrmRouter::KeepOnewayOutgoingTurnIncomingEdges(TurnItem const & turn,
                                                     m2::PointD const & p, m2::PointD const & p1OneSeg,
                                                     RoutingMappingPtrT const & mapping) const
{
  ASSERT(mapping.get(), ());
  size_t const outgoingNotesCount = 1;
  if (turns::IsGoStraightOrSlightTurn(turn.m_turn))
    return false;
  else
  {
    GeomTurnCandidateT geoNodes;
    GetTurnGeometry(p, p1OneSeg, geoNodes, mapping);
    if (geoNodes.size() <= outgoingNotesCount)
      return false;
    return true;
  }
}

bool OsrmRouter::KeepOnewayOutgoingTurnRoundabout(bool isRound1, bool isRound2) const
{
  return !isRound1 && isRound2;
}

turns::TurnDirection OsrmRouter::RoundaboutDirection(bool isRound1, bool isRound2,
                                                     bool hasMultiTurns) const
{
  if (isRound1 && isRound2)
  {
    if (hasMultiTurns)
      return turns::TurnDirection::StayOnRoundAbout;
    else
      return turns::TurnDirection::NoTurn;
  }

  if (!isRound1 && isRound2)
    return turns::TurnDirection::EnterRoundAbout;

  if (isRound1 && !isRound2)
    return turns::TurnDirection::LeaveRoundAbout;

  ASSERT(false, ());
  return turns::TurnDirection::NoTurn;
}

// @todo(vbykoianko) Move this method and all dependencies to turns_generator.cpp
void OsrmRouter::GetTurnDirection(PathData const & node1,
                                  PathData const & node2,
                                  RoutingMappingPtrT const & routingMapping, TurnItem & turn)
{
  ASSERT(routingMapping.get(), ());
  OsrmMappingTypes::FtSeg const seg1 =
      turns::GetSegment(node1, *routingMapping, GetLastSegmentPointIndex);
  OsrmMappingTypes::FtSeg const seg2 =
      turns::GetSegment(node2, *routingMapping, [](pair<size_t, size_t> const & p)
      {
        return p.first;
      });

  if (!seg1.IsValid() || !seg2.IsValid())
  {
    LOG(LWARNING, ("Some turns can't load geometry"));
    turn.m_turn = turns::TurnDirection::NoTurn;
    return;
  }

  FeatureType ft1, ft2;
  Index::FeaturesLoaderGuard loader1(*m_pIndex, routingMapping->GetMwmId());
  Index::FeaturesLoaderGuard loader2(*m_pIndex, routingMapping->GetMwmId());
  loader1.GetFeature(seg1.m_fid, ft1);
  loader2.GetFeature(seg2.m_fid, ft2);

  ft1.ParseGeometry(FeatureType::BEST_GEOMETRY);
  ft2.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ASSERT_LESS(MercatorBounds::DistanceOnEarth(ft1.GetPoint(seg1.m_pointEnd), ft2.GetPoint(seg2.m_pointStart)), FEATURES_NEAR_TURN_M, ());

  m2::PointD const p = ft1.GetPoint(seg1.m_pointEnd);
  m2::PointD const p1 = GetPointForTurnAngle(seg1, ft1, p,
                    [](const size_t start, const size_t end, const size_t i)
                    {
                      return end > start ? end - i : end + i;
                    });
  m2::PointD const p2 = GetPointForTurnAngle(seg2, ft2, p,
                    [](const size_t start, const size_t end, const size_t i)
                    {
                      return end > start ? start + i : start - i;
                    });
  double const a = my::RadToDeg(ang::TwoVectorsAngle(p, p1, p2));

  m2::PointD const p1OneSeg = ft1.GetPoint(seg1.m_pointStart < seg1.m_pointEnd ? seg1.m_pointEnd - 1 : seg1.m_pointEnd + 1);
  TurnCandidatesT nodes;
  GetPossibleTurns(node1.node, p1OneSeg, p, routingMapping, nodes);

  turn.m_turn = turns::TurnDirection::NoTurn;
  size_t const nodesSz = nodes.size();
  bool const hasMultiTurns = (nodesSz >= 2);

  if (nodesSz == 0)
  {
    return;
  }

  if (nodes.front().m_node == node2.node)
    turn.m_turn = MostRightDirection(a);
  else if (nodes.back().m_node == node2.node)
    turn.m_turn = MostLeftDirection(a);
  else turn.m_turn = IntermediateDirection(a);

  bool const isRound1 = ftypes::IsRoundAboutChecker::Instance()(ft1);
  bool const isRound2 = ftypes::IsRoundAboutChecker::Instance()(ft2);

  if (isRound1 || isRound2)
  {
    turn.m_turn = RoundaboutDirection(isRound1, isRound2, hasMultiTurns);
    return;
  }

  turn.m_keepAnyway = (!ftypes::IsLinkChecker::Instance()(ft1)
                       && ftypes::IsLinkChecker::Instance()(ft2));

  // get names
  string name1, name2;
  {
    ft1.GetName(FeatureType::DEFAULT_LANG, turn.m_sourceName);
    ft2.GetName(FeatureType::DEFAULT_LANG, turn.m_targetName);

    search::GetStreetNameAsKey(turn.m_sourceName, name1);
    search::GetStreetNameAsKey(turn.m_targetName, name2);
  }

  string road1 = ft1.GetRoadNumber();
  string road2 = ft2.GetRoadNumber();

  if (!turn.m_keepAnyway
      && ((!name1.empty() && name1 == name2) || (!road1.empty() && road1 == road2)))
  {
    turn.m_turn = turns::TurnDirection::NoTurn;
    return;
  }

  if (!hasMultiTurns
      && !KeepOnewayOutgoingTurnIncomingEdges(turn, p, p1OneSeg, routingMapping)
      && !KeepOnewayOutgoingTurnRoundabout(isRound1, isRound2))
  {
    turn.m_turn = turns::TurnDirection::NoTurn;
    return;
  }

  if (turn.m_turn == turns::TurnDirection::GoStraight)
  {
    if (!hasMultiTurns)
      turn.m_turn = turns::TurnDirection::NoTurn;

    return;
  }

  if (turn.m_turn == turns::TurnDirection::NoTurn)
    turn.m_turn = turns::TurnDirection::UTurn;
}

IRouter::ResultCode OsrmRouter::FindPhantomNodes(string const & fName, m2::PointD const & point, m2::PointD const & direction,
                                                 FeatureGraphNodeVecT & res, size_t maxCount, RoutingMappingPtrT const & mapping)
{
  Point2PhantomNode getter(mapping->m_segMapping, m_pIndex, direction);
  getter.SetPoint(point);

  m_pIndex->ForEachInRectForMWM(getter,
    MercatorBounds::RectByCenterXYAndSizeInMeters(point, FEATURE_BY_POINT_RADIUS_M),
    scales::GetUpperScale(), mapping->GetMwmId());

  if (!getter.HasCandidates())
    return StartPointNotFound;

  getter.MakeResult(res, maxCount);
  return NoError;
}

}
