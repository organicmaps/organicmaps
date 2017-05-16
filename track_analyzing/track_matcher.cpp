#include "track_analyzing/track_matcher.hpp"

#include <routing/index_graph_loader.hpp>

#include <routing_common/car_model.hpp>

#include <indexer/scales.hpp>

#include <geometry/distance.hpp>

using namespace tracking;
using namespace std;

namespace
{
double constexpr kMatchingRange = 20.0;
uint64_t constexpr kShortTrackDuration = 60;

double DistanceToSegment(m2::PointD const & segmentBegin, m2::PointD const & segmentEnd,
                         m2::PointD const & point)
{
  m2::ProjectionToSection<m2::PointD> projection;
  projection.SetBounds(segmentBegin, segmentEnd);
  m2::PointD const projectionPoint = projection(point);
  return MercatorBounds::DistanceOnEarth(point, projectionPoint);
}

double DistanceToSegment(routing::Segment const & segment, m2::PointD const & point,
                         routing::IndexGraph & indexGraph)
{
  return DistanceToSegment(indexGraph.GetGeometry().GetPoint(segment.GetRoadPoint(false)),
                           indexGraph.GetGeometry().GetPoint(segment.GetRoadPoint(true)), point);
}

bool EdgesContain(vector<routing::SegmentEdge> const & edges, routing::Segment const & segment)
{
  for (auto const & edge : edges)
  {
    if (edge.GetTarget() == segment)
      return true;
  }

  return false;
}
}  // namespace

namespace tracking
{
// TrackMatcher ------------------------------------------------------------------------------------
TrackMatcher::TrackMatcher(storage::Storage const & storage, routing::NumMwmId mwmId,
                           platform::CountryFile const & countryFile)
  : m_mwmId(mwmId)
  , m_vehicleModel(routing::CarModelFactory().GetVehicleModelForCountry(countryFile.GetName()))
{
  auto localCountryFile = storage.GetLatestLocalFile(countryFile);
  CHECK(localCountryFile, ("Can't find latest country file for", countryFile.GetName()));
  auto registerResult = m_index.Register(*localCountryFile);
  CHECK_EQUAL(registerResult.second, MwmSet::RegResult::Success,
              ("Can't register mwm", countryFile.GetName()));

  m_graph = make_unique<routing::IndexGraph>(
      routing::GeometryLoader::Create(m_index, registerResult.first, m_vehicleModel),
      routing::EdgeEstimator::CreateForCar(nullptr /* trafficStash */,
                                           m_vehicleModel->GetMaxSpeed()));

  MwmSet::MwmHandle const handle = m_index.GetMwmHandleByCountryFile(countryFile);
  routing::DeserializeIndexGraph(*handle.GetValue<MwmValue>(), *m_graph);
}

void TrackMatcher::MatchTrack(vector<DataPoint> const & track, vector<MatchedTrack> & matchedTracks)
{
  m_pointsCount += track.size();

  vector<Step> steps;
  for (auto const & routePoint : track)
    steps.emplace_back(routePoint);

  for (size_t trackBegin = 0; trackBegin < steps.size();)
  {
    for (; trackBegin < steps.size(); ++trackBegin)
    {
      steps[trackBegin].FillCandidatesWithNearbySegments(m_index, *m_vehicleModel, m_mwmId);
      if (steps[trackBegin].HasCandidates())
        break;

      ++m_nonMatchedPointsCount;
    }

    if (trackBegin >= steps.size())
      break;

    size_t trackEnd = trackBegin;
    for (; trackEnd < steps.size() - 1; ++trackEnd)
    {
      Step & nextStep = steps[trackEnd + 1];
      nextStep.FillCandidates(steps[trackEnd], *m_graph);
      if (!nextStep.HasCandidates())
        break;
    }

    steps[trackEnd].ChooseNearestSegment();

    for (size_t i = trackEnd; i > trackBegin; --i)
      steps[i - 1].ChooseSegment(steps[i], *m_graph);

    ++m_tracksCount;

    uint64_t const trackTime =
        steps[trackEnd].GetDataPoint().m_timestamp - steps[trackBegin].GetDataPoint().m_timestamp;
    if (trackTime < kShortTrackDuration)
    {
      ++m_shortTracksCount;
      m_shortTrackPointsCount += trackEnd + 1 - trackBegin;
    }
    else
    {
      matchedTracks.push_back({});
      MatchedTrack & matchedTrack = matchedTracks.back();
      for (size_t i = trackBegin; i <= trackEnd; ++i)
      {
        Step const & step = steps[i];
        matchedTrack.emplace_back(step.GetDataPoint(), step.GetSegment());
      }
    }

    trackBegin = trackEnd + 1;
  }
}

// TrackMatcher::Step ------------------------------------------------------------------------------
TrackMatcher::Step::Step(DataPoint const & dataPoint)
  : m_dataPoint(dataPoint), m_point(MercatorBounds::FromLatLon(dataPoint.m_latLon))
{
}

void TrackMatcher::Step::FillCandidatesWithNearbySegments(
    Index const & index, routing::IVehicleModel const & vehicleModel, routing::NumMwmId mwmId)
{
  index.ForEachInRect(
      [&](FeatureType const & ft) {
        if (!ft.GetID().IsValid())
          return;

        if (ft.GetID().m_mwmId.GetInfo()->GetType() != MwmInfo::COUNTRY)
          return;

        if (!vehicleModel.IsRoad(ft))
          return;

        ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

        for (size_t segIdx = 0; segIdx + 1 < ft.GetPointsCount(); ++segIdx)
        {
          double const distance =
              DistanceToSegment(ft.GetPoint(segIdx), ft.GetPoint(segIdx + 1), m_point);
          if (distance < kMatchingRange)
          {
            m_candidates.emplace_back(
                routing::Segment(mwmId, ft.GetID().m_index, static_cast<uint32_t>(segIdx), true),
                distance);

            if (!vehicleModel.IsOneWay(ft))
              m_candidates.emplace_back(
                  routing::Segment(mwmId, ft.GetID().m_index, static_cast<uint32_t>(segIdx), false),
                  distance);
          }
        }
      },
      MercatorBounds::RectByCenterXYAndSizeInMeters(m_point, kMatchingRange),
      scales::GetUpperScale());
}

void TrackMatcher::Step::FillCandidates(Step const & previousStep, routing::IndexGraph & graph)
{
  vector<routing::SegmentEdge> edges;

  for (Candidate const & candidate : previousStep.m_candidates)
  {
    routing::Segment const & segment = candidate.GetSegment();
    m_candidates.emplace_back(segment, DistanceToSegment(segment, m_point, graph));

    edges.clear();
    graph.GetEdgeList(segment, true, edges);

    for (routing::SegmentEdge const & edge : edges)
    {
      routing::Segment const & target = edge.GetTarget();
      if (!segment.IsInverse(target))
        m_candidates.emplace_back(target, DistanceToSegment(target, m_point, graph));
    }
  }

  my::SortUnique(m_candidates);

  m_candidates.erase(remove_if(m_candidates.begin(), m_candidates.end(),
                               [&](Candidate const & candidate) {
                                 return candidate.GetDistance() > kMatchingRange;
                               }),
                     m_candidates.end());
}

void TrackMatcher::Step::ChooseSegment(Step const & nextStep, routing::IndexGraph & indexGraph)
{
  CHECK(!m_candidates.empty(), ());

  double minDistance = numeric_limits<double>::max();

  vector<routing::SegmentEdge> edges;
  indexGraph.GetEdgeList(nextStep.m_segment, false, edges);
  edges.emplace_back(nextStep.m_segment, 0.0 /* weight */);

  for (Candidate const & candidate : m_candidates)
  {
    if (candidate.GetDistance() < minDistance && EdgesContain(edges, candidate.GetSegment()))
    {
      minDistance = candidate.GetDistance();
      m_segment = candidate.GetSegment();
    }
  }

  CHECK_LESS(minDistance, numeric_limits<double>::max(),
             ("Can't find previous segment for", nextStep.m_segment));
}

void TrackMatcher::Step::ChooseNearestSegment()
{
  CHECK(!m_candidates.empty(), ());

  double minDistance = numeric_limits<double>::max();

  for (Candidate const & candidate : m_candidates)
  {
    if (candidate.GetDistance() < minDistance)
    {
      minDistance = candidate.GetDistance();
      m_segment = candidate.GetSegment();
    }
  }
}
}  // namespace tracking
