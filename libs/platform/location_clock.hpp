#pragma once

#include <chrono>

namespace location
{
// Android measurements use elapsed realtime, which includes device suspend time.
struct Clock
{
  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<Clock>;
  static bool constexpr is_steady = true;

  static time_point now();
};
}  // namespace location
