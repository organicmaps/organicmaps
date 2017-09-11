#pragma once

#include "routing/road_point.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/feature_altitude.hpp"
#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace routing
{
class RoadGeometry final
{
public:
  using Points = buffer_vector<m2::PointD, 32>;

  RoadGeometry() = default;
  RoadGeometry(bool oneWay, double speed, Points const & points);

  void Load(VehicleModelInterface const & vehicleModel, FeatureType const & feature,
            feature::TAltitudes const * altitudes);

  bool IsOneWay() const { return m_isOneWay; }
  // Kilometers per hour.
  double GetSpeed() const { return m_speed; }
  bool IsTransitAllowed() const { return m_isTransitAllowed; }

  Junction const & GetJunction(uint32_t junctionId) const
  {
    ASSERT_LESS(junctionId, m_junctions.size(), ());
    return m_junctions[junctionId];
  }

  m2::PointD const & GetPoint(uint32_t pointId) const { return GetJunction(pointId).GetPoint(); }

  uint32_t GetPointsCount() const { return static_cast<uint32_t>(m_junctions.size()); }

  // Note. It's possible that car_model was changed after the map was built.
  // For example, the map from 12.2016 contained highway=pedestrian
  // in car_model but this type of highways is removed as of 01.2017.
  // In such cases RoadGeometry is not valid.
  bool IsValid() const { return m_valid; }

  bool IsEndPointId(uint32_t pointId) const
  {
    ASSERT_LESS(pointId, m_junctions.size(), ());
    return pointId == 0 || pointId + 1 == GetPointsCount();
  }

  void SetTransitAllowedForTests(bool transitAllowed) { m_isTransitAllowed = transitAllowed; }

private:
  buffer_vector<Junction, 32> m_junctions;
  double m_speed = 0.0;
  bool m_isOneWay = false;
  bool m_valid = false;
  bool m_isTransitAllowed = false;
};

class GeometryLoader
{
public:
  virtual ~GeometryLoader() = default;

  virtual void Load(uint32_t featureId, RoadGeometry & road) = 0;

  // handle should be alive: it is caller responsibility to check it.
  static std::unique_ptr<GeometryLoader> Create(Index const & index,
                                                MwmSet::MwmHandle const & handle,
                                                std::shared_ptr<VehicleModelInterface> vehicleModel,
                                                bool loadAltitudes);

  /// This is for stand-alone work.
  /// Use in generator_tool and unit tests.
  static std::unique_ptr<GeometryLoader> CreateFromFile(
      std::string const & fileName, std::shared_ptr<VehicleModelInterface> vehicleModel);
};

class Geometry final
{
public:
  Geometry() = default;
  explicit Geometry(std::unique_ptr<GeometryLoader> loader);

  RoadGeometry const & GetRoad(uint32_t featureId);

  m2::PointD const & GetPoint(RoadPoint const & rp)
  {
    return GetRoad(rp.GetFeatureId()).GetPoint(rp.GetPointId());
  }

private:
  // Feature id to RoadGeometry map.
  std::unordered_map<uint32_t, RoadGeometry> m_roads;
  std::unique_ptr<GeometryLoader> m_loader;
};
}  // namespace routing
