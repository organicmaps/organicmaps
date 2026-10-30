#pragma once
#include "coding/string_utf8_multilang.hpp"
#include "coding/writer.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

namespace kml
{
struct FileData;

auto constexpr kDefaultLang = StringUtf8Multilang::kDefaultCode;
auto constexpr kDefaultTrackWidth = 5.0;
auto constexpr kDefaultTrackColor = 0x006ec7ff;

std::string PointToString(m2::PointD const & org, char const separator);
std::string PointToLineString(geometry::PointWithAltitude const & pt);
std::string PointToGxString(geometry::PointWithAltitude const & pt);

// Compact imported tracks, allowing a duplicate to supply a missing capture time.
// A section retains its times only when every retained point has a valid, nondecreasing time.
// A missing or invalid time clears the section's times and preserves its geometry.
void NormalizeImportedTracks(FileData & data);

// Checks WGS84 coordinate ranges. The import parsers guarantee finite inputs.
bool IsValidLatLon(double lat, double lon);

// Rounds a finite altitude to meters and clamps it to the representable range, excluding
// geometry::kInvalidAltitude. The import parsers guarantee finite inputs.
geometry::Altitude ToAltitude(double altitude);

// Escapes XML text and quoted attributes, preserving whitespace and dropping forbidden C0 controls.
std::string EscapeXml(std::string_view value);

void SaveStringWithCDATA(Writer & writer, std::string_view s);

std::string_view constexpr kIndent0{};
std::string_view constexpr kIndent2{"  "};
std::string_view constexpr kIndent4{"    "};
std::string_view constexpr kIndent6{"      "};
std::string_view constexpr kIndent8{"        "};
std::string_view constexpr kIndent10{"          "};

}  // namespace kml
