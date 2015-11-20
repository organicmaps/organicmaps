#pragma once

#include "std/array.hpp"
#include "std/ctime.hpp"
#include "std/string.hpp"

// It would be better to use c++ standard means of
// time parsing and unparsing like
// time_get/time_put (<locale>) or get_time/put_time (<iomanip>).
// But at the moment of writing they was not widely supporded
// and/or worked inkorekd.
//
// All covertions are made in UTC

namespace w3ctime
{
char constexpr kW3CTimeFormat[] = "%Y-%m-%dT%H:%MZ";

// Two more digits for year and one for \n
size_t constexpr kBufSize = sizeof(kW3CTimeFormat) + 2 + 1;

time_t constexpr kNotATime = -1;

inline time_t ParseTime(std::string const & w3ctime) noexcept
{
  std::tm tm{};
  if (strptime(w3ctime.data(), kW3CTimeFormat, &tm) == nullptr)
    return kNotATime;

  return timegm(&tm);
}

inline std::string TimeToString(time_t const timestamp) noexcept
{
  std::tm tm{};
  array<char, kBufSize> buff{};

  gmtime_r(&timestamp, &tm);
  strftime(buff.data(), kBufSize, kW3CTimeFormat, &tm);

  return buff.data();
}

} // namespace w3ctime
