#pragma once
#include "../indexer/search_trie.hpp"
#include "../base/string_utils.hpp"
#include "../base/base.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/queue.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/unordered_map.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

namespace search
{
namespace impl
{

template <class SrcIterT, class CompIterT>
size_t CalcEqualLength(SrcIterT b, SrcIterT e, CompIterT bC, CompIterT eC)
{
  size_t count = 0;
  while ((b != e) && (bC != eC) && (*b++ == *bC++))
    ++count;
  return count;
}

TrieIterator * MoveTrieIteratorToString(TrieIterator const & trieRoot,
                                        strings::UniString const & queryS,
                                        size_t & symbolsMatched)
{
  scoped_ptr<search::TrieIterator> pIter(trieRoot.Clone());
  symbolsMatched = 0;
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
      return NULL;
  }
  return pIter->Clone();
}

template <typename F>
void FullMatchInTrie(TrieIterator const & trieRoot,
                     strings::UniString const & s,
                     F & f)
{
  size_t symbolsMatched = 0;
  scoped_ptr<search::TrieIterator> pIter(MoveTrieIteratorToString(trieRoot, s, symbolsMatched));
  if (!pIter || symbolsMatched != s.size())
    return;
  for (size_t i = 0; i < pIter->m_value.size(); ++i)
    f(pIter->m_value[i].m_featureId, pIter->m_value[i].m_rank);
}

template <typename F>
void PrefixMatchInTrie(TrieIterator const & trieRoot,
                       strings::UniString const & s,
                       F & f)
{
  size_t symbolsMatched = 0;
  scoped_ptr<search::TrieIterator> pIter(MoveTrieIteratorToString(trieRoot, s, symbolsMatched));
  if (!pIter)
    return;

  priority_queue<pair<uint8_t, search::TrieIterator *> > trieQueue;
  trieQueue.push(make_pair(uint8_t(-1), pIter->Clone()));

  uint8_t maxRank = 0;
  for (size_t i = 0; i < pIter->m_edge.size(); ++i)
    maxRank = max(maxRank, pIter->m_edge[i].m_value);

  int featuresStillToMatch = 100000;
  while (!trieQueue.empty() && (featuresStillToMatch > 0 || trieQueue.top().first == maxRank))
  {
    scoped_ptr<search::TrieIterator> pIter(trieQueue.top().second);
    trieQueue.pop();
    for (size_t i = 0; i < pIter->m_value.size(); ++i)
      f(pIter->m_value[i].m_featureId, pIter->m_value[i].m_rank);
    featuresStillToMatch -= pIter->m_value.size();
    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
        trieQueue.push(make_pair(pIter->m_edge[i].m_value, pIter->GoToEdge(i)));
  }
}

struct OffsetIntersecter
{
  typedef unordered_map<uint32_t, uint16_t> MapType;

  MapType m_prevMap;
  MapType m_map;
  bool m_bFirstStep;

  void operator() (uint32_t offset, uint8_t rank)
  {
    uint16_t prevRankSum = 0;
    if (!m_bFirstStep)
    {
      MapType::const_iterator it = m_prevMap.find(offset);
      if (it == m_prevMap.end())
        return;
      prevRankSum = it->second;
    }

    uint16_t & mappedRank = m_map[offset];
    mappedRank = max(mappedRank, static_cast<uint16_t>(prevRankSum + rank));
  }

  void NextStep()
  {
    m_prevMap.swap(m_map);
    m_map.clear();
    m_bFirstStep = false;
  }
};

}  // namespace search::impl

template <typename F>
void MatchFeaturesInTrie(strings::UniString const * tokens, size_t tokenCount,
                         strings::UniString const & prefix,
                         TrieIterator const & trieRoot,
                         F & f,
                         size_t resultsNeeded)
{
  impl::OffsetIntersecter intersecter;

  // Match tokens.
  for (size_t i = 0; i < tokenCount; ++i)
  {
    impl::FullMatchInTrie(trieRoot, tokens[i], intersecter);
    intersecter.NextStep();
  }

  // Match prefix.
  if (prefix.size() > 0)
  {
    impl::PrefixMatchInTrie(trieRoot, prefix, intersecter);
    intersecter.NextStep();
  }

  typedef vector<pair<uint32_t, uint16_t> > ResType;
  ResType res(intersecter.m_prevMap.begin(), intersecter.m_prevMap.end());

  if (res.size() > resultsNeeded)
  {
    partial_sort(res.begin(), res.begin() + resultsNeeded, res.end(),
                 bind(&ResType::value_type::second, _1) > bind(&ResType::value_type::second, _2));
    res.resize(resultsNeeded);
  }

  for (ResType::const_iterator it = res.begin(); it != res.end(); ++it)
    f(it->first);
}


}  // namespace search
