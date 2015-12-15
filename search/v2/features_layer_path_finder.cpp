#include "search/v2/features_layer_path_finder.hpp"

#include "search/cancel_exception.hpp"
#include "search/v2/features_layer_matcher.hpp"
#include "search/v2/features_filter.hpp"

#include "indexer/features_vector.hpp"

#include "base/cancellable.hpp"

namespace search
{
namespace v2
{
FeaturesLayerPathFinder::FeaturesLayerPathFinder(my::Cancellable const & cancellable)
  : m_cancellable(cancellable)
{
}

void FeaturesLayerPathFinder::BuildGraph(FeaturesLayerMatcher & matcher, FeaturesFilter & filter,
                                         vector<FeaturesLayer const *> const & layers,
                                         vector<uint32_t> & reachable)
{
  if (layers.empty())
    return;

  reachable = *(layers.back()->m_sortedFeatures);

  vector<uint32_t> buffer;

  // The order matters here, as we need to intersect BUILDINGs with
  // STREETs first, and then POIs with BUILDINGs.
  for (size_t i = layers.size() - 1; i != 0; --i)
  {
    BailIfCancelled(m_cancellable);

    if (reachable.empty())
      break;

    if (filter.NeedToFilter(reachable))
    {
      buffer.clear();
      filter.Filter(reachable, MakeBackInsertFunctor(buffer));
      reachable.swap(buffer);
      my::SortUnique(reachable);
    }

    buffer.clear();
    auto addEdge = [&](uint32_t childFeature, uint32_t /* parentFeature */)
    {
      buffer.push_back(childFeature);
    };

    FeaturesLayer parent(*layers[i]);
    parent.m_sortedFeatures = &reachable;
    matcher.Match(*layers[i - 1], parent, addEdge);

    reachable.swap(buffer);
    my::SortUnique(reachable);
  }
}
}  // namespace v2
}  // namespace search
