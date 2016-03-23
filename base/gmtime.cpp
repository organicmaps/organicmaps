#include "base/gmtime.hpp"

#include "std/target_os.hpp"

namespace my
{
std::tm GmTime(time_t const time)
{
  std::tm result{};
#ifndef OMIM_OS_WINDOWS
  gmtime_r(&time, &result);
#else
  gmtime_s(&result, &time);
#endif
  return result;
}
}  // namespace my
