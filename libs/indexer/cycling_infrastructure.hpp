#pragma once

#include "coding/byte_stream.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "geometry/point2d.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Optional rendering attributes. They neither replace the primary highway type
// nor grant routing access or claim a marked/protected crossing.
namespace cycling
{
enum class Kind : char
{
  None = '0',
  Track = 't',
  Lane = 'l',
  Shared = 's'
};
enum class Direction : char
{
  Unknown = '0',
  Forward = 'f',
  Backward = 'r',
  Both = 'b'
};
struct Side
{
  Kind m_kind = Kind::None;
  Direction m_direction = Direction::Unknown;
  std::vector<m2::PointD> m_junctions = {};
};
struct Sides
{
  Side m_left, m_right;

  void Reverse()
  {
    std::swap(m_left, m_right);
    for (auto * side : {&m_left, &m_right})
      if (side->m_direction == Direction::Forward)
        side->m_direction = Direction::Backward;
      else if (side->m_direction == Direction::Backward)
        side->m_direction = Direction::Forward;
  }
};
inline constexpr uint8_t kPayloadVersion = 1;
inline constexpr size_t kMaxJunctionsPerSide = 64;

inline std::string Encode(Sides const & sides)
{
  std::string result;
  PushBackByteSink sink(result);
  WriteToSink(sink, kPayloadVersion);
  for (auto const * side : {&sides.m_left, &sides.m_right})
  {
    auto const kind = std::string_view("0tls").find(static_cast<char>(side->m_kind));
    auto const direction = std::string_view("0frb").find(static_cast<char>(side->m_direction));
    CHECK_LESS(kind, 4, ());
    CHECK_LESS(direction, 4, ());
    WriteToSink(sink, static_cast<uint8_t>(kind | (direction << 2)));
  }
  for (auto const * side : {&sides.m_left, &sides.m_right})
  {
    CHECK_LESS_OR_EQUAL(side->m_junctions.size(), kMaxJunctionsPerSide, ());
    WriteToSink(sink, static_cast<uint8_t>(side->m_junctions.size()));
    for (auto const & point : side->m_junctions)
    {
      WriteToSink(sink, std::bit_cast<uint64_t>(point.x));
      WriteToSink(sink, std::bit_cast<uint64_t>(point.y));
    }
  }
  return result;
}

inline std::optional<Sides> Decode(std::string_view value)
{
  if (value.size() < 5 || static_cast<uint8_t>(value[0]) != kPayloadVersion)
    return {};
  Sides result;
  for (size_t i = 0; i < 2; ++i)
  {
    auto const flag = static_cast<uint8_t>(value[i + 1]);
    if (flag & 0xf0)
      return {};
    auto & side = i == 0 ? result.m_left : result.m_right;
    side.m_kind = static_cast<Kind>(std::string_view("0tls")[flag & 3]);
    side.m_direction = static_cast<Direction>(std::string_view("0frb")[(flag >> 2) & 3]);
  }
  MemReader reader(value.data(), value.size());
  ReaderSource source(reader);
  source.Skip(3);
  for (auto * side : {&result.m_left, &result.m_right})
  {
    if (source.Size() < 1)
      return {};
    auto const count = ReadPrimitiveFromSource<uint8_t>(source);
    if (count > kMaxJunctionsPerSide || source.Size() < uint64_t(count) * 2 * sizeof(double))
      return {};
    for (size_t i = 0; i < count; ++i)
    {
      auto const xBits = ReadPrimitiveFromSource<uint64_t>(source);
      auto const yBits = ReadPrimitiveFromSource<uint64_t>(source);
      // Floating-point classification can be optimized away under -ffast-math.
      uint64_t constexpr kExponentMask = 0x7ff0000000000000;
      if ((xBits & kExponentMask) == kExponentMask || (yBits & kExponentMask) == kExponentMask)
        return {};
      double const x = std::bit_cast<double>(xBits);
      double const y = std::bit_cast<double>(yBits);
      if (std::abs(x) > 180 || std::abs(y) > 180)
        return {};
      side->m_junctions.emplace_back(x, y);
    }
  }
  if (source.Size() != 0)
    return {};
  return result;
}

inline char const * TypeName(Kind kind)
{
  switch (kind)
  {
  case Kind::Track: return "track";
  case Kind::Lane: return "lane";
  case Kind::Shared: return "shared_lane";
  case Kind::None: return "";
  }
  return "";
}

template <typename GetTag>
std::optional<Sides> ReadSides(GetTag const & tag, bool reversed)
{
  auto const negative = [](std::string_view value) { return value == "no" || value == "none" || value == "false"; };
  auto const protectedFromTraffic = [&](std::string const & key)
  {
    for (std::string const kind : {"separation", "barrier"})
    {
      auto const positive = [&](std::string const & value)
      {
        return !value.empty() && !negative(value) && value != "no_separation" &&
               (kind != "separation" || (value != "studs" && value != "cone"));
      };
      std::string const prefix = key + ":" + kind;
      if (positive(tag(prefix)) || positive(tag(prefix + ":both")))
        return true;
      for (std::string const edge : {"left", "right"})
      {
        auto const mode = tag(key + ":traffic_mode:" + edge);
        if (positive(tag(prefix + ":" + edge)) && (mode == "motor_vehicle" || mode == "motorcar" || mode == "vehicle"))
          return true;
      }
    }
    return false;
  };
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
    if (side.m_kind == Kind::Lane && protectedFromTraffic(key))
      side.m_kind = Kind::Track;
    auto const oneway = tag(key + ":oneway");
    if (oneway == "yes" || oneway == "1" || oneway == "true")
      side.m_direction = Direction::Forward;
    else if (oneway == "-1" || value.starts_with("opposite_"))
      side.m_direction = Direction::Backward;
    else if (negative(oneway))
      side.m_direction = Direction::Both;
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
}  // namespace cycling
