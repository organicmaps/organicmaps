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

std::string PointToString(m2::PointD const & org, char const separator);

std::string PointToLineString(geometry::PointWithAltitude const & pt);
std::string PointToGxString(geometry::PointWithAltitude const & pt);

void SaveStringWithCDATA(Writer & writer, std::string s);
std::string const * GetDefaultLanguage(LocalizableString const & lstr);

std::string_view constexpr kIndent0{};
std::string_view constexpr kIndent2{"  "};
std::string_view constexpr kIndent4{"    "};
std::string_view constexpr kIndent6{"      "};
std::string_view constexpr kIndent8{"        "};
std::string_view constexpr kIndent10{"          "};

}  // namespace kml
