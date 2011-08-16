#include "search_trie_matching.hpp"
#include "query.hpp"
#include "string_match.hpp"

#include "../indexer/feature_visibility.hpp"

#include "../std/algorithm.hpp"
#include "../std/functional.hpp"
#include "../std/queue.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"


void search::MatchAgainstTrie(search::impl::Query & query, search::TrieIterator & trieRoot,
                              FeaturesVector const & featuresVector)
{
  strings::UniString const & queryS = query.GetQueryUniText();
  if (queryS.empty())
    return;

  scoped_ptr<search::TrieIterator> pIter(trieRoot.Clone());

  size_t symbolsMatched = 0;
  while (symbolsMatched < queryS.size())
  {
    bool bMatched = false;
    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
    {
      if (pIter->m_edge[i].m_str.size() + symbolsMatched <= queryS.size() &&
          equal(pIter->m_edge[i].m_str.begin(), pIter->m_edge[i].m_str.end(),
                queryS.begin() + symbolsMatched))
      {
        scoped_ptr<search::TrieIterator>(pIter->GoToEdge(i)).swap(pIter);
        symbolsMatched += queryS.size();
        bMatched = true;
        break;
      }
    }
    if (!bMatched)
      return;
  }

  typedef pair<uint8_t, uint32_t> FeatureQueueValueType;
  priority_queue<
      FeatureQueueValueType,
      vector<FeatureQueueValueType>,
      greater<FeatureQueueValueType> > featureQueue;
  priority_queue<pair<uint8_t, search::TrieIterator *> > trieQueue;
  trieQueue.push(make_pair(uint8_t(-1), pIter->Clone()));
  size_t const featuresToMatch = 10;
  while (!trieQueue.empty() &&
         (featureQueue.size() < featuresToMatch ||
          trieQueue.top().first >= featureQueue.top().first))
  {
    scoped_ptr<search::TrieIterator> pIter(trieQueue.top().second);
    trieQueue.pop();
    for (size_t i = 0; i < pIter->m_value.size(); ++i)
      featureQueue.push(make_pair(pIter->m_value[i].m_rank, pIter->m_value[i].m_featureId));
    while (featureQueue.size() > featuresToMatch)
      featureQueue.pop();
    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
      trieQueue.push(make_pair(pIter->m_edge[i].m_value, pIter->GoToEdge(i)));
  }
  while (!featureQueue.empty())
  {
    // TODO: Deal with different names of the same feature.
    // TODO: Best name for features.
    FeatureType feature;
    featuresVector.Get(featureQueue.top().second, feature);
    FeatureType::GetNamesFn names;
    feature.ForEachNameRef(names);
    string displayName;
    for (size_t i = 0; i < names.m_size; ++i)
    {
      strings::UniString const s = NormalizeAndSimplifyString(names.m_names[i]);
      if (s.size() >= queryS.size() && equal(queryS.begin(), queryS.end(), s.begin()))
      {
        displayName = names.m_names[i];
        break;
      }
    }

    /// @todo Need Yuri's review.
    //ASSERT(!displayName.empty(), ());
    if (!displayName.empty())
    {
      query.AddResult(impl::IntermediateResult(query.GetViewport(), feature, displayName, 0,
                                               feature::DrawableScaleRangeForText(feature).first));
    }

    featureQueue.pop();
  }
}
