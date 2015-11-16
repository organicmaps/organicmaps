#pragma once

#include "search/v2/features_layer_matcher.hpp"

#include "std/unordered_map.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"

class FeaturesVector;

namespace search
{
namespace v2
{
struct FeaturesLayer;

class FeaturesLayerPathFinder
{
public:
  using TAdjList = vector<uint32_t>;
  using TLayerGraph = unordered_map<uint32_t, TAdjList>;

  FeaturesLayerPathFinder(FeaturesVector const & featuresVector);

  template <typename TFn>
  void ForEachReachableVertex(vector<FeaturesLayer *> const & layers, TFn && fn)
  {
    if (layers.empty())
      return;

    BuildGraph(layers);

    m_visited.clear();
    for (uint32_t featureId : (*layers.back()).m_features)
      Dfs(featureId);

    for (uint32_t featureId : (*layers.front()).m_features)
    {
      if (m_visited.count(featureId) != 0)
        fn(featureId);
    }
  }

private:
  void BuildGraph(vector<FeaturesLayer *> const & layers);

  void Dfs(uint32_t u);

  FeaturesLayerMatcher m_matcher;
  TLayerGraph m_graph;
  unordered_set<uint32_t> m_visited;
};
}  // namespace v2
}  // namespace search
