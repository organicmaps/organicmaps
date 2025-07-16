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
  V8MM = 10,  // 27 July 2023: MapsMe released version v15.0.71617. Technically its version is 8
              // (first byte is 0x08), but it's not compatible with V8 from this repo. It has
              // no compilations.
  V9MM = 11   // In July 2024 MapsMe released version with a new KMB format. Technically its version is 9
              // (first byte is 0x09), but it's not compatible with OrganicMaps V9 from this repo.
              // It supports multiline geometry.
};

inline std::string DebugPrint(Version v)
{
  return ::DebugPrint(static_cast<int>(v));
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
