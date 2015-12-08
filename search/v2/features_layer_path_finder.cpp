#include "search/v2/features_layer_path_finder.hpp"

#include "search/v2/features_layer_matcher.hpp"

#include "indexer/features_vector.hpp"

namespace search
{
namespace v2
{
FeaturesLayerPathFinder::FeaturesLayerPathFinder(FeaturesLayerMatcher & matcher)
  : m_matcher(matcher)
{
}

void FeaturesLayerPathFinder::BuildGraph(vector<FeaturesLayer *> const & layers)
{
  m_graph.clear();

  if (layers.empty())
    return;

  // The order matters here, as we need to intersect BUILDINGs with
  // STREETs first, and then POIs with BUILDINGs.
  for (size_t i = layers.size() - 1; i != 0; --i)
  {
    auto & child = (*layers[i - 1]);
    auto & parent = (*layers[i]);
    auto addEdges = [&](uint32_t from, uint32_t to)
    {
      m_graph[to].push_back(from);
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
