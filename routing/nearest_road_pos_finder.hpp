#pragma once

#include "road_graph.hpp"
#include "vehicle_model.hpp"

#include "../geometry/point2d.hpp"

#include "../indexer/index.hpp"
#include "../indexer/mwm_set.hpp"

#include "../std/unique_ptr.hpp"
#include "../std/vector.hpp"

class FeatureType;

namespace routing
{
/// Helper functor class to filter nearest RoadPos'es to the given starting point.
class NearestRoadPosFinder
{
  static constexpr uint32_t INVALID_FID = numeric_limits<uint32_t>::infinity();

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
  };

  m2::PointD m_point;
  m2::PointD m_direction;
  unique_ptr<IVehicleModel> const & m_vehicleModel;
  vector<Candidate> m_candidates;
  MwmSet::MwmId m_mwmId;

public:
  NearestRoadPosFinder(m2::PointD const & point, m2::PointD const & direction,
                unique_ptr<IVehicleModel> const & vehicleModel)
      : m_point(point),
        m_direction(direction),
        m_vehicleModel(vehicleModel),
        m_mwmId(MwmSet::INVALID_MWM_ID)
  {
  }

  inline bool HasCandidates() const { return !m_candidates.empty(); }

  void AddInformationSource(FeatureType const & ft);

  void MakeResult(vector<RoadPos> & res, size_t const maxCount);

  inline MwmSet::MwmId GetMwmId() const { return m_mwmId; }
};
}
