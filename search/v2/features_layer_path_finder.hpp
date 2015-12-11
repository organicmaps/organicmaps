#pragma once

#include "search/v2/features_layer.hpp"

#include "std/vector.hpp"

class FeaturesVector;
class MwmValue;

namespace search
{
namespace v2
{
class FeaturesLayerMatcher;

class FeaturesLayerPathFinder
{
public:
  template <typename TFn>
  void ForEachReachableVertex(FeaturesLayerMatcher & matcher,
                              vector<FeaturesLayer const *> const & layers, TFn && fn)
  {
    if (layers.empty())
      return;

    vector<uint32_t> reachable;
    BuildGraph(matcher, layers, reachable);

    for (uint32_t featureId : reachable)
      fn(featureId);
  }

private:
  void BuildGraph(FeaturesLayerMatcher & matcher, vector<FeaturesLayer const *> const & layers,
                  vector<uint32_t> & reachable);
};
}  // namespace v2
}  // namespace search
