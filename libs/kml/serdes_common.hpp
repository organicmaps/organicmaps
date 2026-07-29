#pragma once
#include "kml/type_utils.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

namespace kml
{
auto constexpr kDefaultLang = StringUtf8Multilang::kDefaultCode;
auto constexpr kDefaultTrackWidth = 5.0;
auto constexpr kDefaultTrackColor = 0x006ec7ff;

std::string PointToLineString(geometry::PointWithAltitude const & pt);
std::string PointToGxString(geometry::PointWithAltitude const & pt);

// True if the line carries real elevation, i.e. any point has a non-default, non-invalid
// altitude. Used to decide whether to export elevation (GPX <ele>, GeoJSON Z) for a line, so a
// flat sea-level track isn't bloated with zero altitudes. Mirrors Track::HasAltitudes intent.
bool LineHasAltitude(TrackGeometry const & line);

void SaveStringWithCDATA(Writer & writer, std::string const & s);
std::string const * GetDefaultLanguage(LocalizableString const & lstr);

// Name/description to write into an exported file, shared by all exporters (KML, GPX, GeoJSON).
// A name typed by the user is stored in m_customName and must win over the original (POI) name
// kept in m_name. Strings prefer default/int_name/en; if none exists, the lowest language code is
// used as a deterministic last resort so that a non-empty localized value is never dropped.
std::string GetNameForExport(BookmarkData const & bmData);
std::string GetStringForExport(LocalizableString const & lstr);

std::string_view constexpr kIndent0{};
std::string_view constexpr kIndent2{"  "};
std::string_view constexpr kIndent4{"    "};
std::string_view constexpr kIndent6{"      "};
std::string_view constexpr kIndent8{"        "};
std::string_view constexpr kIndent10{"          "};

}  // namespace kml
