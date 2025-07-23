#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/fake_ending.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/guides_graph.hpp"
#include "routing/router.hpp"
#include "routing/segment.hpp"

#include "kml/type_utils.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace routing
{
// Information needed to attach guide track to the OSM segments via fake edges.
struct ConnectionToOsm
{
  // Fake ending connecting |m_projectedPoint| on the tracks segment |m_realSegment| to OSM.
  FakeEnding m_fakeEnding;
  // Loop vertex for |m_projectedPoint|.
  FakeVertex m_loopVertex;
  // Segment on the track to which the |m_loopVertex| should be attached via |m_partsOfReal|.
  Segment m_realSegment;
  // Terminal points of |m_realSegment|.
  LatLonWithAltitude m_realFrom;
  LatLonWithAltitude m_realTo;
  // Vertexes and corresponding PartOfReal segments connecting |m_loopVertex| and |m_realSegment|.
  std::vector<std::pair<FakeVertex, Segment>> m_partsOfReal;
  // Projection of the checkpoint to the track.
  geometry::PointWithAltitude m_projectedPoint;
  bool m_fromCheckpoint = false;
};

// Information about checkpoint projection to the nearest guides track.
struct CheckpointTrackProj
{
  CheckpointTrackProj() = default;
  CheckpointTrackProj(kml::MarkGroupId guideId, size_t trackIdx, size_t trackPointIdx,
                      geometry::PointWithAltitude const & projectedPoint, double distToProjectedPointM);
  // Guide id.
  kml::MarkGroupId m_guideId = 0;
  // Index of the track belonging to |m_guideId|.
  size_t m_trackIdx = 0;
  // Index of the nearest segment 'from' or 'to' point on the track to the checkpoint.
  size_t m_trackPointIdx = 0;
  // Projection of the checkpoint to the track.
  geometry::PointWithAltitude m_projectedPoint;
  // Distance between the checkpoint and |m_projectedPoint|
  double m_distToProjectedPointM = 0;
};

using Projections = std::map<size_t, CheckpointTrackProj>;
using IterProjections = Projections::iterator;

// Prepares guides tracks for attaching to the roads graph.
class GuidesConnections
{
public:
  GuidesConnections() = default;
  GuidesConnections(GuidesTracks const & guides);

  // Sets mwm id and speed values for guides graph.
  void SetGuidesGraphParams(NumMwmId mwmId, double maxSpeed);

  // Finds closest guides tracks for checkpoints, fills guides graph.
  void ConnectToGuidesGraph(std::vector<m2::PointD> const & checkpoints);

  // Overwrites osm connections for checkpoint by its index |checkpointIdx|.
  void UpdateOsmConnections(size_t checkpointIdx, std::vector<ConnectionToOsm> const & links);

  // Set |newFakeEnding| for checkpoint.
  void OverwriteFakeEnding(size_t checkpointIdx, FakeEnding const & newFakeEnding);

  // Merge existing |srcFakeEnding| with |dstFakeEnding|.
  static void ExtendFakeEndingProjections(FakeEnding const & srcFakeEnding, FakeEnding & dstFakeEnding);

  // Returns guides graph |m_graph| for index graph starter.
  GuidesGraph const & GetGuidesGraph() const;

  // Returns all connections to the OSM graph relevant to checkpoint with |checkpointIdx| index.
  std::vector<ConnectionToOsm> GetOsmConnections(size_t checkpointIdx) const;

  // Checks if checkpoint is close enough to some track.
  bool FitsForDirectLinkToGuide(size_t checkpointIdx, size_t checkpointsCount) const;

  // Checks if checkpoint is considered to be attached to some track.
  bool IsCheckpointAttached(size_t checkpointIdx) const;

  // Checks if GuidesConnections instance is active.
  bool IsActive() const;

  // Checks if some guides tracks in GuidesConnections instance are attached to the checkpoints.
  bool IsAttached() const;

  // Returns mwm id linked to all tracks in the guides graph.
  NumMwmId GetMwmId() const;

  // Returns fake ending associated with checkpoint by its index |checkpointIdx|.
  FakeEnding GetFakeEnding(size_t checkpointIdx) const;

private:
  // Fills |ConnectionToOsm| for checkpoints for its further attachment to the roads graph.
  void AddConnectionToOsm(size_t checkpointIdx, Segment const & real, geometry::PointWithAltitude const & loop,
                          bool fromCheckpoint);

  // Attaches checkpoints to the nearest guides tracks if possible.
  void PullCheckpointsToTracks(std::vector<m2::PointD> const & checkpoints);

  // Attaches terminal point on the track to the terminal checkpoint: start to start;
  // finish - to finish.
  void AddTerminalGuidePoint(size_t checkpointIdx, size_t neighbourIdx, m2::PointD const & curPoint);

  // Attaches neighbour terminal checkpoints for those which are already attached.
  void PullAdditionalCheckpointsToTracks(std::vector<m2::PointD> const & checkpoints);

  GuidesTracks m_allTracks;
  Projections m_checkpointsOnTracks;

  // Maps with checkpoint indexes as keys.
  std::map<size_t, FakeEnding> m_checkpointsFakeEndings;
  std::map<size_t, std::vector<ConnectionToOsm>> m_connectionsToOsm;

  GuidesGraph m_graph;
};
}  // namespace routing
