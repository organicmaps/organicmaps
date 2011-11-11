#include "../base/SRC_FIRST.hpp"

#include "framework_factory.hpp"
#include "benchmark_framework.hpp"
#include "feature_vec_model.hpp"

#include "../platform/settings.hpp"

template <typename TModel>
Framework<TModel> * FrameworkFactory<TModel>::CreateFramework()
{
  bool benchmarkingEnabled = false;
  (void)Settings::Get("IsBenchmarking", benchmarkingEnabled);

  if (benchmarkingEnabled)
    return new BenchmarkFramework<TModel>();
  else
    return new Framework<TModel>();
}

template class FrameworkFactory<model::FeaturesFetcher>;
