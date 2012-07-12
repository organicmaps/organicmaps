#pragma once

#include "framework.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/timer.hpp"
#include "../base/thread.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

struct BenchmarkRectProvider;
class WindowHandle;
class PaintEvent;

/// BenchmarkEngine is class which:
/// - Creates it's own thread
/// - Feeds the Framework with the paint tasks he wants to performs
/// - Wait until each tasks completion
/// - Measure the time of each task and save the result
class BenchmarkEngine : public threads::IRoutine
{
private:

  threads::Thread m_thread;

  double m_paintDuration;
  double m_maxDuration;
  m2::RectD m_maxDurationRect;
  m2::RectD m_curBenchmarkRect;

  string m_startTime;

  struct BenchmarkResult
  {
    string m_name;
    m2::RectD m_rect;
    double m_time;
  };

  vector<BenchmarkResult> m_benchmarkResults;
  my::Timer m_benchmarksTimer;

  struct Benchmark
  {
    shared_ptr<BenchmarkRectProvider> m_provider;
    string m_name;
  };

  vector<Benchmark> m_benchmarks;
  size_t m_curBenchmark;

  Framework * m_framework;

  void BenchmarkCommandFinished();
  bool NextBenchmarkCommand();
  void SaveBenchmarkResults();
  void SendBenchmarkResults();

  void MarkBenchmarkResultsStart();
  void MarkBenchmarkResultsEnd();

  void PrepareMaps();

  void Do();

public:

  BenchmarkEngine(Framework * fw);

  void Start();
};
