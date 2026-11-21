#include "generator/cycling_infrastructure.hpp"

#include "generator/osm_element.hpp"

#include <string>

namespace generator
{
bool HasMotorTrafficSeparation(OsmElement const & element, std::string const & key)
{
  for (std::string const kind : {"separation", "barrier"})
  {
    auto const positive = [&](std::string const & value)
    {
      return !value.empty() && value != "no" && value != "none" && value != "false" && value != "no_separation" &&
             (kind != "separation" || (value != "studs" && value != "cone"));
    };
    std::string const prefix = key + ":" + kind;
    if (positive(element.GetTag(prefix)) || positive(element.GetTag(prefix + ":both")))
      return true;
    for (std::string const edge : {"left", "right"})
    {
      if (!positive(element.GetTag(prefix + ":" + edge)))
        continue;
      auto const mode = element.GetTag(key + ":traffic_mode:" + edge);
      if (mode == "motor_vehicle" || mode == "motorcar" || mode == "vehicle")
        return true;
    }
  }
  return false;
}

std::optional<cycling::Sides> ReadCyclingSides(OsmElement const & element, bool reversed)
{
  using cycling::Direction;
  using cycling::Kind;
  using cycling::Side;
  using cycling::Sides;
  auto const tag = [&element](std::string const & key) { return element.GetTag(key); };
  auto const negative = [](std::string_view value) { return value == "no" || value == "none" || value == "false"; };
  auto const read = [&](std::string const & key, std::optional<Side> fallback) -> std::optional<Side>
  {
    auto const value = tag(key);
    auto const lane = tag(key + ":lane");
    Side side = value.empty() && lane.empty() ? fallback.value_or(Side{}) : Side{};
    if (negative(value) || value == "separate" || value == "use_sidepath")
      return side;
    if (value == "track" || value == "opposite_track" || value == "protected_lane")
      side.m_kind = Kind::Track;
    else if (value == "lane" || value == "opposite_lane" || value == "buffered_lane" || lane == "exclusive")
      side.m_kind = Kind::Lane;
    else if (value == "shared_lane" || value == "share_busway" || value == "opposite_share_busway" ||
             value == "shoulder")
      side.m_kind = Kind::Shared;
    if (lane == "advisory" || lane == "pictogram")
      side.m_kind = Kind::Shared;
    if (side.m_kind == Kind::None)
      return {};
    if (side.m_kind == Kind::Lane && HasMotorTrafficSeparation(element, key))
      side.m_kind = Kind::Track;
    auto const oneway = tag(key + ":oneway");
    if (oneway == "yes" || oneway == "1" || oneway == "true")
      side.m_direction = Direction::Forward;
    else if (oneway == "-1")
      side.m_direction = Direction::Backward;
    else if (negative(oneway))
      side.m_direction = Direction::Both;
    else if (!oneway.empty())
      side.m_direction = Direction::Unknown;
    else if (value.starts_with("opposite_"))
      side.m_direction = Direction::Backward;
    return side;
  };
  auto const both = read("cycleway:both", {});
  auto const left = read("cycleway:left", both);
  auto const right = read("cycleway:right", both);
  if (!both && !left && !right)
    return {};  // Do not invent sides from an unqualified centerline category.
  Sides result{left.value_or(both.value_or(Side{})), right.value_or(both.value_or(Side{}))};
  if (reversed)
    result.Reverse();
  return result;
}
}  // namespace generator
