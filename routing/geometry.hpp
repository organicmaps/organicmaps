#pragma once

#include "routing/road_point.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/fifo_cache.hpp"

#include <cstdint>
#include <memory>
#include <string>

class DataSource;

namespace routing
{
class RoadGeometry final
{
public:
  using Points = buffer_vector<m2::PointD, 32>;

  RoadGeometry() = default;
  RoadGeometry(bool oneWay, double weightSpeedKMpH, double etaSpeedKMpH, Points const & points);

  void Load(VehicleModelInterface const & vehicleModel, FeatureType & feature,
            feature::TAltitudes const * altitudes);

  bool IsOneWay() const { return m_isOneWay; }
  VehicleModelInterface::SpeedKMpH const & GetSpeed() const { return m_speed; }
  bool IsPassThroughAllowed() const { return m_isPassThroughAllowed; }

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

  void SetPassThroughAllowedForTests(bool passThroughAllowed)
  {
    m_isPassThroughAllowed = passThroughAllowed;
  }

private:
  buffer_vector<Junction, 32> m_junctions;
  VehicleModelInterface::SpeedKMpH m_speed;
  bool m_isOneWay = false;
  bool m_valid = false;
  bool m_isPassThroughAllowed = false;
};

class GeometryLoader
{
public:
  virtual ~GeometryLoader() = default;

  virtual void Load(uint32_t featureId, RoadGeometry & road) = 0;

  // handle should be alive: it is caller responsibility to check it.
  static std::unique_ptr<GeometryLoader> Create(DataSource const & dataSource,
                                                MwmSet::MwmHandle const & handle,
                                                std::shared_ptr<VehicleModelInterface> vehicleModel,
                                                bool loadAltitudes);

  /// This is for stand-alone work.
  /// Use in generator_tool and unit tests.
  static std::unique_ptr<GeometryLoader> CreateFromFile(
      std::string const & fileName, std::shared_ptr<VehicleModelInterface> vehicleModel);
};

/// \brief This class supports loading geometry of roads for routing.
/// \note Loaded information about road geometry is kept in a fixed-size cache |m_featureIdToRoad|.
/// On the other hand methods GetRoad() and GetPoint() return geometry information by reference.
/// The reference may be invalid after the next call of GetRoad() or GetPoint() because the cache
/// item which is referred by returned reference may be evicted. It's done for performance reasons.
class Geometry final
{
public:
  Geometry() = default;
  explicit Geometry(std::unique_ptr<GeometryLoader> loader);

  /// \note The reference returned by the method is valid until the next call of GetRoad()
  /// of GetPoint() methods.
  RoadGeometry const & GetRoad(uint32_t featureId);

  /// \note The reference returned by the method is valid until the next call of GetRoad()
  /// of GetPoint() methods.
  m2::PointD const & GetPoint(RoadPoint const & rp)
  {
    return GetRoad(rp.GetFeatureId()).GetPoint(rp.GetPointId());
  }

private:
  std::unique_ptr<GeometryLoader> m_loader;
  std::unique_ptr<FifoCache<uint32_t, RoadGeometry>> m_featureIdToRoad;
};
}  // namespace routing
