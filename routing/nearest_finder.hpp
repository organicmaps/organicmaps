#pragma once

#include "road_graph.hpp"
#include "vehicle_model.hpp"

#include "../geometry/distance.hpp"
#include "../geometry/point2d.hpp"

#include "../indexer/index.hpp"

#include "../std/unique_ptr.hpp"

namespace routing
{

class NearestFinder
{
  static constexpr uint32_t INVALID_FID = numeric_limits<uint32_t>::max();

  struct Candidate
  {
    double m_dist;
    uint32_t m_segIdx;
    m2::PointD m_point;
    uint32_t m_fid;
    bool m_isOneway;
    Candidate() : m_dist(numeric_limits<double>::max()), m_fid(INVALID_FID) {}
  };

  m2::PointD m_point;
  m2::PointD m_direction;
  unique_ptr<IVehicleModel> const & m_vehicleModel;
  buffer_vector<Candidate, 128> m_candidates;
  uint32_t m_mwmId;

public:
  NearestFinder(m2::PointD const & point,
                m2::PointD const & direction,
                unique_ptr<IVehicleModel> const & vehicleModel)
    : m_point(point), m_direction(direction), m_vehicleModel(vehicleModel),
      m_mwmId(numeric_limits<uint32_t>::max())
  {}

  bool HasCandidates() const { return !m_candidates.empty(); }

  void operator() (FeatureType const & ft);

  void MakeResult(vector<RoadPos> & res, size_t const maxCount);

  uint32_t GetMwmID() {return m_mwmId;}

};

}
