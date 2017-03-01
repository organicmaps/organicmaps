#pragma once

#include "routing/road_point.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"

#include "std/cstdint.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"

namespace routing
{
class RoadGeometry final
{
public:
  using Points = buffer_vector<m2::PointD, 32>;

  RoadGeometry() = default;
  RoadGeometry(bool oneWay, double speed, Points const & points);

  void Load(IVehicleModel const & vehicleModel, FeatureType const & feature);

  bool IsOneWay() const { return m_isOneWay; }
  // Kilometers per hour.
  double GetSpeed() const { return m_speed; }
  m2::PointD const & GetPoint(uint32_t pointId) const
  {
    ASSERT_LESS(pointId, m_points.size(), ());
    return m_points[pointId];
  }

  uint32_t GetPointsCount() const { return static_cast<uint32_t>(m_points.size()); }

  // Note. It's possible that car_model was changed after the map was built.
  // For example, the map from 12.2016 contained highway=pedestrian
  // in car_model but this type of highways is removed as of 01.2017.
  // In such cases RoadGeometry is not valid.
  bool IsValid() const { return m_valid; }

  bool IsEndPointId(uint32_t pointId) const
  {
    ASSERT_LESS(pointId, m_points.size(), ());
    return pointId == 0 || pointId + 1 == GetPointsCount();
  }

private:
  Points m_points;
  double m_speed = 0.0;
  bool m_isOneWay = false;
  bool m_valid = false;
};

class GeometryLoader
{
public:
  virtual ~GeometryLoader() = default;

  virtual void Load(uint32_t featureId, RoadGeometry & road) const = 0;

  // mwmId should be alive: it is caller responsibility to check it.
  static unique_ptr<GeometryLoader> Create(Index const & index, MwmSet::MwmId const & mwmId,
                                           shared_ptr<IVehicleModel> vehicleModel);
};

class Geometry final
{
public:
  Geometry() = default;
  explicit Geometry(unique_ptr<GeometryLoader> loader);

  RoadGeometry const & GetRoad(uint32_t featureId);

  m2::PointD const & GetPoint(RoadPoint const & rp)
  {
    return GetRoad(rp.GetFeatureId()).GetPoint(rp.GetPointId());
  }

private:
  // Feature id to RoadGeometry map.
  unordered_map<uint32_t, RoadGeometry> m_roads;
  unique_ptr<GeometryLoader> m_loader;
};
}  // namespace routing
