#include "search/v2/features_layer_path_finder.hpp"

#include "search/cancel_exception.hpp"
#include "search/v2/features_layer_matcher.hpp"

#include "indexer/features_vector.hpp"

#include "base/cancellable.hpp"

namespace search
{
namespace v2
{
namespace
{
// This function tries to estimate amount of work needed to perform an
// intersection pass on a sequence of layers.
template<typename TIt>
uint64_t CalcPassCost(TIt begin, TIt end)
{
  uint64_t cost = 0;

  if (begin == end)
    return cost;

  uint64_t reachable = max((*begin)->m_sortedFeatures->size(), size_t(1));
  for (++begin; begin != end; ++begin)
  {
    uint64_t const layer = max((*begin)->m_sortedFeatures->size(), size_t(1));
    cost += layer * reachable;
    reachable = min(reachable, layer);
  }
  return cost;
}

uint64_t CalcTopDownPassCost(vector<FeaturesLayer const *> const & layers)
{
  return CalcPassCost(layers.rbegin(), layers.rend());
}

uint64_t CalcBottomUpPassCost(vector<FeaturesLayer const *> const & layers)
{
  return CalcPassCost(layers.begin(), layers.end());
}
}  // namespace

FeaturesLayerPathFinder::FeaturesLayerPathFinder(my::Cancellable const & cancellable)
  : m_cancellable(cancellable)
{
}

void FeaturesLayerPathFinder::FindReachableVertices(FeaturesLayerMatcher & matcher,
                                                    vector<FeaturesLayer const *> const & layers,
                                                    vector<uint32_t> & reachable)
{
  if (layers.empty())
    return;

  uint64_t const topDownCost = CalcTopDownPassCost(layers);
  uint64_t const bottomUpCost = CalcBottomUpPassCost(layers);

  if (bottomUpCost < topDownCost)
    FindReachableVerticesBottomUp(matcher, layers, reachable);
  else
    FindReachableVerticesTopDown(matcher, layers, reachable);
}

void FeaturesLayerPathFinder::FindReachableVerticesTopDown(
    FeaturesLayerMatcher & matcher, vector<FeaturesLayer const *> const & layers,
    vector<uint32_t> & reachable)
{
  reachable = *(layers.back()->m_sortedFeatures);

  vector<uint32_t> buffer;

  auto addEdge = [&](uint32_t childFeature, uint32_t /* parentFeature */)
  {
    buffer.push_back(childFeature);
  };

  // The order matters here, as we need to intersect BUILDINGs with
  // STREETs first, and then POIs with BUILDINGs.
  for (size_t i = layers.size() - 1; i != 0; --i)
  {
    BailIfCancelled(m_cancellable);

    if (reachable.empty())
      break;

    FeaturesLayer parent(*layers[i]);
    parent.m_sortedFeatures = &reachable;
    parent.m_hasDelayedFeatures = false;

    FeaturesLayer child(*layers[i - 1]);
    child.m_hasDelayedFeatures = false;
    if (child.m_type == SearchModel::SEARCH_TYPE_BUILDING)
    {
      vector<string> tokens;
      NormalizeHouseNumber(child.m_subQuery, tokens);

      // When first token of |child.m_subQuery| looks like house
      // number, additional search of matching buildings must be
      // performed during POI-BUILDING or BUILDING-STREET
      // intersections.
      bool const looksLikeHouseNumber = !tokens.empty() && feature::IsHouseNumber(tokens.front());
      child.m_hasDelayedFeatures = looksLikeHouseNumber;
    }

    buffer.clear();
    matcher.Match(child, parent, addEdge);
    reachable.swap(buffer);
    my::SortUnique(reachable);
  }
}

void FeaturesLayerPathFinder::FindReachableVerticesBottomUp(
    FeaturesLayerMatcher & matcher, vector<FeaturesLayer const *> const & layers,
    vector<uint32_t> & reachable)
{
  reachable = *(layers.front()->m_sortedFeatures);

  unordered_map<uint32_t, uint32_t> parentGraph;
  for (uint32_t id : reachable)
    parentGraph[id] = id;

  unordered_map<uint32_t, uint32_t> nparentGraph;
  vector<uint32_t> buffer;

  // We're going from low levels to high levels and we need to report
  // features from the lowest layer that are reachable from the higher
  // layer by some path. Therefore, following code checks and updates
  // edges from features in |reachable| to features on the lowest
  // layer.
  auto addEdge = [&](uint32_t childFeature, uint32_t parentFeature)
  {
    auto const it = parentGraph.find(childFeature);
    if (it == parentGraph.cend())
      return;
    nparentGraph[parentFeature] = it->second;
    buffer.push_back(parentFeature);
  };


  for (size_t i = 0; i + 1 != layers.size(); ++i)
  {
    BailIfCancelled(m_cancellable);

    if (reachable.empty())
      break;

    FeaturesLayer child(*layers[i]);
    child.m_sortedFeatures = &reachable;
    child.m_hasDelayedFeatures = false;

    FeaturesLayer parent(*layers[i + 1]);
    parent.m_hasDelayedFeatures = false;
    if (parent.m_type == SearchModel::SEARCH_TYPE_BUILDING)
    {
      vector<string> tokens;
      NormalizeHouseNumber(parent.m_subQuery, tokens);
      bool const looksLikeHouseNumber = !tokens.empty() && feature::IsHouseNumber(tokens.front());
      parent.m_hasDelayedFeatures = looksLikeHouseNumber;
    }

    buffer.clear();
    nparentGraph.clear();
    matcher.Match(child, parent, addEdge);
    parentGraph.swap(nparentGraph);
    reachable.swap(buffer);
    my::SortUnique(reachable);
  }

  buffer.clear();
  for (uint32_t id : reachable)
  {
    ASSERT_NOT_EQUAL(parentGraph.count(id), 0, ());
    buffer.push_back(parentGraph[id]);
  }
  reachable.swap(buffer);
  my::SortUnique(reachable);
}
}  // namespace v2
}  // namespace search
