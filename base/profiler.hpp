#pragma once

//#define PROFILER_COMMON
//#define PROFILER_DRAWING
//#define PROFILER_YG

#define LPROF LINFO

namespace prof
{
  enum metric_name
  {
    metrics_array_start = 0,
    /// here custom metrics begin.

    for_each_feature,
    feature_count,
    do_draw,

    yg_upload_data,
    yg_draw_path,
    yg_draw_path_count,
    yg_draw_path_body,
    yg_draw_path_join,
    yg_flush,

    /// here custom metrics ends.
    metrics_array_size
  };

  double * metrics();

  double timer_elapsed();

  template <metric_name name>
  void counter()
  {
    metrics()[(name - 1) * 2 + 1] += 1;
  }

  template <metric_name name>
  inline void start()
  {
     metrics()[(name - 1) * 2] = timer_elapsed();
  };

  template <metric_name name>
  inline void end()
  {
    metrics()[(name - 1) * 2] = timer_elapsed() - metrics()[(name - 1) * 2];
    metrics()[(name - 1) * 2 + 1] += metrics()[(name - 1) * 2];
  }

  template <metric_name name>
  inline void reset()
  {
    metrics()[(name - 1) * 2 + 1] = 0;
  }

  inline void reset()
  {
    for (unsigned i = 0; i < metrics_array_size - 1; ++i)
      metrics()[i * 2 + 1] = 0;
  }

  template <metric_name name>
  double metric()
  {
    return metrics()[(name - 1) * 2 + 1];
  }

  template <metric_name name>
  struct block
  {
    block() {start<name>();}
    ~block() {end<name>();}
  };
}
