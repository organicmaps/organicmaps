#include "openlr/openlr_decoder.hpp"

#include "openlr/candidate_paths_getter.hpp"
#include "openlr/candidate_points_getter.hpp"
#include "openlr/decoded_path.hpp"
#include "openlr/graph.hpp"
#include "openlr/helpers.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/paths_connector.hpp"
#include "openlr/road_info_getter.hpp"
#include "openlr/router.hpp"
#include "openlr/way_point.hpp"

#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/index.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/country_file.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <queue>
#include <thread>
#include <utility>

using namespace routing;
using namespace std;

namespace openlr
{
namespace
{
size_t constexpr kCacheLineSize = 64;

struct alignas(kCacheLineSize) Stats
{
  void Add(Stats const & rhs)
  {
    m_shortRoutes += rhs.m_shortRoutes;
    m_zeroCanditates += rhs.m_zeroCanditates;
    m_moreThanOneCandidate += rhs.m_moreThanOneCandidate;
    m_routeIsNotCalculated += rhs.m_routeIsNotCalculated;
    m_tightOffsets += rhs.m_tightOffsets;
    m_total += rhs.m_total;
  }

  uint32_t m_shortRoutes = 0;
  uint32_t m_zeroCanditates = 0;
  uint32_t m_moreThanOneCandidate = 0;
  uint32_t m_routeIsNotCalculated = 0;
  uint32_t m_tightOffsets = 0;
  uint32_t m_total = 0;
};

bool IsRealVertex(m2::PointD const & p, FeatureID const & fid, Index const & index)
{
  Index::FeaturesLoaderGuard g(index, fid.m_mwmId);
  auto const ft = g.GetOriginalFeatureByIndex(fid.m_index);
  bool matched = false;
  ft->ForEachPoint(
      [&p, &matched](m2::PointD const & fp) {
        if (p == fp)
          matched = true;
      },
      FeatureType::BEST_GEOMETRY);
  return matched;
};

void ExpandFake(Graph::EdgeVector & path, Graph::EdgeVector::iterator edgeIt, Index const & index,
                Graph & g)
{
  if (!edgeIt->IsFake())
    return;

  Graph::EdgeVector edges;
  if (IsRealVertex(edgeIt->GetStartPoint(), edgeIt->GetFeatureId(), index))
  {
    g.GetRegularOutgoingEdges(edgeIt->GetStartJunction(), edges);
  }
  else
  {
    ASSERT(IsRealVertex(edgeIt->GetEndPoint(), edgeIt->GetFeatureId(), index), ());
    g.GetRegularIngoingEdges(edgeIt->GetEndJunction(), edges);
  }

  CHECK(!edges.empty(), ());

  auto const it = find_if(begin(edges), end(edges), [&edgeIt](Graph::Edge const & real) {
      if (real.GetFeatureId() == edgeIt->GetFeatureId() && real.GetSegId() == edgeIt->GetSegId())
        return true;
      return false;
    });

  CHECK(it != end(edges), ());

  // If a fake edge is larger than a half of the corresponding real one, substitute
  // the fake one with real one. Drop the fake one otherwize.
  if (2 * EdgeLength(*edgeIt) >= EdgeLength(*it))
    *edgeIt = *it;
  else
    path.erase(edgeIt);
};

void ExpandFakes(Index const & index, Graph & g, Graph::EdgeVector & path)
{
  ASSERT(!path.empty(), ());

  ExpandFake(path, begin(path), index, g);
  ExpandFake(path, --end(path), index, g);
}

// Returns an iterator pointing to the first edge that should not be cut off.
// Offsets denote a distance in meters one should travel from the start/end of the path
// to some point alog that path and drop everything form the start to that point or from
// that point to the end.
template <typename InputIterator>
InputIterator CutOffset(InputIterator start, InputIterator const stop, uint32_t const offset)
{
  if (offset == 0)
    return start;

  for (uint32_t distance = 0; start != stop; ++start)
  {
    auto const edgeLen = EdgeLength(*start);
    if (distance <= offset && offset < distance + edgeLen)
    {
      // Throw out this edge if (offest - distance) is greater than edgeLength / 2.
      if (2 * (offset - distance) >= edgeLen)
        ++start;
      break;
    }
    distance += edgeLen;
  }

  return start;
}

template <typename InputIterator, typename OutputIterator>
void CopyWithoutOffsets(InputIterator const start, InputIterator const stop, OutputIterator out,
                        uint32_t const positiveOffset, uint32_t const negativeOffset)
{
  auto from = start;
  auto to = stop;

  if (distance(start, stop) > 1)
  {
    from = CutOffset(start, stop, positiveOffset);
    // |to| points past the last edge we need to take.
    to = CutOffset(reverse_iterator<InputIterator>(stop), reverse_iterator<InputIterator>(start),
                   negativeOffset)
             .base();
  }

  if (from >= to)
    return;

  copy(from, to, out);
}
}  // namespace

// OpenLRDecoder::SegmentsFilter -------------------------------------------------------------
OpenLRDecoder::SegmentsFilter::SegmentsFilter(string const & idsPath,
                                              bool const multipointsOnly)
  : m_idsSet(false), m_multipointsOnly(multipointsOnly)
{
  if (idsPath.empty())
    return;

  ifstream ifs(idsPath);
  CHECK(ifs, ("Can't find", idsPath));
  m_ids.insert(istream_iterator<uint32_t>(ifs), istream_iterator<uint32_t>());

  CHECK(!ifs, ("Garbage in", idsPath));
  m_idsSet = true;
}

bool OpenLRDecoder::SegmentsFilter::Matches(LinearSegment const & segment) const
{
  if (m_multipointsOnly && segment.m_locationReference.m_points.size() == 2)
    return false;
  if (m_idsSet && m_ids.count(segment.m_segmentId) == 0)
    return false;
  return true;
}

// OpenLRDecoder -----------------------------------------------------------------------------
OpenLRDecoder::OpenLRDecoder(vector<Index> const & indexes,
                             CountryParentNameGetter const & countryParentNameGetter)
  : m_indexes(indexes), m_countryParentNameGetter(countryParentNameGetter)
{
}

void OpenLRDecoder::DecodeV1(vector<LinearSegment> const & segments, uint32_t const numThreads,
                             vector<DecodedPath> & paths)
{
  double const kOffsetToleranceM = 10;

  // This code computes the most optimal (in the sense of cache lines
  // occupancy) batch size.
  size_t constexpr a = my::LCM(sizeof(LinearSegment), kCacheLineSize) / sizeof(LinearSegment);
  size_t constexpr b =
      my::LCM(sizeof(IRoadGraph::TEdgeVector), kCacheLineSize) / sizeof(IRoadGraph::TEdgeVector);
  size_t constexpr kBatchSize = my::LCM(a, b);
  size_t constexpr kProgressFrequency = 100;

  auto worker = [&segments, &paths, kBatchSize, kProgressFrequency, kOffsetToleranceM, numThreads,
                 this](size_t threadNum, Index const & index, Stats & stats) {
    FeaturesRoadGraph roadGraph(index, IRoadGraph::Mode::ObeyOnewayTag,
                                make_unique<CarModelFactory>(m_countryParentNameGetter));
    RoadInfoGetter roadInfoGetter(index);
    Router router(roadGraph, roadInfoGetter);

    size_t const numSegments = segments.size();

    vector<WayPoint> points;

    for (size_t i = threadNum * kBatchSize; i < numSegments; i += numThreads * kBatchSize)
    {
      for (size_t j = i; j < numSegments && j < i + kBatchSize; ++j)
      {
        auto const & segment = segments[j];
        auto const & ref = segment.m_locationReference;

        paths[j].m_segmentId.Set(segment.m_segmentId);

        points.clear();
        for (auto const & point : ref.m_points)
          points.emplace_back(point);

        auto positiveOffsetM = ref.m_positiveOffsetMeters;
        if (positiveOffsetM >= points[0].m_distanceToNextPointM)
        {
          LOG(LWARNING, ("Wrong positive offset for segment:", segment.m_segmentId));
          positiveOffsetM = 0;
        }

        auto negativeOffsetM = ref.m_negativeOffsetMeters;
        if (negativeOffsetM >= points[points.size() - 2].m_distanceToNextPointM)
        {
          LOG(LWARNING, ("Wrong negative offset for segment:", segment.m_segmentId));
          negativeOffsetM = 0;
        }

        {
          double expectedLength = 0;
          for (size_t i = 0; i + 1 < points.size(); ++i)
            expectedLength += points[i].m_distanceToNextPointM;

          if (positiveOffsetM + negativeOffsetM >= expectedLength)
          {
            LOG(LINFO,
                ("Skipping", segment.m_segmentId, "due to wrong positive/negative offsets."));
            ++stats.m_routeIsNotCalculated;
            continue;
          }

          if (positiveOffsetM + negativeOffsetM + kOffsetToleranceM >= expectedLength)
          {
            LOG(LINFO, ("Too tight positive and negative offsets, setting them to zero."));
            positiveOffsetM = 0;
            negativeOffsetM = 0;
            ++stats.m_tightOffsets;
          }
        }

        auto & path = paths[j].m_path;
        if (!router.Go(points, positiveOffsetM, negativeOffsetM, path))
          ++stats.m_routeIsNotCalculated;

        ++stats.m_total;

        if (stats.m_total % kProgressFrequency == 0)
        {
          LOG(LINFO, ("Thread", threadNum, "processed:", stats.m_total,
                      "failed:", stats.m_routeIsNotCalculated));
        }
      }
    }
  };

  vector<Stats> stats(numThreads);
  vector<thread> workers;
  for (size_t i = 1; i < numThreads; ++i)
    workers.emplace_back(worker, i, ref(m_indexes[i]), ref(stats[i]));
  worker(0 /* threadNum */, m_indexes[0], stats[0]);
  for (auto & worker : workers)
    worker.join();

  Stats allStats;
  for (auto const & s : stats)
    allStats.Add(s);

  LOG(LINFO, ("Parsed segments:", allStats.m_total));
  LOG(LINFO, ("Routes failed:", allStats.m_routeIsNotCalculated));
  LOG(LINFO, ("Tight offsets:", allStats.m_tightOffsets));
  LOG(LINFO, ("Short routes:", allStats.m_shortRoutes));
  LOG(LINFO, ("Ambiguous routes:", allStats.m_moreThanOneCandidate));
  LOG(LINFO, ("Path is not reconstructed:", allStats.m_zeroCanditates));
}

void OpenLRDecoder::DecodeV2(vector<LinearSegment> const & segments,
                             uint32_t const /* numThreads */, vector<DecodedPath> & paths)
{
  ASSERT(!m_indexes.empty(), ());
  auto const & index = m_indexes.back();

  Graph graph(index, make_unique<CarModelFactory>(m_countryParentNameGetter));

  v2::Stats stat;
  my::Timer timer;
  RoadInfoGetter infoGetter(index);
  for (size_t i = 0; i < segments.size(); ++i)
  {
    if (!DecodeSingleSegment(segments[i], index, graph, infoGetter, paths[i], stat))
      ++stat.m_routesFailed;
    ++stat.m_routesHandled;

    if (i % 100 == 0 || i == segments.size() - 1)
    {
      LOG(LINFO, (i, "segments are processed in",
                  timer.ElapsedSeconds(),
                  "seconds"));
      timer.Reset();
    }
  }

  LOG(LINFO, ("Total routes handled:", stat.m_routesHandled));
  LOG(LINFO, ("Failed:", stat.m_routesFailed));
  LOG(LINFO, ("No candidate lines:", stat.m_noCandidateFound));
  LOG(LINFO, ("Wrong distance to next point:", stat.m_dnpIsZero));
  LOG(LINFO, ("Wrong offsets:", stat.m_wrongOffsets));
  LOG(LINFO, ("No shortest path:", stat.m_noShortestPathFound));
}

bool OpenLRDecoder::DecodeSingleSegment(LinearSegment const & segment, Index const & index,
                                        Graph & graph, RoadInfoGetter & infoGetter,
                                        DecodedPath & path, v2::Stats & stat)
{
  // TODO(mgsergio): Scale indexes.

  double const kPathLengthTolerance = 0.30;
  uint32_t const kMaxJunctionCandidates = 10;
  uint32_t const kMaxProjectionCandidates = 5;

  graph.ResetFakes();

  path.m_segmentId.Set(segment.m_segmentId);

  auto const & points = segment.GetLRPs();
  vector<vector<Graph::EdgeVector>> lineCandidates;
  lineCandidates.reserve(points.size());
  LOG(LDEBUG, ("Decoding segment:", segment.m_segmentId, "with", points.size(), "points"));

  CandidatePointsGetter pointsGetter(kMaxJunctionCandidates, kMaxProjectionCandidates, index, graph);
  CandidatePathsGetter pathsGetter(pointsGetter, graph, infoGetter, stat);

  if (!pathsGetter.GetLineCandidatesForPoints(points, lineCandidates))
    return false;

  vector<Graph::EdgeVector> resultPath;
  PathsConnector connector(kPathLengthTolerance, graph, stat);
  if (!connector.ConnectCandidates(points, lineCandidates, resultPath))
    return false;

  Graph::EdgeVector route;
  for (auto const & part : resultPath)
    route.insert(end(route), begin(part), end(part));

  uint32_t requiredRouteDistanceM = 0;
  // Sum app all distances between points. Last point's m_distanceToNextPoint
  // should be equal to zero, but let's skip it just in case.
  for (auto it = begin(points); it != prev(end(points));  ++it)
    requiredRouteDistanceM += it->m_distanceToNextPoint;
  uint32_t actualRouteDistanceM = 0;
  for (auto const & e : route)
    actualRouteDistanceM += EdgeLength(e);

  auto const scale = static_cast<double>(actualRouteDistanceM) / requiredRouteDistanceM;
  LOG(LDEBUG, ("actualRouteDistance:", actualRouteDistanceM,
               "requiredRouteDistance:", requiredRouteDistanceM, "scale:", scale));

  auto const positiveOffset =
      static_cast<uint32_t>(segment.m_locationReference.m_positiveOffsetMeters * scale);
  auto const negativeOffset =
      static_cast<uint32_t>(segment.m_locationReference.m_negativeOffsetMeters * scale);

  if (positiveOffset + negativeOffset >= requiredRouteDistanceM)
  {
    ++stat.m_wrongOffsets;
    LOG(LINFO, ("Wrong offsets for segment:", segment.m_segmentId));
    return false;
  }

  ExpandFakes(index, graph, route);
  ASSERT(none_of(begin(route), end(route), mem_fn(&Graph::Edge::IsFake)), ());
  CopyWithoutOffsets(begin(route), end(route), back_inserter(path.m_path), positiveOffset,
                     negativeOffset);

  if (path.m_path.empty())
  {
    ++stat.m_wrongOffsets;
    LOG(LINFO, ("Path is empty after offsets cutting. segmentId:", segment.m_segmentId));
    return false;
  }

  return true;
}
}  // namespace openlr
