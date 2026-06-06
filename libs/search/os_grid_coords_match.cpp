#include "search/os_grid_coords_match.hpp"

#include "platform/os_grid_utils.hpp"

namespace search
{
namespace
{
// A bare "2 letters + few digits" string (e.g. "SW12", "SE10", "HP13", "NO 10", "NY 1000")
// is also a UK postcode district / ordinary query, yet parses as a low-precision OS grid
// reference. Since a coordinate match suppresses the normal geocoder/postcode search, accept
// only unambiguous references here: two equal-length digit groups ("SH 6098 5437", "NN 166 712")
// or a single compact run of >= 6 digits ("SH6098954379"). FormatOSGrid emits the former, so
// copy/paste round-trips. Letter and grid-range validation stays in OSGridToLatLon.
bool IsUnambiguousGridRef(std::string const & query)
{
  auto const isSpace = [](char c) { return c == ' ' || c == '\t'; };
  auto const isDigit = [](char c) { return c >= '0' && c <= '9'; };
  auto const isAlpha = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); };

  size_t i = 0;
  while (i < query.size() && isSpace(query[i]))
    ++i;
  if (query.size() - i < 2 || !isAlpha(query[i]) || !isAlpha(query[i + 1]))
    return false;
  i += 2;

  // Collect the lengths of whitespace-separated digit groups; reject any other character.
  size_t groupLen[2] = {0, 0};
  int groupCount = 0;
  while (i < query.size())
  {
    if (isSpace(query[i]))
    {
      ++i;
      continue;
    }
    if (!isDigit(query[i]))
      return false;
    size_t len = 0;
    while (i < query.size() && isDigit(query[i]))
    {
      ++i;
      ++len;
    }
    if (groupCount < 2)
      groupLen[groupCount] = len;
    if (++groupCount > 2)
      return false;
  }

  if (groupCount == 1)
    return groupLen[0] >= 6 && groupLen[0] % 2 == 0;
  if (groupCount == 2)
    return groupLen[0] == groupLen[1];
  return false;
}
}  // namespace

std::optional<ms::LatLon> MatchOSGridCoords(std::string const & query)
{
  if (!IsUnambiguousGridRef(query))
    return {};
  return os_grid_utils::OSGridToLatLon(query);
}
}  // namespace search
