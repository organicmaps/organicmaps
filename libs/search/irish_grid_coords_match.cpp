#include "search/irish_grid_coords_match.hpp"

#include "platform/irish_grid_utils.hpp"

namespace search
{
namespace
{
auto constexpr kIsSpace = [](char c) { return c == ' ' || c == '\t'; };
auto constexpr kIsDigit = [](char c) { return c >= '0' && c <= '9'; };

// Accept a single grid letter followed by either two equal-length digit groups of >= 3 digits
// ("O 152 345", 6-figure / 100 m) or a single compact run of >= 6 even digits ("O152345"). This is the
// "necessary precision" for a deliberate reference: it rejects road numbers ("N4", "M50", "R110"),
// Dublin districts ("D4", "D6") and Eircodes (whose second group has letters, e.g. "D04 X285"), so
// those never even produce a coordinate. The single leading letter still collides with some strings,
// but because the processor treats Irish Grid as a weak (non-suppressing) match, that is harmless -
// the coordinate is just offered alongside the normal search results. Letter and grid-range validation
// stay in IrishGridToLatLon; whether the decoded point is in Ireland is the processor's call (by region).
bool IsPlausibleIrishGridRef(std::string const & query)
{
  auto const isAlpha = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); };

  size_t i = 0;
  while (i < query.size() && kIsSpace(query[i]))
    ++i;

  // Exactly one leading letter: a second letter would be an OS Grid reference or an Eircode.
  if (i >= query.size() || !isAlpha(query[i]))
    return false;
  ++i;
  if (i < query.size() && isAlpha(query[i]))
    return false;

  size_t groupLen[2] = {0, 0};
  int groupCount = 0;
  while (i < query.size())
  {
    if (kIsSpace(query[i]))
    {
      ++i;
      continue;
    }
    if (!kIsDigit(query[i]))
      return false;
    size_t len = 0;
    while (i < query.size() && kIsDigit(query[i]))
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
    return groupLen[0] == groupLen[1] && groupLen[0] >= 3;
  return false;
}
}  // namespace

std::optional<ms::LatLon> MatchIrishGridCoords(std::string const & query)
{
  if (!IsPlausibleIrishGridRef(query))
    return {};
  return irish_grid_utils::IrishGridToLatLon(query);
}

std::optional<ms::LatLon> MatchITMCoords(std::string const & query)
{
  // Two whitespace/comma-separated 6-digit groups (ITM eastings and northings across the island are
  // 6 digits); ITMToLatLon then parses the pair, and the processor confirms it lies in Ireland (by
  // region). A bare number pair is ambiguous, but - like Irish Grid - ITM is a weak (non-suppressing)
  // match, so an in-range pair is only offered alongside the normal search, never instead of it.
  size_t i = 0;
  int groupCount = 0;
  while (i < query.size())
  {
    if (kIsSpace(query[i]) || query[i] == ',')
    {
      ++i;
      continue;
    }
    if (!kIsDigit(query[i]))
      return {};
    size_t len = 0;
    while (i < query.size() && kIsDigit(query[i]))
    {
      ++i;
      ++len;
    }
    if (len != 6 || ++groupCount > 2)
      return {};
  }
  if (groupCount != 2)
    return {};

  return irish_grid_utils::ITMToLatLon(query);
}
}  // namespace search
