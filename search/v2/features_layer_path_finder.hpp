#pragma once

#include "search/v2/features_layer.hpp"

#include "std/vector.hpp"

class FeaturesVector;
class MwmValue;

namespace my
{
class Cancellable;
}

namespace search
{
namespace v2
{
class FeaturesFilter;
class FeaturesLayerMatcher;

// This class is able to find all paths through a layered graph, with
// vertices as features, and edges as pairs of vertices satisfying
// belongs-to relation. For more details on belongs-to relation see
// documentation for FeaturesLayerMatcher.
//
// In short, this class is able to find all features matching to a
// given interpretation of a search query.
//
// NOTE: this class *IS* thread-safe.
class FeaturesLayerPathFinder
{
public:
  FeaturesLayerPathFinder(my::Cancellable const & cancellable);

  template <typename TFn>
  void ForEachReachableVertex(FeaturesLayerMatcher & matcher, FeaturesFilter & filter,
                              vector<FeaturesLayer const *> const & layers, TFn && fn)
  {
    if (layers.empty())
      return;

    vector<uint32_t> reachable;
    BuildGraph(matcher, filter, layers, reachable);

    for (uint32_t featureId : reachable)
      fn(featureId);
  }

private:
  void BuildGraph(FeaturesLayerMatcher & matcher, FeaturesFilter & filter,
                  vector<FeaturesLayer const *> const & layers, vector<uint32_t> & reachable);

  my::Cancellable const & m_cancellable;
};
}  // namespace v2
}  // namespace search
