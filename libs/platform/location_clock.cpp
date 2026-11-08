#include "platform/location_clock.hpp"

#include "base/assert.hpp"

#include "std/target_os.hpp"

#include <ctime>

namespace location
{
Clock::time_point Clock::now()
{
#ifdef OMIM_OS_ANDROID
  timespec time{};
  CHECK_EQUAL(clock_gettime(CLOCK_BOOTTIME, &time), 0, ());
  return time_point(std::chrono::seconds(time.tv_sec) + std::chrono::nanoseconds(time.tv_nsec));
#else
  return time_point(std::chrono::duration_cast<duration>(std::chrono::steady_clock::now().time_since_epoch()));
#endif
}
}  // namespace location
