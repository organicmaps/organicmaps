#pragma once

#include "routing/road_graph.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/feature_altitude.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace routing
{
using IsEdgeProjGood = std::function<bool(std::pair<Edge, geometry::PointWithAltitude> const &)>;

/// Helper functor class to filter nearest roads to the given starting point.
/// Class returns pairs of outgoing edge and projection point on the edge
class NearestEdgeFinder
{
public:
  NearestEdgeFinder(m2::PointD const & point, IsEdgeProjGood const & isEdgeProjGood);

  inline bool HasCandidates() const { return !m_candidates.empty(); }

  void AddInformationSource(IRoadGraph::FullRoadInfo const & roadInfo);

  using EdgeProjectionT = std::pair<Edge, geometry::PointWithAltitude>;
  void MakeResult(std::vector<EdgeProjectionT> & res, size_t maxCountFeatures);

private:
  struct Candidate
  {
    static auto constexpr kInvalidSegmentId = std::numeric_limits<uint32_t>::max();

    double m_squaredDist = std::numeric_limits<double>::max();
    uint32_t m_segId = kInvalidSegmentId;
    geometry::PointWithAltitude m_segStart;
    geometry::PointWithAltitude m_segEnd;
    geometry::PointWithAltitude m_projPoint;
    FeatureID m_fid;
    bool m_bidirectional = true;
  };

  void AddResIf(Candidate const & candidate, bool forward, size_t maxCountFeatures,
                std::vector<EdgeProjectionT> & res) const;
  void CandidateToResult(Candidate const & candidate, size_t maxCountFeatures,
                         std::vector<EdgeProjectionT> & res) const;

  m2::PointD const m_point;
  std::vector<Candidate> m_candidates;
  IsEdgeProjGood m_isEdgeProjGood;
};
}  // namespace routing
