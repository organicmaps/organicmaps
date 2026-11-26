#pragma once

#include "routing/vehicle_mask.hpp"

#include "base/small_map.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>
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

private:
  RoadType m_options = 0;
};

/// \brief Calculation strategy. Normal — full road model (speed, traffic, penalties, climb).
/// DistanceBiased — roads whose Normal weight speed is at or above the per-vehicle
/// distance-bias speed cap are ranked purely by distance (weight = distance / cap), while
/// slower roads keep their Normal weight, so the search yields the shortest chain of reasonable
/// roads without gaining from segments the model considers unreasonably slow (tracks, footways,
/// steps, bad surfaces — see issue #12953). Weights never drop below distance / cap and thus
/// below distance / the model's maximum speed, so the A* heuristic stays admissible and may even
/// divide by the cap where no faster-priced edges exist (see EdgeEstimator::CalcHeuristic).
/// Non-segment penalties applied at the graph layer (u-turn, ferry, etc. in
/// IndexGraph::CalculateEdgeWeight) still take effect. Used to compute a reasonable,
/// distance-biased alternative alongside the normal route.
enum class RoadRoutingStrategy : uint8_t
{
  Normal,
  DistanceBiased
};

std::string_view ToString(RoadRoutingStrategy strategy);
std::string DebugPrint(RoadRoutingStrategy strategy);

// Captured by IndexRouter on each road variant, including the alternative.
struct RoadRoutingPreferences
{
  VehicleType m_vehicleType;
  RoutingOptions m_avoidances;
  RoadRoutingStrategy m_strategy;

  std::map<std::string, std::string> ToTrackProperties() const;
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

/// Options guard for debugging/tests.
class RoutingOptionSetter
{
public:
  explicit RoutingOptionSetter(RoutingOptions::RoadType roadsMask, VehicleType vehicleType = VehicleType::Car);
  ~RoutingOptionSetter();

private:
  VehicleType m_vehicleType;
  std::optional<std::string> m_saved;
};
}  // namespace routing
