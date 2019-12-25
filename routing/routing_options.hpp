#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace routing
{
class RoutingOptions
{
public:
  static std::string const kAvoidRoutingOptionSettingsForCar;

  enum class Road : uint8_t
  {
    Usual    = 1u << 0,
    Toll     = 1u << 1,
    Motorway = 1u << 2,
    Ferry    = 1u << 3,
    Dirty    = 1u << 4,

    Max      = (1u << 4) + 1
  };

  using RoadType = std::underlying_type_t<Road>;

  RoutingOptions() = default;
  explicit RoutingOptions(RoadType mask) : m_options(mask) {}

  static RoutingOptions LoadCarOptionsFromSettings();
  static void SaveCarOptionsToSettings(RoutingOptions options);

  void Add(Road type);
  void Remove(Road type);
  bool Has(Road type) const;

  RoadType GetOptions() const { return m_options; }

private:
  RoadType m_options = 0;
};

class RoutingOptionsClassifier
{
public:
  RoutingOptionsClassifier();

  std::optional<RoutingOptions::Road> Get(uint32_t type) const;
  static RoutingOptionsClassifier const & Instance();

private:
  std::unordered_map<uint32_t, RoutingOptions::Road> m_data;
};

RoutingOptions::Road ChooseMainRoutingOptionRoad(RoutingOptions options);

std::string DebugPrint(RoutingOptions const & routingOptions);
std::string DebugPrint(RoutingOptions::Road type);

}  // namespace routing

