#include "search_trie_matching.hpp"
#include "query.hpp"

#include "../indexer/feature_visibility.hpp"
#include "../indexer/string_search_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/functional.hpp"
#include "../std/queue.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"


namespace
{
  template <class SrcIterT, class CompIterT>
  size_t CalcEqualLength(SrcIterT b, SrcIterT e, CompIterT bC, CompIterT eC)
  {
    size_t count = 0;
    while ((b != e) && (bC != eC) && (*b++ == *bC++))
      ++count;
    return count;
  }
}

void search::MatchAgainstTrie(search::impl::Query & query, search::TrieIterator & trieRoot,
                              FeaturesVector const & featuresVector)
{
  strings::UniString const & queryS = query.GetQueryUniText();
  if (queryS.empty())
    return;

  scoped_ptr<search::TrieIterator> pIter(trieRoot.Clone());

  size_t symbolsMatched = 0;
  size_t const szQuery = queryS.size();
  while (symbolsMatched < szQuery)
  {
    bool bMatched = false;

    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
    {
      size_t const szEdge = pIter->m_edge[i].m_str.size();

      size_t const count = CalcEqualLength(pIter->m_edge[i].m_str.begin(),
                                           pIter->m_edge[i].m_str.end(),
                                           queryS.begin() + symbolsMatched,
                                           queryS.end());

      if ((count > 0) && (count == szEdge || szQuery == count + symbolsMatched))
      {
        scoped_ptr<search::TrieIterator>(pIter->GoToEdge(i)).swap(pIter);
        symbolsMatched += count;
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
          trieQueue.top().first > featureQueue.top().first))
  {
    scoped_ptr<search::TrieIterator> pIter(trieQueue.top().second);
    trieQueue.pop();
    for (size_t i = 0; i < pIter->m_value.size(); ++i)
    {
      if (featureQueue.size() < featuresToMatch)
        featureQueue.push(make_pair(pIter->m_value[i].m_rank, pIter->m_value[i].m_featureId));
      else if (pIter->m_value[i].m_rank > featureQueue.top().first)
      {
        featureQueue.push(make_pair(pIter->m_value[i].m_rank, pIter->m_value[i].m_featureId));
        featureQueue.pop();
      }
    }
    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
      if (featureQueue.size() < featuresToMatch ||
          pIter->m_edge[i].m_value > featureQueue.top().first)
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
