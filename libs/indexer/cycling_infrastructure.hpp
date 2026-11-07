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
  // Parallel to junctions; empty means legacy/unknown approach geometry.
  std::vector<std::vector<m2::PointD>> m_approaches = {};
};
struct ConnectorContact
{
  m2::PointD m_junction;
  std::vector<m2::PointD> m_roadPath;
  uint32_t m_roadType = 0;
  Kind m_kind = Kind::None;
  int8_t m_side = 0;
};
struct Sides
{
  Side m_left, m_right;
  std::vector<ConnectorContact> m_contacts = {};

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
inline constexpr uint8_t kPayloadVersion = 3;
inline constexpr size_t kMaxJunctionsPerSide = 64;
inline constexpr size_t kMaxApproachesPerJunction = 64;
inline constexpr size_t kMaxConnectorContacts = 8;

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
  for (auto const * side : {&sides.m_left, &sides.m_right})
  {
    CHECK(side->m_approaches.empty() || side->m_approaches.size() == side->m_junctions.size(), ());
    for (size_t i = 0; i < side->m_junctions.size(); ++i)
    {
      auto const size = side->m_approaches.empty() ? 0 : side->m_approaches[i].size();
      // Never publish a partial list that could conceal an unconnected incident.
      auto const count = size <= kMaxApproachesPerJunction ? size : 0;
      WriteToSink(sink, static_cast<uint32_t>(count));
      if (count)
        for (auto const & point : side->m_approaches[i])
        {
          WriteToSink(sink, std::bit_cast<uint64_t>(point.x));
          WriteToSink(sink, std::bit_cast<uint64_t>(point.y));
        }
    }
  }
  CHECK_LESS_OR_EQUAL(sides.m_contacts.size(), kMaxConnectorContacts, ());
  WriteToSink(sink, static_cast<uint8_t>(sides.m_contacts.size()));
  for (auto const & contact : sides.m_contacts)
  {
    CHECK(contact.m_side == -1 || contact.m_side == 1, ());
    CHECK(contact.m_kind == Kind::Track || contact.m_kind == Kind::Lane, ());
    CHECK(contact.m_roadPath.size() >= 2 && contact.m_roadPath.size() <= 5, ());
    WriteToSink(sink, contact.m_roadType);
    WriteToSink(sink, static_cast<uint8_t>(contact.m_kind));
    WriteToSink(sink, static_cast<uint8_t>(contact.m_side < 0 ? 1 : 2));
    WriteToSink(sink, static_cast<uint8_t>(contact.m_roadPath.size()));
    for (auto const & point : {contact.m_junction})
    {
      WriteToSink(sink, std::bit_cast<uint64_t>(point.x));
      WriteToSink(sink, std::bit_cast<uint64_t>(point.y));
    }
    for (auto const & point : contact.m_roadPath)
    {
      WriteToSink(sink, std::bit_cast<uint64_t>(point.x));
      WriteToSink(sink, std::bit_cast<uint64_t>(point.y));
    }
  }
  return result;
}

inline std::optional<Sides> Decode(std::string_view value)
{
  if (value.size() < 5 || (value[0] != 1 && value[0] != 2 && value[0] != kPayloadVersion))
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
  if (value[0] >= 2)
    for (auto * side : {&result.m_left, &result.m_right})
      for (size_t i = 0; i < side->m_junctions.size(); ++i)
      {
        if (source.Size() < sizeof(uint32_t))
          return {};
        auto const count = ReadPrimitiveFromSource<uint32_t>(source);
        if (count > kMaxApproachesPerJunction || count > source.Size() / (2 * sizeof(double)))
          return {};
        auto & points = side->m_approaches.emplace_back();
        for (uint32_t j = 0; j < count; ++j)
        {
          auto const x = ReadPrimitiveFromSource<uint64_t>(source);
          auto const y = ReadPrimitiveFromSource<uint64_t>(source);
          uint64_t constexpr kExponentMask = 0x7ff0000000000000;
          if ((x & kExponentMask) == kExponentMask || (y & kExponentMask) == kExponentMask)
            return {};
          m2::PointD const point(std::bit_cast<double>(x), std::bit_cast<double>(y));
          if (std::abs(point.x) > 180 || std::abs(point.y) > 180)
            return {};
          points.push_back(point);
        }
      }
  if (value[0] == 3)
  {
    if (source.Size() < 1)
      return {};
    auto const count = ReadPrimitiveFromSource<uint8_t>(source);
    if (count > kMaxConnectorContacts)
      return {};
    for (size_t i = 0; i < count; ++i)
    {
      if (source.Size() < 7)
        return {};
      ConnectorContact contact;
      contact.m_roadType = ReadPrimitiveFromSource<uint32_t>(source);
      contact.m_kind = static_cast<Kind>(ReadPrimitiveFromSource<uint8_t>(source));
      auto const side = ReadPrimitiveFromSource<uint8_t>(source);
      auto const points = ReadPrimitiveFromSource<uint8_t>(source);
      if (contact.m_roadType == 0 || (contact.m_kind != Kind::Track && contact.m_kind != Kind::Lane) ||
          (side != 1 && side != 2) || points < 2 || points > 5 || source.Size() < (points + 1) * 16u)
        return {};
      contact.m_side = side == 1 ? -1 : 1;
      for (size_t j = 0; j <= points; ++j)
      {
        auto const x = ReadPrimitiveFromSource<uint64_t>(source);
        auto const y = ReadPrimitiveFromSource<uint64_t>(source);
        uint64_t constexpr kExponentMask = 0x7ff0000000000000;
        if ((x & kExponentMask) == kExponentMask || (y & kExponentMask) == kExponentMask)
          return {};
        m2::PointD const point(std::bit_cast<double>(x), std::bit_cast<double>(y));
        if (std::abs(point.x) > 180 || std::abs(point.y) > 180)
          return {};
        if (j == 0)
          contact.m_junction = point;
        else
          contact.m_roadPath.push_back(point);
      }
      result.m_contacts.push_back(std::move(contact));
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

}  // namespace cycling
