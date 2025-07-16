#pragma once

#include "search/cancel_exception.hpp"
#include "search/features_layer.hpp"
#include "search/intersection_result.hpp"

#ifdef DEBUG
#include "base/logging.hpp"
#include "base/timer.hpp"
#endif  // DEBUG

#include <utility>
#include <vector>

class FeaturesVector;
class MwmValue;

namespace base
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
  // An internal mode. The modes should produce similar results
  // and differ only in efficiency: a mode that is faster
  // for a search query may be slower for another.
  // Modes other than MODE_AUTO should be used only by the testing code.
  enum Mode
  {
    MODE_AUTO,
    MODE_TOP_DOWN,
    MODE_BOTTOM_UP
  };

  FeaturesLayerPathFinder(base::Cancellable const & cancellable) : m_cancellable(cancellable) {}

  template <typename TFn>
  void ForEachReachableVertex(FeaturesLayerMatcher & matcher, std::vector<FeaturesLayer const *> const & layers,
                              TFn && fn)
  {
    if (layers.empty())
      return;

// TODO (@y): remove following code as soon as
// FindReachableVertices() will work fast for most cases
// (significantly less than 1 second).
#ifdef DEBUG
    for (auto const * layer : layers)
      LOG(LDEBUG, (DebugPrint(*layer)));

    size_t count = 0;
    base::Timer timer;
#endif  // DEBUG

    FindReachableVertices(matcher, layers, [&](IntersectionResult const & r)
    {
      fn(r);
#ifdef DEBUG
      ++count;
#endif  // DEBUG
    });

#ifdef DEBUG
    LOG(LDEBUG, ("Found:", count, "elapsed:", timer.ElapsedSeconds(), "seconds"));
#endif  // DEBUG
  }

  static void SetModeForTesting(Mode mode) { m_mode = mode; }

private:
  void BailIfCancelled() { ::search::BailIfCancelled(m_cancellable); }

  template <class FnT>
  void FindReachableVertices(FeaturesLayerMatcher & matcher, std::vector<FeaturesLayer const *> const & layers,
                             FnT && fn);

  // Tries to find all |reachable| features from the lowest layer in a
  // high level -> low level pass.
  template <class FnT>
  void FindReachableVerticesTopDown(FeaturesLayerMatcher & matcher, std::vector<FeaturesLayer const *> const & layers,
                                    FnT && fn);

  // Tries to find all |reachable| features from the lowest layer in a
  // low level -> high level pass.
  template <class FnT>
  void FindReachableVerticesBottomUp(FeaturesLayerMatcher & matcher, std::vector<FeaturesLayer const *> const & layers,
                                     FnT && fn);

  base::Cancellable const & m_cancellable;

  static Mode m_mode;
};
}  // namespace search
