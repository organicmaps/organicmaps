#pragma once

#include "routing/fseg.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include "std/cstdint.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"

namespace routing
{
class Geometry
{
public:
  virtual ~Geometry() = default;
  virtual bool IsRoad(uint32_t featureId) const = 0;
  virtual bool IsOneWay(uint32_t featureId) const = 0;
  virtual m2::PointD const & GetPoint(FSegId fseg) const = 0;
  virtual double CalcEdgesWeight(uint32_t featureId, uint32_t pointFrom,
                                 uint32_t pointTo) const = 0;
  virtual double CalcHeuristic(FSegId from, FSegId to) const = 0;
};

unique_ptr<Geometry> CreateGeometry(Index const & index, MwmSet::MwmId const & mwmId,
                                    shared_ptr<IVehicleModel> vehicleModel);
}  // namespace routing
