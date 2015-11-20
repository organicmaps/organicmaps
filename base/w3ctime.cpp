#include "w3ctime.hpp"

#include "std/array.hpp"

// It would be better to use c++ standard means of
// time parsing and unparsing like
// time_get/time_put (<locale>) or get_time/put_time (<iomanip>).
// But at the moment of writing they was not widely supporded
// and/or worked incorrect.
//
// All covertions are made in UTC

namespace
{
char constexpr kW3CTimeFormat[] = "%Y-%m-%dT%H:%MZ";

// Two more digits for year.
size_t constexpr kBufSize = sizeof(kW3CTimeFormat) + 2;
} // namespace

namespace base
{
time_t ParseTime(std::string const & w3ctime) noexcept
{
  std::tm tm{};
  if (strptime(w3ctime.data(), kW3CTimeFormat, &tm) == nullptr)
    return -1;

  return timegm(&tm);
}

std::string TimeToString(time_t const timestamp)
{
  std::tm tm{};
  array<char, kBufSize> buff{};

  gmtime_r(&timestamp, &tm);
  if (strftime(buff.data(), kBufSize, kW3CTimeFormat, &tm) == 0)
    buff[0] = 0;

  return buff.data();
}
} // namespace base
