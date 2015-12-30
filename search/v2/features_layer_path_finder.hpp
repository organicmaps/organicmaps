#pragma once

#include "search/v2/features_layer.hpp"

#include "std/vector.hpp"

#if defined(DEBUG)
#include "base/timer.hpp"
#include "std/cstdio.hpp"
#endif  // defined(DEBUG)

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
  void ForEachReachableVertex(FeaturesLayerMatcher & matcher,
                              vector<FeaturesLayer const *> const & layers, TFn && fn)
  {
    if (layers.empty())
      return;

    // TODO (@y): remove following code as soon as
    // FindReachableVertices() will work fast for most cases
    // (significanly less than 1 second).
#if defined(DEBUG)
    fprintf(stderr, "FeaturesLayerPathFinder()\n");
    for (auto const * layer : layers)
      fprintf(stderr, "Layer: %s\n", DebugPrint(*layer).c_str());
    my::Timer timer;
#endif  // defined(DEBUG)

    vector<uint32_t> reachable;
    FindReachableVertices(matcher, layers, reachable);

#if defined(DEBUG)
    fprintf(stderr, "Found: %zu, elapsed: %lf seconds\n", reachable.size(), timer.ElapsedSeconds());
#endif  // defined(DEBUG)

    for (uint32_t featureId : reachable)
      fn(featureId);
  }

private:
  void FindReachableVertices(FeaturesLayerMatcher & matcher,
                             vector<FeaturesLayer const *> const & layers,
                             vector<uint32_t> & reachable);

  my::Cancellable const & m_cancellable;
};
}  // namespace v2
}  // namespace search
