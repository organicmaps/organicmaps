#include "search/v2/features_layer_path_finder.hpp"

#include "search/v2/features_layer_matcher.hpp"

#include "indexer/features_vector.hpp"

namespace search
{
namespace v2
{
void FeaturesLayerPathFinder::BuildGraph(FeaturesLayerMatcher & matcher,
                                         vector<FeaturesLayer const *> const & layers,
                                         vector<uint32_t> & reachable)
{
  if (layers.empty())
    return;

  FeaturesLayer child;

  reachable = layers.back()->m_sortedFeatures;

  vector<uint32_t> tmpBuffer;

  // The order matters here, as we need to intersect BUILDINGs with
  // STREETs first, and then POIs with BUILDINGs.
  for (size_t i = layers.size() - 1; i != 0; --i)
  {
    tmpBuffer.clear();
    auto addEdge = [&](uint32_t childFeature, uint32_t /* parentFeature */)
    {
      tmpBuffer.push_back(childFeature);
    };

    matcher.Match(*layers[i - 1], reachable, layers[i]->m_type, addEdge);

    my::SortUnique(tmpBuffer);
    reachable.swap(tmpBuffer);
  }
}
}  // namespace v2
}  // namespace search
