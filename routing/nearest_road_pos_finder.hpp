#pragma once

#include "routing/road_graph.hpp"
#include "routing/vehicle_model.hpp"

#include "geometry/point2d.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "std/limits.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class FeatureType;

namespace routing
{
/// Helper functor class to filter nearest RoadPos'es to the given starting point.
class NearestRoadPosFinder
{
  static constexpr uint32_t INVALID_FID = numeric_limits<uint32_t>::max();

  struct Candidate
  {
    double m_dist;
    uint32_t m_segId;
    m2::PointD m_point;
    uint32_t m_fid;
    bool m_isOneway;

    Candidate()
        : m_dist(numeric_limits<double>::max()),
          m_segId(0),
          m_point(m2::PointD::Zero()),
          m_fid(INVALID_FID),
          m_isOneway(false)
    {
    }

    inline bool Valid() const { return m_fid != INVALID_FID; }
  };

  m2::PointD m_point;
  m2::PointD m_direction;
  vector<Candidate> m_candidates;
  IRoadGraph * m_roadGraph;

public:
  NearestRoadPosFinder(m2::PointD const & point, m2::PointD const & direction,
                       IRoadGraph * roadGraph)
      : m_point(point), m_direction(direction), m_roadGraph(roadGraph)
  {
  }

  inline bool HasCandidates() const { return !m_candidates.empty(); }

  void AddInformationSource(uint32_t featureId);

  void MakeResult(vector<RoadPos> & res, size_t const maxCount);
};
}
