#pragma once

#include "routing/geometry.hpp"
#include "routing/vehicle_model.hpp"

#include "geometry/point2d.hpp"

#include "std/cstdint.hpp"
#include "std/shared_ptr.hpp"

namespace routing
{
class EdgeEstimator
{
public:
  virtual ~EdgeEstimator() = default;

  virtual double CalcEdgesWeight(RoadGeometry const & road, uint32_t pointFrom,
                                 uint32_t pointTo) const = 0;
  virtual double CalcHeuristic(m2::PointD const & from, m2::PointD const & to) const = 0;

  static shared_ptr<EdgeEstimator> CreateForCar(IVehicleModel const & vehicleModel);
};
}  // namespace routing
