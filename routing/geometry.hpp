#pragma once

#include "routing/road_point.hpp"
#include "routing/vehicle_model.hpp"

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

  bool IsRoad() const { return m_isRoad; }

  bool IsOneWay() const { return m_isOneWay; }

  bool GetSpeed() const { return m_speed; }

  m2::PointD const & GetPoint(uint32_t pointId) const
  {
    ASSERT_LESS(pointId, m_points.size(), ());
    return m_points[pointId];
  }

  uint32_t GetPointsCount() const { return m_points.size(); }

private:
  bool m_isRoad = false;
  bool m_isOneWay = false;
  double m_speed = 0.0;
  Points m_points;
};

class GeometryLoader
{
public:
  virtual ~GeometryLoader() = default;

  virtual void Load(uint32_t featureId, RoadGeometry & road) const = 0;
};

class Geometry final
{
public:
  Geometry() = default;
  explicit Geometry(unique_ptr<GeometryLoader> loader);

  RoadGeometry const & GetRoad(uint32_t featureId) const
  {
    auto const & it = m_roads.find(featureId);
    if (it != m_roads.cend())
      return it->second;

    RoadGeometry & road = m_roads[featureId];
    m_loader->Load(featureId, road);
    return road;
  }

  m2::PointD const & GetPoint(RoadPoint const & rp) const
  {
    return GetRoad(rp.GetFeatureId()).GetPoint(rp.GetPointId());
  }

private:
  // Feature id to RoadGeometry map.
  mutable unordered_map<uint32_t, RoadGeometry> m_roads;
  unique_ptr<GeometryLoader> m_loader;
};

unique_ptr<GeometryLoader> CreateGeometryLoader(Index const & index, MwmSet::MwmId const & mwmId,
                                                shared_ptr<IVehicleModel> vehicleModel);
}  // namespace routing
