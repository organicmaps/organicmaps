#include "feature_impl.hpp"

#include "../base/string_utils.hpp"


namespace feature
{

bool IsDigit(int c)
{
  return (c <= 127 && isdigit(c));
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

/// Check that token can be house number.
bool IsHouseNumber(strings::UniString const & s)
{
  size_t const count = s.size();
  /// @todo Probably, call some check function from House::
  return (count > 0 && count < 8 && IsDigit(s[0]));
}

bool IsHouseNumber(string const & s)
{
  return (!s.empty() && IsDigit(s[0]));
}

}
