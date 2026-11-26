#pragma once

#include "routing/vehicle_mask.hpp"

#include "base/small_map.hpp"

#include <optional>
#include <string>
#include <type_traits>

namespace routing
{
enum class RouterType;

class RoutingOptions
{
public:
  enum Road : uint8_t
  {
    Usual = 1u << 0,
    Toll = 1u << 1,
    Motorway = 1u << 2,
    Ferry = 1u << 3,
    Dirty = 1u << 4,
    Steps = 1u << 5,
    DifficultTrail = 1u << 6,  // The walking avoidance mask uses this bit for both trail grades.
    ExpertTrail = 1u << 7,     // Last free bit in RoadType.

    Max = ExpertTrail + 1
  };

  using RoadType = std::underlying_type_t<Road>;

  RoutingOptions() = default;
  explicit RoutingOptions(RoadType mask) : m_options(mask) {}

  static RoutingOptions LoadFromSettings(VehicleType vehicleType);
  static void SaveToSettings(VehicleType vehicleType, RoutingOptions options);
  static RoadType GetSupportedOptions(VehicleType vehicleType);
  static std::optional<VehicleType> GetVehicleType(RouterType routerType);
  static std::optional<Road> RoadFromId(uint32_t id);

  static bool LoadRouteOptimizationFromSettings();
  static void SaveRouteOptimizationToSettings(bool enabled);

  void Add(Road type);
  void Remove(Road type);
  bool Has(Road type) const;

  RoadType GetOptions() const { return m_options; }
  bool operator==(RoutingOptions const &) const = default;

private:
  RoadType m_options = 0;
};

enum class BicycleSurfaceBias : uint8_t
{
  Any = 0,
  PreferPaved = 1,
  PreferUnpaved = 2
};

// Match signed platform inputs without narrowing before validation.
enum class BicycleSurfacePreference : int64_t
{
  Any = static_cast<int64_t>(BicycleSurfaceBias::Any),
  PreferPaved = static_cast<int64_t>(BicycleSurfaceBias::PreferPaved),
  PreferUnpaved = static_cast<int64_t>(BicycleSurfaceBias::PreferUnpaved),
  AvoidUnpaved = 3
};

struct RoutePlanningOptions
{
  RoutingOptions m_avoid;
  BicycleSurfaceBias m_bicycleSurfaceBias = BicycleSurfaceBias::Any;

  static RoutePlanningOptions LoadFromSettings(VehicleType vehicle);
  static BicycleSurfacePreference LoadBicycleSurfacePreference();
  static void SaveBicycleSurfacePreference(BicycleSurfacePreference preference);
  bool operator==(RoutePlanningOptions const &) const = default;
};

class RoutingOptionsClassifier
{
public:
  std::optional<RoutingOptions::Road> Get(uint32_t type) const;
  static RoutingOptionsClassifier const & Instance(VehicleType vehicleType);

private:
  explicit RoutingOptionsClassifier(bool includeDirtyRoadClasses);

  base::SmallMap<uint32_t, RoutingOptions::Road> m_data;
};

std::string DebugPrint(RoutingOptions const & routingOptions);
std::string DebugPrint(RoutingOptions::Road type);
std::string DebugPrint(BicycleSurfaceBias bias);
std::string DebugPrint(BicycleSurfacePreference preference);

/// Options guard for debugging/tests.
class RoutingOptionSetter
{
public:
  explicit RoutingOptionSetter(RoutingOptions::RoadType roadsMask, VehicleType vehicleType = VehicleType::Car);
  ~RoutingOptionSetter();

private:
  VehicleType m_vehicleType;
  std::optional<std::string> m_saved;
  std::optional<std::string> m_savedBicycleSurface;
};
}  // namespace routing
