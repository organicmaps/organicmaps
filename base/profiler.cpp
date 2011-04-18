#include "profiler.hpp"

#include "timer.hpp"

namespace prof
{
  my::Timer timer;

  double * metrics()
  {
    static double data[2 * (metrics_array_size - 1)];
    return data;
  }

  double timer_elapsed()
  {
    return timer.ElapsedSeconds();
  }
}
