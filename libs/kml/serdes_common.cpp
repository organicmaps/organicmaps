#include "kml/serdes_common.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace kml
{

std::string PointToString(m2::PointD const & org, char const separator)
{
  double const lon = mercator::XToLon(org.x);
  double const lat = mercator::YToLat(org.y);

  uint8_t constexpr kDigitsAfterComma = 6;
  return strings::to_string_dac(lon, kDigitsAfterComma) + separator + strings::to_string_dac(lat, kDigitsAfterComma);
}

std::string PointToLineString(geometry::PointWithAltitude const & pt)
{
  char constexpr kSeparator = ',';
  if (pt.GetAltitude() != geometry::kInvalidAltitude)
    return PointToString(pt.GetPoint(), kSeparator) + kSeparator + strings::to_string(pt.GetAltitude());
  return PointToString(pt.GetPoint(), kSeparator);
}

std::string PointToGxString(geometry::PointWithAltitude const & pt)
{
  char constexpr kSeparator = ' ';
  if (pt.GetAltitude() != geometry::kInvalidAltitude)
    return PointToString(pt.GetPoint(), kSeparator) + kSeparator + strings::to_string(pt.GetAltitude());
  return PointToString(pt.GetPoint(), kSeparator);
}

bool LineHasAltitude(TrackGeometry const & line)
{
  return base::AnyOf(line, [](geometry::PointWithAltitude const & pt)
  {
    auto const alt = pt.GetAltitude();
    return alt != geometry::kInvalidAltitude && alt != geometry::kDefaultAltitudeMeters;
  });
}

// math::is_finite() is not redundant: this TU is compiled with -ffast-math (unlike the one defining
// it), where the compiler assumes there are no NaNs and mercator::ValidLat(NaN) is true in Release.
bool IsValidLatLon(double lat, double lon)
{
  return math::is_finite(lat) && math::is_finite(lon) && mercator::ValidLat(lat) && mercator::ValidLon(lon);
}

geometry::Altitude ToAltitude(double altitude)
{
  // Also rejects NaN, which has no representable value and would make the conversion below undefined.
  if (!math::is_finite(altitude))
    return geometry::kInvalidAltitude;

  double constexpr kMinAltitude = geometry::kInvalidAltitude + 1.0;
  double constexpr kMaxAltitude = std::numeric_limits<geometry::Altitude>::max();
  return static_cast<geometry::Altitude>(std::clamp(std::round(altitude), kMinAltitude, kMaxAltitude));
}

void SaveStringWithCDATA(Writer & writer, std::string_view s)
{
  if (s.empty())
    return;

  // Expat loads XML 1.0 only. Sometimes users copy and paste bookmark descriptions or even names from the web.
  // Rarely, in these copy-pasted texts, there are invalid XML1.0 symbols.
  // See https://en.wikipedia.org/wiki/Valid_characters_in_XML
  // A robust solution requires parsing invalid XML on loading (then users can restore "bad" XML files), see
  // https://github.com/organicmaps/organicmaps/issues/3837
  // When a robust solution is implemented, this workaround can be removed for better performance/battery.
  //
  // This solution is a simple ASCII-range check that does not check symbols from other unicode ranges
  // (they will require a more complex and slower approach of converting UTF-8 string to unicode first).
  // It should be enough for many cases, according to user reports and wrong characters in their data.
  auto const isInvalidXmlChar = [](unsigned char c) { return c < 0x20 && c != 0x09 && c != 0x0a && c != 0x0d; };

  // Only copy and modify the string if invalid chars are actually found (rare case).
  std::string filtered;
  std::string_view clean = s;
  if (std::any_of(s.begin(), s.end(), isInvalidXmlChar))
  {
    filtered = s;
    std::erase_if(filtered, isInvalidXmlChar);
    if (filtered.empty())
      return;
    clean = filtered;
  }

  // According to kml/xml spec, we need to escape special symbols with CDATA.
  if (clean.find_first_of("<&") != std::string_view::npos)
    writer << "<![CDATA[" << clean << "]]>";
  else
    writer << clean;
}

}  // namespace kml
