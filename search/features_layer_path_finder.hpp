#pragma once

#include "search/features_layer.hpp"
#include "search/intersection_result.hpp"

#include "std/vector.hpp"

#if defined(DEBUG)
#include "base/logging.hpp"
#include "base/timer.hpp"
#endif  // defined(DEBUG)

class FeaturesVector;
class MwmValue;

namespace my
{
class Cancellable;
}

namespace search
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
// (significantly less than 1 second).
#if defined(DEBUG)
    for (auto const * layer : layers)
      LOG(LINFO, (DebugPrint(*layer)));
    my::Timer timer;
#endif  // defined(DEBUG)

    vector<IntersectionResult> results;
    FindReachableVertices(matcher, layers, results);

#if defined(DEBUG)
    LOG(LINFO, ("Found:", results.size(), "elapsed:", timer.ElapsedSeconds(), "seconds"));
#endif  // defined(DEBUG)

    for_each(results.begin(), results.end(), forward<TFn>(fn));
  }

private:
  void FindReachableVertices(FeaturesLayerMatcher & matcher,
                             vector<FeaturesLayer const *> const & layers,
                             vector<IntersectionResult> & results);

  // Tries to find all |reachable| features from the lowest layer in a
  // high level -> low level pass.
  void FindReachableVerticesTopDown(FeaturesLayerMatcher & matcher,
                                    vector<FeaturesLayer const *> const & layers,
                                    vector<IntersectionResult> & results);

  // Tries to find all |reachable| features from the lowest layer in a
  // low level -> high level pass.
  void FindReachableVerticesBottomUp(FeaturesLayerMatcher & matcher,
                                     vector<FeaturesLayer const *> const & layers,
                                     vector<IntersectionResult> & results);

  my::Cancellable const & m_cancellable;
};
}  // namespace search
