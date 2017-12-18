#include "openlr/openlr_decoder.hpp"

#include "openlr/decoded_path.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/road_info_getter.hpp"
#include "openlr/router.hpp"
#include "openlr/way_point.hpp"

#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/index.hpp"

#include "geometry/polyline2d.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include <fstream>
#include <thread>

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
OpenLRDecoder::OpenLRDecoder(
    vector<Index> const & indexes, CountryParentNameGetterFn const & countryParentNameGetterFn)
  : m_indexes(indexes), m_countryParentNameGetterFn(countryParentNameGetterFn)
{
}

void OpenLRDecoder::Decode(vector<LinearSegment> const & segments, uint32_t const numThreads,
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
                                make_unique<CarModelFactory>(m_countryParentNameGetterFn));
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
          LOG(LINFO, ("Thread", threadNum, "processed:", stats.m_total, "failed:",
                      stats.m_routeIsNotCalculated));
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
}  // namespace openlr
