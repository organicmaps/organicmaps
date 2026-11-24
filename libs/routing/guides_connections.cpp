#include "routing/guides_connections.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include <optional>

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

bool GuidesConnections::IsActive() const
{
  return !m_allTracks.empty();
}

std::vector<ConnectionToOsm> GuidesConnections::GetOsmConnections(size_t checkpointIdx) const
{
  auto const it = m_connectionsToOsm.find(checkpointIdx);
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

void GuidesConnections::PullCheckpointsToTracks(CheckpointsGeometry const & checkpoints)
{
  for (size_t checkpointIdx = 0; checkpointIdx < checkpoints.size(); ++checkpointIdx)
  {
    auto const & checkpoint = checkpoints[checkpointIdx];
    std::optional<CheckpointTrackProj> closest;
    for (auto const & [guideId, tracks] : m_allTracks)
    {
      for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx)
      {
        auto const & track = tracks[trackIdx];
        CHECK(!track.empty(), (trackIdx));
        for (size_t pointIdx = 0; pointIdx + 1 < track.size(); ++pointIdx)
        {
          auto const & from = track[pointIdx].GetPoint();
          auto const & to = track[pointIdx + 1].GetPoint();
          auto const checkpointProj = m2::ParametrizedSegment<m2::PointD>(from, to).ClosestPointTo(checkpoint);
          double const distM = mercator::DistanceOnEarth(checkpointProj, checkpoint);

          // Skip too far track.
          if (distM > kMaxDistToTrackForSkippingM)
            break;

          // We consider only really close points to be attached to the track.
          if (distM <= kMaxDistToTrackPointM && (!closest || distM < closest->m_distToProjectedPointM))
            closest.emplace(guideId, trackIdx, pointIdx, checkpointProj, distM);
        }
      }
    }
    if (closest)
      m_checkpointsOnTracks.emplace(checkpointIdx, *closest);
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

  bool const isStart = checkpointIdx == 0;
  auto const & projectedPoint = (isStart ? track.front() : track.back()).GetPoint();
  m_checkpointsOnTracks[checkpointIdx] = {guideId, trackIdx, isStart ? 0 : track.size() - 2, projectedPoint,
                                          mercator::DistanceOnEarth(curPoint, projectedPoint)};
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

void GuidesConnections::PullAdditionalCheckpointsToTracks(CheckpointsGeometry const & checkpoints)
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

void GuidesConnections::ConnectToGuidesGraph(CheckpointsGeometry const & checkpoints)
{
  // May be called more than once per route (e.g. primary + alternative pass in
  // IndexRouter::CalculateRoute). Reset the per-calculation state rebuilt below; |m_allTracks| is the
  // input and |m_graph| is reset in SetGuidesGraphParams().
  m_checkpointsOnTracks.clear();
  m_checkpointsFakeEndings.clear();
  m_connectionsToOsm.clear();

  PullCheckpointsToTracks(checkpoints);

  PullAdditionalCheckpointsToTracks(checkpoints);

  std::map<std::pair<kml::MarkGroupId, size_t>, Segment> addedTracks;

  for (auto const & [checkpointIdx, proj] : m_checkpointsOnTracks)
  {
    auto const & checkpoint = checkpoints[checkpointIdx];
    auto const & track = m_allTracks.at(proj.m_guideId)[proj.m_trackIdx];
    auto const & from = track[proj.m_trackPointIdx];
    auto const & to = track[proj.m_trackPointIdx + 1];
    auto const & projectedPoint = proj.m_projectedPoint;
    double const fraction = projectedPoint == from.GetPoint() ? 0.0
                          : projectedPoint == to.GetPoint()
                              ? 1.0
                              : projectedPoint.Length(from.GetPoint()) / to.GetPoint().Length(from.GetPoint());
    geometry::PointWithAltitude const checkpointProj(projectedPoint, from.Interpolate(to, fraction).GetAltitude());

    Segment segmentOnTrack;
    auto const [it, insertedSegment] =
        addedTracks.emplace(std::make_pair(proj.m_guideId, proj.m_trackIdx), segmentOnTrack);
    if (insertedSegment)
    {
      segmentOnTrack = m_graph.AddTrack(track, proj.m_trackPointIdx);
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
    if (!(fitsForOsmLink && track.front().GetPoint() == projectedPoint))
    {
      auto const & firstSegmentOnTrack = m_graph.FindSegment(segmentOnTrack, 0);
      AddConnectionToOsm(checkpointIdx, firstSegmentOnTrack, track.front(), false /* fromCheckpoint */);
    }

    if (!(fitsForOsmLink && track.back().GetPoint() == projectedPoint))
    {
      auto const & lastSegmentOnTrack = m_graph.FindSegment(segmentOnTrack, track.size() - 2);
      AddConnectionToOsm(checkpointIdx, lastSegmentOnTrack, track.back(), false /* fromCheckpoint */);
    }
  }
}

void GuidesConnections::AddConnectionToOsm(size_t checkpointIdx, Segment const & real,
                                           geometry::PointWithAltitude const & loop, bool fromCheckpoint)
{
  LatLonWithAltitude const loopPoint(loop);

  ConnectionToOsm link;
  link.m_loopVertex = FakeVertex(kFakeNumMwmId, loopPoint, loopPoint, FakeVertex::Type::PureFake);
  link.m_realSegment = real;
  link.m_projectedPoint = loop;
  link.m_fromCheckpoint = fromCheckpoint;
  std::tie(link.m_realFrom, link.m_realTo) = m_graph.GetFromTo(real);

  m_connectionsToOsm[checkpointIdx].push_back(link);
}
}  // namespace routing
