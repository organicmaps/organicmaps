#include "indexer/osm_value_format.hpp"

#include "platform/measurement_utils.hpp"

#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string_view>

namespace osm
{
namespace
{
// https://en.wikipedia.org/wiki/List_of_tallest_buildings_in_the_world
auto constexpr kMaxBuildingLevelsInTheWorld = 167;

bool Prefix2Double(std::string const & str, double & d)
{
  char * stop;
  char const * s = str.c_str();
  // TODO: Replace with a faster and locale-ignored double conversion.
  d = std::strtod(s, &stop);
  return (s != stop && math::is_finite(d));
}
}  // namespace

std::string ValidateAndFormat_stars(std::string const & v)
{
  if (v.empty())
    return {};

  // We are accepting stars from 1 to 7.
  if (v[0] <= '0' || v[0] > '7')
    return {};

  // Ignore numbers larger than 9.
  if (v.size() > 1 && ::isdigit(v[1]))
    return {};

  return std::string(1, v[0]);
}

std::string ValidateAndFormat_url(std::string const & v)
{
  // Remove the last slash if it's after the hostname to beautify URLs in the UI and save a byte of space:
  // https://www.test.com/ => https://www.test.com
  // www.test.com/ => www.test.com
  // www.test.com/path => www.test.com/path
  // www.test.com/path/ => www.test.com/path/
  constexpr std::string_view kHttps = "https://";
  constexpr std::string_view kHttp = "http://";
  size_t start = 0;
  if (v.starts_with(kHttps))
    start = kHttps.size();
  else if (v.starts_with(kHttp))
    start = kHttp.size();
  auto const first = v.find('/', start);
  if (first == std::string::npos)
    return v;
  if (first + 1 == v.size())
    return std::string{v.begin(), --v.end()};
  return v;
}

std::string ValidateAndFormat_internet(std::string const & value)
{
  std::string v = value;
  strings::AsciiToLower(v);
  if (v == "wlan" || v == "wired" || v == "terminal" || v == "yes" || v == "no")
    return v;
  // Process additional top tags.
  if (v == "free" || v == "wifi" || v == "public")
    return "wlan";
  return {};
}

std::string ValidateAndFormat_height(std::string const & v)
{
  return measurement_utils::OSMDistanceToMetersString(v, false /*supportZeroAndNegativeValues*/, 1);
}

std::string ValidateAndFormat_building_levels(std::string const & value)
{
  std::string v = value;
  // Some mappers use full width unicode digits. We can handle that.
  strings::NormalizeDigits(v);
  // value of building_levels is only one number
  double levels;
  if (Prefix2Double(v, levels) && levels >= 0 && levels <= kMaxBuildingLevelsInTheWorld)
    return strings::to_string_dac(levels, 1);

  return {};
}

std::string ValidateAndFormat_level(std::string const & value)
{
  std::string v = value;
  // Some mappers use full width unicode digits. We can handle that.
  strings::NormalizeDigits(v);
  // value of level can be more than one number, so e.g. "1;2" or "3-5"
  return v;
}

std::string ValidateAndFormat_drive_through(std::string const & value)
{
  std::string v = value;
  strings::AsciiToLower(v);
  if (v == "yes" || v == "no")
    return v;
  return {};
}

std::string ValidateAndFormat_self_service(std::string const & value)
{
  std::string v = value;
  strings::AsciiToLower(v);
  if (v == "yes" || v == "only" || v == "partially" || v == "no")
    return v;
  return {};
}

std::string ValidateAndFormat_outdoor_seating(std::string const & value)
{
  std::string v = value;
  strings::AsciiToLower(v);
  if (v == "yes" || v == "no")
    return v;
  return {};
}

std::string NormalizeCuisineToken(std::string const & v)
{
  std::string token = v;
  strings::MakeLowerCaseInplace(token);
  strings::Trim(token, " ");
  auto const isRepeatedSpace = [](char lhs, char rhs) { return lhs == rhs && lhs == ' '; };
  token.erase(std::unique(token.begin(), token.end(), isRepeatedSpace), token.end());
  std::replace(token.begin(), token.end(), ' ', '_');

  // Avoid duplication for some cuisines.
  if (token == "bbq" || token == "barbeque")
    return "barbecue";
  if (token == "doughnut")
    return "donut";
  if (token == "steak")
    return "steak_house";
  if (token == "coffee")
    return "coffee_shop";

  return token;
}
}  // namespace osm
