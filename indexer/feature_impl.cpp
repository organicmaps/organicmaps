#include "indexer/feature_impl.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"

#include <algorithm>

using namespace std;

namespace feature
{

bool IsDigit(int c)
{
  return (int('0') <= c && c <= int('9'));
}

bool IsNumber(strings::UniString const & s)
{
  for (size_t i = 0; i < s.size(); ++i)
  {
    // android production ndk-r8d has bug. "еда" detected as a number.
    if (!IsDigit(s[i]))
      return false;
  }
  return true;
}

bool IsStreetNumber(strings::UniString const & s)
{
  if (s.size() < 2)
    return false;

  /// add different localities in future, if it's a problem.
  for (auto const & streetEnding : {"st", "nd", "rd", "th"})
  {
    if (strings::EndsWith(strings::ToUtf8(s), streetEnding))
      return true;
  }
  return false;
}

bool IsHouseNumberDeepCheck(strings::UniString const & s)
{
  size_t const count = s.size();
  if (count == 0)
    return false;
  if (!IsDigit(s[0]))
    return false;
  if (IsStreetNumber(s))
    return false;
  return (count < 8);
}

bool IsHouseNumber(string const & s)
{
  return (!s.empty() && IsDigit(s[0]));
}

bool IsHouseNumber(strings::UniString const & s)
{
  return (!s.empty() && IsDigit(s[0]));
}

uint8_t PopulationToRank(uint64_t p)
{
  return static_cast<uint8_t>(min(0xFF, base::SignedRound(log(double(p)) / log(1.1))));
}

uint64_t RankToPopulation(uint8_t r)
{
  return static_cast<uint64_t>(pow(1.1, r));
}

} // namespace feature
