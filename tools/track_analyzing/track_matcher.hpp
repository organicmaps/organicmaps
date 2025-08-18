#pragma once

#include "track_analyzing/track.hpp"

#include "routing/index_graph.hpp"
#include "routing/segment.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "indexer/data_source.hpp"

#include <storage/storage.hpp>

#include "geometry/point2d.hpp"

#include <memory>
#include <string>
#include <vector>

namespace track_analyzing
{
class TrackMatcher final
{
public:
  TrackMatcher(storage::Storage const & storage, routing::NumMwmId mwmId, platform::CountryFile const & countryFile);

  void MatchTrack(std::vector<DataPoint> const & track, std::vector<MatchedTrack> & matchedTracks);

  uint64_t GetTracksCount() const { return m_tracksCount; }
  uint64_t GetPointsCount() const { return m_pointsCount; }
  uint64_t GetNonMatchedPointsCount() const { return m_nonMatchedPointsCount; }

private:
  class Candidate final
  {
  public:
    Candidate(routing::Segment segment, double distance) : m_segment(segment), m_distance(distance) {}

    routing::Segment const & GetSegment() const { return m_segment; }
    double GetDistance() const { return m_distance; }
    bool operator==(Candidate const & candidate) const { return m_segment == candidate.m_segment; }
    bool operator<(Candidate const & candidate) const { return m_segment < candidate.m_segment; }

  private:
    routing::Segment m_segment;
    double m_distance;
  };

  class Step final
  {
  public:
    explicit Step(DataPoint const & dataPoint);

    DataPoint const & GetDataPoint() const { return m_dataPoint; }
    routing::Segment const & GetSegment() const { return m_segment; }
    bool HasCandidates() const { return !m_candidates.empty(); }
    void FillCandidatesWithNearbySegments(DataSource const & dataSource, routing::IndexGraph const & graph,
                                          routing::VehicleModelInterface const & vehicleModel, routing::NumMwmId mwmId);
    void FillCandidates(Step const & previousStep, routing::IndexGraph & graph);
    void ChooseSegment(Step const & nextStep, routing::IndexGraph & indexGraph);
    void ChooseNearestSegment();

  private:
    void AddCandidate(routing::Segment const & segment, double distance, routing::IndexGraph const & graph);

    DataPoint m_dataPoint;
    m2::PointD m_point;
    routing::Segment m_segment;
    std::vector<Candidate> m_candidates;
  };

  routing::NumMwmId const m_mwmId;
  FrozenDataSource m_dataSource;
  std::shared_ptr<routing::VehicleModelInterface> m_vehicleModel;
  std::unique_ptr<routing::IndexGraph> m_graph;
  uint64_t m_tracksCount = 0;
  uint64_t m_pointsCount = 0;
  uint64_t m_nonMatchedPointsCount = 0;
};
}  // namespace track_analyzing
