#include "openlr/openlr_simple_decoder.hpp"

#include "openlr/openlr_model.hpp"
#include "openlr/openlr_sample.hpp"
#include "openlr/openlr_simple_parser.hpp"
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

#include "std/algorithm.hpp"
#include "std/fstream.hpp"
#include "std/thread.hpp"

using namespace routing;

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
    m_total += rhs.m_total;
  }

  uint32_t m_shortRoutes = 0;
  uint32_t m_zeroCanditates = 0;
  uint32_t m_moreThanOneCandidate = 0;
  uint32_t m_routeIsNotCalculated = 0;
  uint32_t m_total = 0;
};

openlr::SamplePool MakeSamplePool(std::vector<LinearSegment> const & segments,
                                  std::vector<IRoadGraph::TEdgeVector> const & paths)
{
  openlr::SamplePool pool;
  for (size_t i = 0; i < segments.size(); ++i)
  {
    auto const & segment = segments[i];
    auto const & path = paths[i];

    if (path.empty())
      continue;

    pool.emplace_back(openlr::PartnerSegmentId(segment.m_segmentId));
    auto & sampleItem = pool.back();

    for (auto const & edge : path)
    {
      CHECK(!edge.IsFake(), ("There should be no fake edges in the path."));

      sampleItem.m_segments.emplace_back(
          edge.GetFeatureId(), edge.GetSegId(), edge.IsForward(),
          MercatorBounds::DistanceOnEarth(edge.GetStartJunction().GetPoint(),
                                          edge.GetEndJunction().GetPoint()));
    }
  }
  return pool;
}
}  // namespace

// OpenLRSimpleDecoder::SegmentsFilter -------------------------------------------------------------
OpenLRSimpleDecoder::SegmentsFilter::SegmentsFilter(string const & idsPath,
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

bool OpenLRSimpleDecoder::SegmentsFilter::Matches(LinearSegment const & segment) const
{
  if (m_multipointsOnly && segment.m_locationReference.m_points.size() == 2)
    return false;
  if (m_idsSet && m_ids.count(segment.m_segmentId) == 0)
    return false;
  return true;
}

// OpenLRSimpleDecoder -----------------------------------------------------------------------------
// static
int const OpenLRSimpleDecoder::kHandleAllSegments = -1;

OpenLRSimpleDecoder::OpenLRSimpleDecoder(string const & dataFilename, vector<Index> const & indexes)
  : m_indexes(indexes)
{
  auto const load_result = m_document.load_file(dataFilename.data());
  if (!load_result)
    MYTHROW(DecoderError, ("Can't load file", dataFilename, ":", load_result.description()));
}

void OpenLRSimpleDecoder::Decode(string const & outputFilename, int const segmentsToHandle,
                                 SegmentsFilter const & filter, int const numThreads)
{
  // TODO(mgsergio): Feed segments directly to the decoder. Parsing should not
  // take place inside decoder process.
  vector<LinearSegment> segments;
  if (!ParseOpenlr(m_document, segments))
    MYTHROW(DecoderError, ("Can't parse data."));

  my::EraseIf(segments,
              [&filter](LinearSegment const & segment) { return !filter.Matches(segment); });

  if (segmentsToHandle != kHandleAllSegments && segmentsToHandle < segments.size())
    segments.resize(segmentsToHandle);

  sort(segments.begin(), segments.end(), my::LessBy(&LinearSegment::m_segmentId));

  vector<IRoadGraph::TEdgeVector> paths(segments.size());

  // This code computes the most optimal (in the sense of cache lines
  // occupancy) batch size.
  size_t constexpr a = my::LCM(sizeof(LinearSegment), kCacheLineSize) / sizeof(LinearSegment);
  size_t constexpr b =
      my::LCM(sizeof(IRoadGraph::TEdgeVector), kCacheLineSize) / sizeof(IRoadGraph::TEdgeVector);
  size_t constexpr kBatchSize = my::LCM(a, b);
  size_t constexpr kProgressFrequency = 100;

  auto worker = [&segments, &paths, kBatchSize, kProgressFrequency, numThreads, this](
      size_t threadNum, Index const & index, Stats & stats) {
    FeaturesRoadGraph roadGraph(index, IRoadGraph::Mode::ObeyOnewayTag,
                                make_unique<CarModelFactory>());
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
        }

        auto & path = paths[j];
        if (!router.Go(points, positiveOffsetM, negativeOffsetM, path))
          ++stats.m_routeIsNotCalculated;

        ++stats.m_total;

        if (stats.m_total % kProgressFrequency == 0)
          LOG(LINFO, ("Thread", threadNum, "processed:", stats.m_total));
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

  auto const samplePool = MakeSamplePool(segments, paths);
  SaveSamplePool(outputFilename, samplePool, false /* saveEvaluation */);

  Stats allStats;
  for (auto const & s : stats)
    allStats.Add(s);

  LOG(LINFO, ("Parsed segments:", allStats.m_total,
              "Routes failed:", allStats.m_routeIsNotCalculated,
              "Short routes:", allStats.m_shortRoutes,
              "Ambiguous routes:", allStats.m_moreThanOneCandidate,
              "Path is not reconstructed:", allStats.m_zeroCanditates));
}
}  // namespace openlr
