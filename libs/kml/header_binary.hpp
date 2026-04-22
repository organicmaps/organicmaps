#pragma once

#include "coding/serdes_binary_header.hpp"

#include <cstdint>

namespace kml
{
namespace binary
{
enum class Version : uint8_t
{
  V0 = 0,
  V1 = 1,  // 11th April 2018: new Point2D storage, added deviceId, feature name -> custom name.
  V2 = 2,  // 25th April 2018: added serverId.
  V3 = 3,  // 7th May 2018: persistent feature types. V3 is binary compatible with lower versions.
  V4 = 4,  // 26th August 2019: key-value properties and nearestToponym for bookmarks and tracks,
           // cities -> toponyms.
  V5 = 5,  // 21st November 2019: extended color palette.
  V6 = 6,  // 3rd December 2019: extended bookmark icons. V6 is binary compatible with V4 and V5
           // versions.
  V7 = 7,  // 13th February 2020: track points are replaced by points with altitude.
  V8 = 8,  // 24 September 2020: add compilations to types and corresponding section to kmb and
           // tags to kml
  V9 = 9,  // 01 October 2020: add minZoom to bookmarks
  Latest = V9,
  // MapsMe-incompatible variants. Enum values 10/11 are internal identifiers assigned after V9
  // (V8/V9 occupy 8/9); they are NOT the on-disk version byte. On disk, V8MM still starts with
  // 0x08 and V9MM still starts with 0x09 — the variant is detected via a header-shape heuristic
  // (5 section offsets instead of 6) in DeserializerKml::InitializeIfNeeded.
  V8MM = 10,  // 27 July 2023: MapsMe released version v15.0.71617. On-disk byte is 0x08 but the
              // layout is not compatible with this repo's V8 (no compilations section).
  V9MM = 11   // July 2024: MapsMe released a new KMB format. On-disk byte is 0x09 but not
              // compatible with this repo's V9 (track uses vector<MultiGeometry>, no
              // compilations). A later MapsMe release evolved the track layout to drop
              // m_constant3 and append a per-point capture-timestamp vector; both shapes
              // decode through this path — the legacy c3=0 byte reads as ts_count=0.
};

inline std::string DebugPrint(Version v)
{
  switch (v)
  {
  case Version::V0: return "V0";
  case Version::V1: return "V1";
  case Version::V2: return "V2";
  case Version::V3: return "V3";
  case Version::V4: return "V4";
  case Version::V5: return "V5";
  case Version::V6: return "V6";
  case Version::V7: return "V7";
  case Version::V8: return "V8";
  case Version::V9: return "V9";  // == Latest
  case Version::V8MM: return "V8MM";
  case Version::V9MM: return "V9MM";
  }
  return "Unknown(" + ::DebugPrint(static_cast<int>(v)) + ")";
}

struct Header
{
  template <typename Visitor>
  void Visit(Visitor & visitor)
  {
    visitor(m_categoryOffset, "categoryOffset");
    visitor(m_bookmarksOffset, "bookmarksOffset");
    visitor(m_tracksOffset, "tracksOffset");
    if (HasCompilationsSection())
      visitor(m_compilationsOffset, "compilationsOffset");
    visitor(m_stringsOffset, "stringsOffset");
    if (!HasCompilationsSection())
      m_compilationsOffset = m_stringsOffset;
    visitor(m_eosOffset, "eosOffset");
  }

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    coding::binary::HeaderSerVisitor<Sink> visitor(sink);
    visitor(*this);
  }

  template <typename Source>
  void Deserialize(Source & source)
  {
    coding::binary::HeaderDesVisitor<Source> visitor(source);
    visitor(*this);
  }

  // Calculates the size of serialized header in bytes.
  uint64_t Size()
  {
    coding::binary::HeaderSizeOfVisitor visitor;
    visitor(*this);
    return visitor.m_size;
  }

  bool HasCompilationsSection() const { return m_version == Version::V8 || m_version == Version::V9; }

  Version m_version = Version::Latest;
  uint64_t m_categoryOffset = 0;
  uint64_t m_bookmarksOffset = 0;
  uint64_t m_tracksOffset = 0;
  uint64_t m_compilationsOffset = 0;
  uint64_t m_stringsOffset = 0;
  uint64_t m_eosOffset = 0;
};
}  // namespace binary
}  // namespace kml
