#include "framework_factory.hpp"
#include "benchmark_framework.hpp"

#include "../platform/settings.hpp"

Framework * FrameworkFactory::CreateFramework()
{
  bool benchmarkingEnabled = false;
  (void)Settings::Get("IsBenchmarking", benchmarkingEnabled);

  if (benchmarkingEnabled)
    return new BenchmarkFramework();
  else
    return new Framework();
}
