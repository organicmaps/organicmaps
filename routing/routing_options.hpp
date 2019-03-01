#pragma once

#include <cstdint>
#include <string>
#include <type_traits>

namespace routing
{
class RoutingOptions
{
public:
  static std::string const kAvoidRoutingOptionSettings;

  enum class Road : uint8_t
  {
    Usual    = 1u << 0,
    Toll     = 1u << 1,
    Motorway = 1u << 2,
    Ferry    = 1u << 3,
    Dirty    = 1u << 4
  };

  using RoadType = std::underlying_type<Road>::type;

  RoutingOptions() = default;
  explicit RoutingOptions(RoadType mask) : m_options(mask) {}

  static RoutingOptions LoadFromSettings();

  void Add(Road type);
  void Remove(Road type);
  bool Has(Road type) const;

  RoadType GetOptions() const { return m_options; }

private:
  RoadType m_options = 0;
};

std::string DebugPrint(RoutingOptions const & routingOptions);
std::string DebugPrint(RoutingOptions::Road type);
}  // namespace routing
