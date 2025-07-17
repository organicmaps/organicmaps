#include "base/gmtime.hpp"

#include "std/target_os.hpp"

namespace base
{
std::tm GmTime(time_t time)
{
  std::tm result{};
#ifndef OMIM_OS_WINDOWS
  gmtime_r(&time, &result);
#else
  gmtime_s(&result, &time);
#endif
  return result;
}
}  // namespace base
