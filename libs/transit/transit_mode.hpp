#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace routing::transit
{
enum class TransitType : uint32_t
{
  // Keep the platform transit-step IDs stable.
  IntermediatePoint,
  Pedestrian,
  Subway,
  Train,
  LightRail,
  Monorail,
  Tram,
  Bus,
  Ferry,
  CableTram,
  AerialLift,
  Funicular,
  Trolleybus,
  AirService,
  WaterService
};

std::optional<TransitType> TryGetTransitType(std::string_view type);
TransitType GetTransitType(std::string_view type);
std::string DebugPrint(TransitType type);

using ModeMask = uint16_t;
enum class Mode : ModeMask
{
  Bus = 1u << 0,
  Tram = 1u << 1,
  Subway = 1u << 2,
  Rail = 1u << 3,
  Ferry = 1u << 4,
  Trolleybus = 1u << 5,
  Monorail = 1u << 6,
  Cable = 1u << 7,
  Air = 1u << 8
};

ModeMask constexpr kAllModes = (static_cast<ModeMask>(Mode::Air) << 1) - 1;

Mode GetMode(TransitType type);
constexpr bool HasMode(ModeMask modes, Mode mode)
{
  return (modes & static_cast<ModeMask>(mode)) != 0;
}
}  // namespace routing::transit
