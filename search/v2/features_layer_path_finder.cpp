#include "search/v2/features_layer_path_finder.hpp"

#include "search/v2/features_layer.hpp"

#include "indexer/features_vector.hpp"

namespace search
{
namespace v2
{
FeaturesLayerPathFinder::FeaturesLayerPathFinder(MwmValue & value,
                                                 FeaturesVector const & featuresVector)
  : m_matcher(value, featuresVector)
{
}

void FeaturesLayerPathFinder::BuildGraph(vector<FeaturesLayer *> const & layers)
{
  m_graph.clear();

  for (size_t i = 0; i + 1 < layers.size(); ++i)
  {
    auto & child = (*layers[i]);
    auto & parent = (*layers[i + 1]);
    auto addEdges = [&](uint32_t from, uint32_t to)
    {
      m_graph[parent.m_sortedFeatures[to]].push_back(child.m_sortedFeatures[from]);
    };
    m_matcher.Match(child, parent, addEdges);
  }
}

void FeaturesLayerPathFinder::Dfs(uint32_t u)
{
  m_visited.insert(u);
  auto const adj = m_graph.find(u);
  if (adj == m_graph.end())
    return;
  for (uint32_t v : adj->second)
  {
    if (m_visited.count(v) == 0)
      Dfs(v);
  }
}
}  // namespace v2
}  // namespace search
