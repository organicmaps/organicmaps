#include "profiler.hpp"

#include "../std/timer.hpp"

namespace prof
{
  boost::timer timer;

  double * metrics()
  {
    static double data[2 * (metrics_array_size - 1)];
    return data;
  }

  double timer_elapsed()
  {
    return timer.elapsed();
  }
}
