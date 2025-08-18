#include "track_analyzing/track_matcher.hpp"

#include "track_analyzing/exceptions.hpp"

#include "routing/index_graph_loader.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/scales.hpp"

#include "geometry/parametrized_segment.hpp"

#include "base/stl_helpers.hpp"

namespace track_analyzing
{
using namespace routing;
using namespace std;
using namespace track_analyzing;

namespace
{
// Matching range in meters.
double constexpr kMatchingRange = 20.0;

// Mercator distance from segment to point in meters.
double DistanceToSegment(m2::PointD const & segmentBegin, m2::PointD const & segmentEnd, m2::PointD const & point)
{
  m2::ParametrizedSegment<m2::PointD> const segment(segmentBegin, segmentEnd);
  m2::PointD const projectionPoint = segment.ClosestPointTo(point);
  return mercator::DistanceOnEarth(point, projectionPoint);
}

double DistanceToSegment(Segment const & segment, m2::PointD const & point, IndexGraph & indexGraph)
{
  auto const & road = indexGraph.GetRoadGeometry(segment.GetFeatureId());
  return DistanceToSegment(mercator::FromLatLon(road.GetPoint(segment.GetPointId(false))),
                           mercator::FromLatLon(road.GetPoint(segment.GetPointId(true))), point);
}

bool EdgesContain(IndexGraph::SegmentEdgeListT const & edges, Segment const & segment)
{
  for (auto const & edge : edges)
    if (edge.GetTarget() == segment)
      return true;

  return false;
}
}  // namespace

// TrackMatcher ------------------------------------------------------------------------------------
TrackMatcher::TrackMatcher(storage::Storage const & storage, NumMwmId mwmId, platform::CountryFile const & countryFile)
  : m_mwmId(mwmId)
  , m_vehicleModel(CarModelFactory({}).GetVehicleModelForCountry(countryFile.GetName()))
{
  auto localCountryFile = storage.GetLatestLocalFile(countryFile);
  CHECK(localCountryFile, ("Can't find latest country file for", countryFile.GetName()));
  auto registerResult = m_dataSource.Register(*localCountryFile);
  CHECK_EQUAL(registerResult.second, MwmSet::RegResult::Success, ("Can't register mwm", countryFile.GetName()));

  MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(countryFile);

  m_graph = make_unique<IndexGraph>(
      make_shared<Geometry>(GeometryLoader::Create(handle, m_vehicleModel, false /* loadAltitudes */)),
      EdgeEstimator::Create(VehicleType::Car, *m_vehicleModel, nullptr /* trafficStash */, nullptr /* dataSource */,
                            nullptr /* numMvmIds */));

  DeserializeIndexGraph(*handle.GetValue(), VehicleType::Car, *m_graph);
}

void TrackMatcher::MatchTrack(vector<DataPoint> const & track, vector<MatchedTrack> & matchedTracks)
{
  m_pointsCount += track.size();

  vector<Step> steps;
  steps.reserve(track.size());
  for (auto const & routePoint : track)
    steps.emplace_back(routePoint);

  for (size_t trackBegin = 0; trackBegin < steps.size();)
  {
    for (; trackBegin < steps.size(); ++trackBegin)
    {
      steps[trackBegin].FillCandidatesWithNearbySegments(m_dataSource, *m_graph, *m_vehicleModel, m_mwmId);
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
      Step const & prevStep = steps[trackEnd];
      nextStep.FillCandidates(prevStep, *m_graph);
      if (!nextStep.HasCandidates())
        break;
    }

    steps[trackEnd].ChooseNearestSegment();

    for (size_t i = trackEnd; i > trackBegin; --i)
      steps[i - 1].ChooseSegment(steps[i], *m_graph);

    ++m_tracksCount;

    matchedTracks.push_back({});
    MatchedTrack & matchedTrack = matchedTracks.back();
    for (size_t i = trackBegin; i <= trackEnd; ++i)
    {
      Step const & step = steps[i];
      matchedTrack.emplace_back(step.GetDataPoint(), step.GetSegment());
    }

    trackBegin = trackEnd + 1;
  }
}

// TrackMatcher::Step ------------------------------------------------------------------------------
TrackMatcher::Step::Step(DataPoint const & dataPoint)
  : m_dataPoint(dataPoint)
  , m_point(mercator::FromLatLon(dataPoint.m_latLon))
{}

void TrackMatcher::Step::FillCandidatesWithNearbySegments(DataSource const & dataSource, IndexGraph const & graph,
                                                          VehicleModelInterface const & vehicleModel, NumMwmId mwmId)
{
  dataSource.ForEachInRect([&](FeatureType & ft)
  {
    if (!ft.GetID().IsValid())
      return;

    if (ft.GetID().m_mwmId.GetInfo()->GetType() != MwmInfo::COUNTRY)
      return;

    feature::TypesHolder const types(ft);
    if (!vehicleModel.IsRoad(types))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    for (size_t segIdx = 0; segIdx + 1 < ft.GetPointsCount(); ++segIdx)
    {
      double const distance = DistanceToSegment(ft.GetPoint(segIdx), ft.GetPoint(segIdx + 1), m_point);
      if (distance < kMatchingRange)
      {
        AddCandidate(Segment(mwmId, ft.GetID().m_index, static_cast<uint32_t>(segIdx), true /* forward */), distance,
                     graph);

        if (!vehicleModel.IsOneWay(types))
        {
          AddCandidate(Segment(mwmId, ft.GetID().m_index, static_cast<uint32_t>(segIdx), false /* forward */), distance,
                       graph);
        }
      }
    }
  }, mercator::RectByCenterXYAndSizeInMeters(m_point, kMatchingRange), scales::GetUpperScale());
}

void TrackMatcher::Step::FillCandidates(Step const & previousStep, IndexGraph & graph)
{
  IndexGraph::SegmentEdgeListT edges;

  for (Candidate const & candidate : previousStep.m_candidates)
  {
    Segment const & segment = candidate.GetSegment();
    m_candidates.emplace_back(segment, DistanceToSegment(segment, m_point, graph));

    edges.clear();
    graph.GetEdgeList(segment, true /* isOutgoing */, true /* useRoutingOptions */, edges);

    for (SegmentEdge const & edge : edges)
    {
      Segment const & target = edge.GetTarget();
      if (!segment.IsInverse(target))
        m_candidates.emplace_back(target, DistanceToSegment(target, m_point, graph));
    }
  }

  base::SortUnique(m_candidates);

  m_candidates.erase(remove_if(m_candidates.begin(), m_candidates.end(),
                               [&](Candidate const & candidate) { return candidate.GetDistance() > kMatchingRange; }),
                     m_candidates.end());
}

void TrackMatcher::Step::ChooseSegment(Step const & nextStep, IndexGraph & indexGraph)
{
  CHECK(!m_candidates.empty(), ());

  double minDistance = numeric_limits<double>::max();

  IndexGraph::SegmentEdgeListT edges;
  indexGraph.GetEdgeList(nextStep.m_segment, false /* isOutgoing */, true /* useRoutingOptions */, edges);
  edges.emplace_back(nextStep.m_segment, GetAStarWeightZero<RouteWeight>());

  for (Candidate const & candidate : m_candidates)
  {
    if (candidate.GetDistance() < minDistance && EdgesContain(edges, candidate.GetSegment()))
    {
      minDistance = candidate.GetDistance();
      m_segment = candidate.GetSegment();
    }
  }

  if (minDistance == numeric_limits<double>::max())
    MYTHROW(MessageException, ("Can't find previous step for", nextStep.m_segment));
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

void TrackMatcher::Step::AddCandidate(Segment const & segment, double distance, IndexGraph const & graph)
{
  if (graph.GetAccessType(segment) == RoadAccess::Type::Yes)
    m_candidates.emplace_back(segment, distance);
}
}  // namespace track_analyzing
