#include "map/nogps/clock.hpp"

#include "std/target_os.hpp"

#include <chrono>

#if defined(OMIM_OS_ANDROID)
#include <ctime>
#endif

namespace nogps
{
int64_t SystemClock::NowMs() const
{
#if defined(OMIM_OS_ANDROID)
  // The base of SystemClock.elapsedRealtime() and Location.getElapsedRealtimeNanos().
  timespec ts;
  clock_gettime(CLOCK_BOOTTIME, &ts);
  return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1'000'000;
#else
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
#endif
}

int64_t SystemClock::UnixNowMs() const
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
}  // namespace nogps
