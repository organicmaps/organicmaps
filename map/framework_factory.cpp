#include "../base/SRC_FIRST.hpp"

#include "framework_factory.hpp"
#include "benchmark_framework.hpp"
#include "feature_vec_model.hpp"

#include "../platform/platform.hpp"

template <typename TModel>
Framework<TModel> * FrameworkFactory<TModel>::CreateFramework(shared_ptr<WindowHandle> const & wh, size_t bottomShift)
{
  if (GetPlatform().IsBenchmarking())
    return new BenchmarkFramework<TModel>(wh, bottomShift);
  else
    return new Framework<TModel>(wh, bottomShift);
}

template class FrameworkFactory<model::FeaturesFetcher>;
