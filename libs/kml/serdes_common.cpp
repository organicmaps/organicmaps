#include "kml/serdes_common.hpp"
#include "kml/types.hpp"

#include "geometry/mercator.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

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

void SaveStringWithCDATA(Writer & writer, std::string const & s)
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
  std::string const * clean = &s;
  if (std::any_of(s.begin(), s.end(), isInvalidXmlChar))
  {
    filtered = s;
    std::erase_if(filtered, isInvalidXmlChar);
    if (filtered.empty())
      return;
    clean = &filtered;
  }

  // According to kml/xml spec, we need to escape special symbols with CDATA.
  if (clean->find_first_of("<&") != std::string::npos)
    writer << "<![CDATA[" << *clean << "]]>";
  else
    writer << *clean;
}

std::string GetStringForExport(LocalizableString const & lstr)
{
  auto const getNonEmpty = [&lstr](int8_t const lang) -> std::string const *
  {
    auto const it = lstr.find(lang);
    return it != lstr.end() && !it->second.empty() ? &it->second : nullptr;
  };

  for (auto const lang : {kDefaultLang, StringUtf8Multilang::kInternationalCode, StringUtf8Multilang::kEnglishCode})
    if (auto const value = getNonEmpty(lang))
      return *value;

  std::string const * fallback = nullptr;
  int8_t fallbackLang = StringUtf8Multilang::kMaxSupportedLanguages;
  for (auto const & [lang, value] : lstr)
  {
    if (!value.empty() && lang < fallbackLang)
    {
      fallback = &value;
      fallbackLang = lang;
    }
  }
  return fallback ? *fallback : std::string{};
}

std::string GetNameForExport(BookmarkData const & bmData)
{
  std::string name = GetStringForExport(bmData.m_customName);
  if (name.empty())
    name = GetStringForExport(bmData.m_name);
  if (name.empty())
    name = GetLocalizedFeatureType(bmData.m_featureTypes);
  return name;
}

}  // namespace kml
