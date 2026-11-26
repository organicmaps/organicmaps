#include "kml/serdes_common.hpp"
#include "kml/types.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace kml
{

std::string PointToString(m2::PointD const & org, char const separator)
{
  double const lon = mercator::XToLon(org.x);
  double const lat = mercator::YToLat(org.y);

  uint8_t constexpr kDigitsAfterComma = 6;
  return strings::to_string_dac(lon, kDigitsAfterComma) + separator + strings::to_string_dac(lat, kDigitsAfterComma);
}

static std::string FormatPointWithAltitude(geometry::PointWithAltitude const & pt, char separator)
{
  auto text = PointToString(pt.GetPoint(), separator);
  if (auto const altitude = pt.GetAltitude(); altitude != geometry::kInvalidAltitude)
  {
    text += separator;
    text += std::to_string(altitude);
  }
  return text;
}

std::string PointToLineString(geometry::PointWithAltitude const & pt)
{
  return FormatPointWithAltitude(pt, ',');
}

std::string PointToGxString(geometry::PointWithAltitude const & pt)
{
  // Keep the legacy two-value gx:coord for points with unknown height.
  return FormatPointWithAltitude(pt, ' ');
}

void NormalizeImportedTracks(FileData & data)
{
  for (auto & track : data.m_tracksData)
  {
    auto & geometry = track.m_geometry;
    ASSERT_EQUAL(geometry.m_lines.size(), geometry.m_timestamps.size(), (track.m_id));
    size_t keptLines = 0;
    for (size_t lineIndex = 0; lineIndex < geometry.m_lines.size(); ++lineIndex)
    {
      auto & line = geometry.m_lines[lineIndex];
      auto & times = geometry.m_timestamps[lineIndex];
      MultiGeometry::RemoveDuplicatePoints(line, times);
      if (line.size() < 2)
      {
        LOG(LWARNING, ("Degenerate track section", GetDefaultStr(track.m_name)));
        continue;
      }

      if (!MultiGeometry::IsValidSectionTimestamps(times, line.size()))
      {
        LOG(LWARNING, ("Dropping incomplete or invalid section timestamps", GetDefaultStr(track.m_name), lineIndex));
        times.clear();
      }

      if (keptLines != lineIndex)
      {
        geometry.m_lines[keptLines] = std::move(line);
        geometry.m_timestamps[keptLines] = std::move(times);
      }
      ++keptLines;
    }
    geometry.m_lines.resize(keptLines);
    geometry.m_timestamps.resize(keptLines);
    if (!geometry.HasLines())
      continue;

    if (track.m_layers.empty())
      track.m_layers.push_back({.m_lineWidth = kDefaultTrackWidth, .m_color = {.m_rgba = kDefaultTrackColor}});
  }

  std::set<LocalId> removedIds;
  base::EraseIf(data.m_tracksData, [&removedIds](TrackData const & track)
  {
    if (track.m_geometry.HasLines())
      return false;
    removedIds.insert(track.m_localId);
    return true;
  });

  if (!removedIds.empty())
    for (auto & bookmark : data.m_bookmarksData)
      base::EraseIf(bookmark.m_boundTracks, [&removedIds](LocalId id) { return removedIds.contains(id); });
}

bool IsValidLatLon(double lat, double lon)
{
  return mercator::ValidLat(lat) && mercator::ValidLon(lon);
}

geometry::Altitude ToAltitude(double altitude)
{
  ASSERT(math::is_finite(altitude), (altitude));

  static_assert(geometry::kInvalidAltitude == std::numeric_limits<geometry::Altitude>::min());
  double constexpr kMinAltitude = std::numeric_limits<geometry::Altitude>::min() + 1.0;
  double constexpr kMaxAltitude = std::numeric_limits<geometry::Altitude>::max();
  return static_cast<geometry::Altitude>(std::clamp(std::round(altitude), kMinAltitude, kMaxAltitude));
}

// XML 1.0 forbids these C0 controls, which can occur in pasted bookmark text (see #3837).
static bool IsInvalidXmlChar(unsigned char c)
{
  return c < 0x20 && c != '\t' && c != '\n' && c != '\r';
}

std::string EscapeXml(std::string_view value)
{
  std::string escaped;
  escaped.reserve(value.size());
  for (char const c : value)
  {
    switch (c)
    {
    case '&': escaped += "&amp;"; break;
    case '<': escaped += "&lt;"; break;
    case '>': escaped += "&gt;"; break;
    case '"': escaped += "&quot;"; break;
    case '\'': escaped += "&apos;"; break;
    // Character references preserve whitespace through XML attribute normalization.
    case '\t': escaped += "&#9;"; break;
    case '\n': escaped += "&#10;"; break;
    case '\r': escaped += "&#13;"; break;
    default:
      if (!IsInvalidXmlChar(c))
        escaped += c;
      break;
    }
  }
  return escaped;
}

void SaveStringWithCDATA(Writer & writer, std::string_view s)
{
  if (s.empty())
    return;

  // Only copy and modify the string if invalid chars are actually found (rare case).
  std::string filtered;
  std::string_view clean = s;
  if (std::any_of(s.begin(), s.end(), IsInvalidXmlChar))
  {
    filtered = s;
    std::erase_if(filtered, IsInvalidXmlChar);
    if (filtered.empty())
      return;
    clean = filtered;
  }

  auto pos = clean.find("]]>");
  if (pos != std::string_view::npos || clean.find_first_of("<&") != std::string_view::npos)
  {
    writer << "<![CDATA[";
    // XML forbids a CDATA terminator in character data and inside a CDATA section. Split it
    // across adjacent sections so a user-supplied name or description stays literal text.
    while (pos != std::string_view::npos)
    {
      writer << clean.substr(0, pos) << "]]]]><![CDATA[>";
      clean.remove_prefix(pos + 3);
      pos = clean.find("]]>");
    }
    writer << clean << "]]>";
  }
  else
    writer << clean;
}

}  // namespace kml
