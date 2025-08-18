#include "routing/guides_connections.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

namespace routing
{
namespace
{
// We consider only really close points to be attached to the track.
double constexpr kMaxDistToTrackPointM = 50.0;

// For points further from track then |kEqDistToTrackPointM| we try to attach them to the OSM
// graph.
double constexpr kEqDistToTrackPointM = 20.0;

// It the checkpoint is further from the guides track than |kMaxDistToTrackForSkippingM|, we skip
// this track completely. For time optimization purposes only.
double constexpr kMaxDistToTrackForSkippingM = 100'000.0;
}  // namespace

CheckpointTrackProj::CheckpointTrackProj(kml::MarkGroupId guideId, size_t trackIdx, size_t trackPointIdx,
                                         geometry::PointWithAltitude const & projectedPoint,
                                         double distToProjectedPointM)
  : m_guideId(guideId)
  , m_trackIdx(trackIdx)
  , m_trackPointIdx(trackPointIdx)
  , m_projectedPoint(projectedPoint)
  , m_distToProjectedPointM(distToProjectedPointM)
{}

std::pair<geometry::PointWithAltitude, double> GetProjectionAndDistOnSegment(
    m2::PointD const & point, geometry::PointWithAltitude const & startPath,
    geometry::PointWithAltitude const & endPath)
{
  m2::PointD const projection =
      m2::ParametrizedSegment<m2::PointD>(startPath.GetPoint(), endPath.GetPoint()).ClosestPointTo(point);
  double const distM = mercator::DistanceOnEarth(projection, point);
  return std::make_pair(geometry::PointWithAltitude(projection, 0 /* altitude */), distM);
}

bool GuidesConnections::IsActive() const
{
  return !m_allTracks.empty();
}

std::vector<ConnectionToOsm> GuidesConnections::GetOsmConnections(size_t checkpointIdx) const
{
  auto it = m_connectionsToOsm.find(checkpointIdx);
  if (it == m_connectionsToOsm.end())
    return {};
  return it->second;
}

void GuidesConnections::UpdateOsmConnections(size_t checkpointIdx, std::vector<ConnectionToOsm> const & links)
{
  auto const it = m_connectionsToOsm.find(checkpointIdx);
  CHECK(it != m_connectionsToOsm.cend(), (checkpointIdx));
  it->second.clear();
  for (auto const & link : links)
    if (!link.m_fakeEnding.m_projections.empty())
      it->second.push_back(link);
  if (it->second.empty())
    m_connectionsToOsm.erase(it);
}

GuidesConnections::GuidesConnections(GuidesTracks const & guides) : m_allTracks(guides) {}

void GuidesConnections::PullCheckpointsToTracks(std::vector<m2::PointD> const & checkpoints)
{
  for (size_t checkpointIdx = 0; checkpointIdx < checkpoints.size(); ++checkpointIdx)
  {
    for (auto const & [guideId, tracks] : m_allTracks)
    {
      for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx)
      {
        CHECK(!tracks[trackIdx].empty(), (trackIdx));
        for (size_t pointIdx = 0; pointIdx < tracks[trackIdx].size() - 1; ++pointIdx)
        {
          auto const [checkpointProj, distM] = GetProjectionAndDistOnSegment(
              checkpoints[checkpointIdx], tracks[trackIdx][pointIdx], tracks[trackIdx][pointIdx + 1]);

          // Skip too far track.
          if (distM > kMaxDistToTrackForSkippingM)
            break;

          // We consider only really close points to be attached to the track.
          if (distM > kMaxDistToTrackPointM)
            continue;

          CheckpointTrackProj const curProj(guideId, trackIdx, pointIdx, checkpointProj, distM);
          auto const [it, inserted] = m_checkpointsOnTracks.emplace(checkpointIdx, curProj);

          if (!inserted && it->second.m_distToProjectedPointM > distM)
            it->second = curProj;
        }
      }
    }
  }
}

void GuidesConnections::AddTerminalGuidePoint(size_t checkpointIdx, size_t neighbourIdx, m2::PointD const & curPoint)
{
  auto const it = m_checkpointsOnTracks.find(neighbourIdx);
  CHECK(it != m_checkpointsOnTracks.cend(), (neighbourIdx));

  auto const & neighbour = it->second;
  auto const guideId = neighbour.m_guideId;
  auto const trackIdx = neighbour.m_trackIdx;
  auto const & track = m_allTracks[guideId][trackIdx];

  CHECK_GREATER(track.size(), 1,
                ("checkpointIdx:", checkpointIdx, "neighbourIdx:", neighbourIdx, "trackIdx:", trackIdx));

  // Connect start checkpoint to the starting point of the track.
  if (checkpointIdx == 0)
  {
    double const distToStartM = mercator::DistanceOnEarth(curPoint, track.front().GetPoint());
    m_checkpointsOnTracks[checkpointIdx] =
        CheckpointTrackProj(guideId, trackIdx, 0 /* trackPointIdx */, track.front() /* proj */, distToStartM);

    return;
  }

  // Connect finish checkpoint to the finish point of the track.
  double const distToSFinishM = mercator::DistanceOnEarth(curPoint, track.back().GetPoint());
  m_checkpointsOnTracks[checkpointIdx] = CheckpointTrackProj(guideId, trackIdx, track.size() - 2 /* trackPointIdx */,
                                                             track.back() /* proj */, distToSFinishM);
}

bool GuidesConnections::IsCheckpointAttached(size_t checkpointIdx) const
{
  return m_checkpointsOnTracks.find(checkpointIdx) != m_checkpointsOnTracks.end();
}

std::vector<size_t> GetNeighbourIntermediatePoints(size_t checkpointIdx, size_t checkpointsCount)
{
  CHECK_GREATER(checkpointsCount, 2, (checkpointsCount));
  std::vector<size_t> neighbours;
  // We add left intermediate point:
  if (checkpointIdx > 1)
    neighbours.push_back(checkpointIdx - 1);
  // We add right intermediate point:
  if (checkpointIdx < checkpointsCount - 2)
    neighbours.push_back(checkpointIdx + 1);
  return neighbours;
}

bool GuidesConnections::FitsForDirectLinkToGuide(size_t checkpointIdx, size_t checkpointsCount) const
{
  auto it = m_checkpointsOnTracks.find(checkpointIdx);
  CHECK(it != m_checkpointsOnTracks.end(), (checkpointIdx));
  bool const checkpointIsFar = it->second.m_distToProjectedPointM > kEqDistToTrackPointM;
  if (checkpointIsFar)
    return false;

  // If checkpoint lies on the track but its neighbour intermediate checkpoint does not, we should
  // connect this checkpoint to the OSM graph.
  if (checkpointsCount <= 2)
    return true;

  for (auto const neighbourIdx : GetNeighbourIntermediatePoints(checkpointIdx, checkpointsCount))
    if (m_checkpointsOnTracks.find(neighbourIdx) == m_checkpointsOnTracks.end())
      return false;

  return true;
}

void GuidesConnections::PullAdditionalCheckpointsToTracks(std::vector<m2::PointD> const & checkpoints)
{
  for (size_t i : {size_t(0), checkpoints.size() - 1})
  {
    // Skip already connected to the tracks checkpoints.
    if (IsCheckpointAttached(i))
      continue;

    // Neighbour point of this terminal point should be on the track.
    size_t neighbourIdx = i == 0 ? i + 1 : i - 1;

    if (!IsCheckpointAttached(neighbourIdx))
      continue;

    AddTerminalGuidePoint(i, neighbourIdx, checkpoints[i]);
  }
}

bool GuidesConnections::IsAttached() const
{
  return !m_checkpointsFakeEndings.empty();
}

FakeEnding GuidesConnections::GetFakeEnding(size_t checkpointIdx) const
{
  if (IsAttached())
  {
    auto it = m_checkpointsFakeEndings.find(checkpointIdx);
    if (it != m_checkpointsFakeEndings.end())
      return it->second;
  }
  return FakeEnding();
}

GuidesGraph const & GuidesConnections::GetGuidesGraph() const
{
  return m_graph;
}

void GuidesConnections::OverwriteFakeEnding(size_t checkpointIdx, FakeEnding const & newFakeEnding)
{
  m_checkpointsFakeEndings[checkpointIdx] = newFakeEnding;
}

// static
void GuidesConnections::ExtendFakeEndingProjections(FakeEnding const & srcFakeEnding, FakeEnding & dstFakeEnding)
{
  dstFakeEnding.m_originJunction = srcFakeEnding.m_originJunction;

  for (auto const & proj : srcFakeEnding.m_projections)
    if (!base::IsExist(dstFakeEnding.m_projections, proj))
      dstFakeEnding.m_projections.push_back(proj);
}

NumMwmId GuidesConnections::GetMwmId() const
{
  return m_graph.GetMwmId();
}

void GuidesConnections::SetGuidesGraphParams(NumMwmId mwmId, double maxSpeed)
{
  m_graph = GuidesGraph(maxSpeed, mwmId);
}

void GuidesConnections::ConnectToGuidesGraph(std::vector<m2::PointD> const & checkpoints)
{
  PullCheckpointsToTracks(checkpoints);

  PullAdditionalCheckpointsToTracks(checkpoints);

  std::map<std::pair<kml::MarkGroupId, size_t>, Segment> addedTracks;

  for (auto const & [checkpointIdx, proj] : m_checkpointsOnTracks)
  {
    auto const & checkpoint = checkpoints[checkpointIdx];
    auto const & checkpointProj = proj.m_projectedPoint;

    Segment segmentOnTrack;
    auto const [it, insertedSegment] =
        addedTracks.emplace(std::make_pair(proj.m_guideId, proj.m_trackIdx), segmentOnTrack);
    if (insertedSegment)
    {
      CHECK(!m_allTracks[proj.m_guideId][proj.m_trackIdx].empty(),
            ("checkpointIdx:", checkpointIdx, "guideId:", proj.m_guideId, "trackIdx:", proj.m_trackIdx));

      segmentOnTrack = m_graph.AddTrack(m_allTracks[proj.m_guideId][proj.m_trackIdx], proj.m_trackPointIdx);
      it->second = segmentOnTrack;
    }
    else
    {
      segmentOnTrack = m_graph.FindSegment(it->second, proj.m_trackPointIdx);
    }
    FakeEnding const newEnding = m_graph.MakeFakeEnding(segmentOnTrack, checkpoint, checkpointProj);
    auto const [itEnding, inserted] = m_checkpointsFakeEndings.emplace(checkpointIdx, newEnding);

    CHECK(inserted, (checkpointIdx));
    bool const fitsForOsmLink = !FitsForDirectLinkToGuide(checkpointIdx, checkpoints.size());

    if (fitsForOsmLink)
      AddConnectionToOsm(checkpointIdx, segmentOnTrack, checkpointProj, true /* fromCheckpoint */);

    // Connect to OSM start and finish points of the track.
    auto const & firstPointOnTrack = m_allTracks[proj.m_guideId][proj.m_trackIdx][0];
    if (!(fitsForOsmLink && firstPointOnTrack == checkpointProj))
    {
      auto const & firstSegmentOnTrack = m_graph.FindSegment(segmentOnTrack, 0);
      AddConnectionToOsm(checkpointIdx, firstSegmentOnTrack, firstPointOnTrack, false /* fromCheckpoint */);
    }

    auto const & lastPointIdx = m_allTracks[proj.m_guideId][proj.m_trackIdx].size() - 1;
    auto const & lastPointOnTrack = m_allTracks[proj.m_guideId][proj.m_trackIdx][lastPointIdx];
    if (!(fitsForOsmLink && lastPointOnTrack == checkpointProj))
    {
      auto const & lastSegmentOnTrack = m_graph.FindSegment(segmentOnTrack, lastPointIdx - 1);
      AddConnectionToOsm(checkpointIdx, lastSegmentOnTrack, lastPointOnTrack, false /* fromCheckpoint */);
    }
  }
}

void GuidesConnections::AddConnectionToOsm(size_t checkpointIdx, Segment const & real,
                                           geometry::PointWithAltitude const & loop, bool fromCheckpoint)
{
  LatLonWithAltitude const loopPoint(mercator::ToLatLon(loop.GetPoint()), loop.GetAltitude());

  ConnectionToOsm link;
  link.m_loopVertex = FakeVertex(kFakeNumMwmId, loopPoint, loopPoint, FakeVertex::Type::PureFake);
  link.m_realSegment = real;
  link.m_projectedPoint = loop;
  link.m_fromCheckpoint = fromCheckpoint;
  std::tie(link.m_realFrom, link.m_realTo) = m_graph.GetFromTo(real);

  m_connectionsToOsm[checkpointIdx].push_back(link);
}
}  // namespace routing
