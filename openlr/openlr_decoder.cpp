#include "openlr/openlr_decoder.hpp"

#include "openlr/cache_line_size.hpp"
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
struct alignas(kCacheLineSize) Stats
{
  void Add(Stats const & rhs)
  {
    m_shortRoutes += rhs.m_shortRoutes;
    m_zeroCanditates += rhs.m_zeroCanditates;
    m_moreThanOneCandidate += rhs.m_moreThanOneCandidate;
    m_routesFailed += rhs.m_routesFailed;
    m_tightOffsets += rhs.m_tightOffsets;
    m_routesHandled += rhs.m_routesHandled;
  }

  void Report() const
  {
    LOG(LINFO, ("Parsed segments:", m_routesHandled));
    LOG(LINFO, ("Routes failed:", m_routesFailed));
    LOG(LINFO, ("Tight offsets:", m_tightOffsets));
    LOG(LINFO, ("Short routes:", m_shortRoutes));
    LOG(LINFO, ("Ambiguous routes:", m_moreThanOneCandidate));
    LOG(LINFO, ("Path is not reconstructed:", m_zeroCanditates));
  }

  uint32_t m_shortRoutes = 0;
  uint32_t m_zeroCanditates = 0;
  uint32_t m_moreThanOneCandidate = 0;
  uint32_t m_routesFailed = 0;
  uint32_t m_tightOffsets = 0;
  uint32_t m_routesHandled = 0;
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
  if (path.empty())
    return;
  ExpandFake(path, --end(path), index, g);
}

// Returns an iterator pointing to the first edge that should not be cut off.
// Offsets denote a distance in meters one should travel from the start/end of the path
// to some point alog that path and drop everything form the start to that point or from
// that point to the end.
template <typename InputIterator>
InputIterator CutOffset(InputIterator start, InputIterator const stop, double const offset)
{
  if (offset == 0)
    return start;

  for (double distance = 0.0; start != stop; ++start)
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

class SegmentsDecoderV1
{
public:
  SegmentsDecoderV1(Index const & index, unique_ptr<CarModelFactory> cmf)
    : m_roadGraph(index, IRoadGraph::Mode::ObeyOnewayTag, move(cmf))
    , m_infoGetter(index)
    , m_router(m_roadGraph, m_infoGetter)
  {
  }

  bool DecodeSegment(LinearSegment const & segment, DecodedPath & path, Stats & stat)
  {
    double const kOffsetToleranceM = 10;

    auto const & ref = segment.m_locationReference;

    path.m_segmentId.Set(segment.m_segmentId);

    m_points.clear();
    for (auto const & point : ref.m_points)
      m_points.emplace_back(point);

    auto positiveOffsetM = ref.m_positiveOffsetMeters;
    if (positiveOffsetM >= m_points[0].m_distanceToNextPointM)
    {
      LOG(LWARNING, ("Wrong positive offset for segment:", segment.m_segmentId));
      positiveOffsetM = 0;
    }

    auto negativeOffsetM = ref.m_negativeOffsetMeters;
    if (negativeOffsetM >= m_points[m_points.size() - 2].m_distanceToNextPointM)
    {
      LOG(LWARNING, ("Wrong negative offset for segment:", segment.m_segmentId));
      negativeOffsetM = 0;
    }

    {
      double expectedLength = 0;
      for (size_t i = 0; i + 1 < m_points.size(); ++i)
        expectedLength += m_points[i].m_distanceToNextPointM;

      if (positiveOffsetM + negativeOffsetM >= expectedLength)
      {
        LOG(LINFO, ("Skipping", segment.m_segmentId, "due to wrong positive/negative offsets."));
        return false;
      }

      if (positiveOffsetM + negativeOffsetM + kOffsetToleranceM >= expectedLength)
      {
        LOG(LINFO, ("Too tight positive and negative offsets, setting them to zero."));
        positiveOffsetM = 0;
        negativeOffsetM = 0;
        ++stat.m_tightOffsets;
      }
    }

    if (!m_router.Go(m_points, positiveOffsetM, negativeOffsetM, path.m_path))
      return false;

    return true;
  }

private:
  FeaturesRoadGraph m_roadGraph;
  RoadInfoGetter m_infoGetter;
  Router m_router;
  vector<WayPoint> m_points;
};

class SegmentsDecoderV2
{
public:
  SegmentsDecoderV2(Index const & index, unique_ptr<CarModelFactory> cmf)
    : m_index(index), m_graph(index, move(cmf)), m_infoGetter(index)
  {
  }

  bool DecodeSegment(LinearSegment const & segment, DecodedPath & path, v2::Stats & stat)
  {
    double const kPathLengthTolerance = 0.30;
    uint32_t const kMaxJunctionCandidates = 10;
    uint32_t const kMaxProjectionCandidates = 5;

    m_graph.ResetFakes();

    path.m_segmentId.Set(segment.m_segmentId);

    auto const & points = segment.GetLRPs();
    CHECK_GREATER(points.size(), 1, ("A segment cannot consist of less than two points"));
    vector<vector<Graph::EdgeVector>> lineCandidates;
    lineCandidates.reserve(points.size());
    LOG(LDEBUG, ("Decoding segment:", segment.m_segmentId, "with", points.size(), "points"));

    CandidatePointsGetter pointsGetter(kMaxJunctionCandidates, kMaxProjectionCandidates, m_index,
                                       m_graph);
    CandidatePathsGetter pathsGetter(pointsGetter, m_graph, m_infoGetter, stat);

    if (!pathsGetter.GetLineCandidatesForPoints(points, lineCandidates))
      return false;

    vector<Graph::EdgeVector> resultPath;
    PathsConnector connector(kPathLengthTolerance, m_graph, stat);
    if (!connector.ConnectCandidates(points, lineCandidates, resultPath))
      return false;

    Graph::EdgeVector route;
    for (auto const & part : resultPath)
      route.insert(end(route), begin(part), end(part));

    double requiredRouteDistanceM = 0.0;
    // Sum app all distances between points. Last point's m_distanceToNextPoint
    // should be equal to zero, but let's skip it just in case.
    CHECK(!points.empty(), ());
    for (auto it = begin(points); it != prev(end(points));  ++it)
      requiredRouteDistanceM += it->m_distanceToNextPoint;

    double actualRouteDistanceM = 0.0;
    for (auto const & e : route)
      actualRouteDistanceM += EdgeLength(e);

    auto const scale = actualRouteDistanceM / requiredRouteDistanceM;
    LOG(LDEBUG, ("actualRouteDistance:", actualRouteDistanceM,
                 "requiredRouteDistance:", requiredRouteDistanceM, "scale:", scale));

    auto const positiveOffsetM = segment.m_locationReference.m_positiveOffsetMeters * scale;
    auto const negativeOffsetM = segment.m_locationReference.m_negativeOffsetMeters * scale;

    if (positiveOffsetM + negativeOffsetM >= requiredRouteDistanceM)
    {
      ++stat.m_wrongOffsets;
      LOG(LINFO, ("Wrong offsets for segment:", segment.m_segmentId));
      return false;
    }

    ExpandFakes(m_index, m_graph, route);
    ASSERT(none_of(begin(route), end(route), mem_fn(&Graph::Edge::IsFake)), (segment.m_segmentId));
    CopyWithoutOffsets(begin(route), end(route), back_inserter(path.m_path), positiveOffsetM,
                       negativeOffsetM);

    if (path.m_path.empty())
    {
      ++stat.m_wrongOffsets;
      LOG(LINFO, ("Path is empty after offsets cutting. segmentId:", segment.m_segmentId));
      return false;
    }

    return true;
  }

private:
  Index const & m_index;
  Graph m_graph;
  RoadInfoGetter m_infoGetter;
};

size_t constexpr GetOptimalBatchSize()
{
  // This code computes the most optimal (in the sense of cache lines
  // occupancy) batch size.
  size_t constexpr a = my::LCM(sizeof(LinearSegment), kCacheLineSize) / sizeof(LinearSegment);
  size_t constexpr b =
      my::LCM(sizeof(IRoadGraph::TEdgeVector), kCacheLineSize) / sizeof(IRoadGraph::TEdgeVector);
  return my::LCM(a, b);
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
  Decode<SegmentsDecoderV1, Stats>(segments, numThreads, paths);
}

void OpenLRDecoder::DecodeV2(vector<LinearSegment> const & segments, uint32_t const numThreads,
                             vector<DecodedPath> & paths)
{
  Decode<SegmentsDecoderV2, v2::Stats>(segments, numThreads, paths);
}

template <typename Decoder, typename Stats>
void OpenLRDecoder::Decode(vector<LinearSegment> const & segments,
                           uint32_t const numThreads, vector<DecodedPath> & paths)
{
  auto const worker = [&segments, &paths, numThreads, this](size_t threadNum, Index const & index,
                                                            Stats & stat) {
    size_t constexpr kBatchSize = GetOptimalBatchSize();
    size_t constexpr kProgressFrequency = 100;

    size_t const numSegments = segments.size();

    Decoder decoder(index, make_unique<CarModelFactory>(m_countryParentNameGetter));
    my::Timer timer;
    for (size_t i = threadNum * kBatchSize; i < numSegments; i += numThreads * kBatchSize)
    {
      for (size_t j = i; j < numSegments && j < i + kBatchSize; ++j)
      {
        if (!decoder.DecodeSegment(segments[j], paths[j], stat))
          ++stat.m_routesFailed;
        ++stat.m_routesHandled;

        if (stat.m_routesHandled % kProgressFrequency == 0 || i == segments.size() - 1)
        {
          LOG(LINFO, ("Thread", threadNum, "processed", stat.m_routesHandled,
                      "failed:", stat.m_routesFailed));
          timer.Reset();
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

  allStats.Report();
}
}  // namespace openlr
