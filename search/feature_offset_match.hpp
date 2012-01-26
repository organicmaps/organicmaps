#pragma once
#include "search_common.hpp"
#include "../indexer/search_trie.hpp"
#include "../base/string_utils.hpp"
#include "../base/base.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/stack.hpp"
#include "../std/unordered_map.hpp"
#include "../std/unordered_set.hpp"
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
                     strings::UniChar const * rootPrefix,
                     size_t rootPrefixSize,
                     strings::UniString s,
                     F & f)
{
  if (rootPrefixSize > 0)
  {
    if (s.size() < rootPrefixSize ||
        !StartsWith(s.begin(), s.end(), rootPrefix, rootPrefix + rootPrefixSize))
      return;
    s = strings::UniString(s.begin() + rootPrefixSize, s.end());
  }

  size_t symbolsMatched = 0;
  scoped_ptr<search::TrieIterator> pIter(MoveTrieIteratorToString(trieRoot, s, symbolsMatched));
  if (!pIter || symbolsMatched != s.size())
    return;
  for (size_t i = 0; i < pIter->m_value.size(); ++i)
    f(pIter->m_value[i].m_featureId);
}

template <typename F>
void PrefixMatchInTrie(TrieIterator const & trieRoot,
                       strings::UniChar const * rootPrefix,
                       size_t rootPrefixSize,
                       strings::UniString s,
                       F & f)
{
  if (rootPrefixSize > 0)
  {
    if (s.size() < rootPrefixSize ||
        !StartsWith(s.begin(), s.end(), rootPrefix, rootPrefix + rootPrefixSize))
      return;
    s = strings::UniString(s.begin() + rootPrefixSize, s.end());
  }

  stack<search::TrieIterator *> trieQueue;
  {
    size_t symbolsMatched = 0;
    search::TrieIterator * const pRootIter = MoveTrieIteratorToString(trieRoot, s, symbolsMatched);
    if (!pRootIter)
      return;
    trieQueue.push(pRootIter);
  }

  while (!trieQueue.empty())
  {
    scoped_ptr<search::TrieIterator> pIter(trieQueue.top());
    trieQueue.pop();
    for (size_t i = 0; i < pIter->m_value.size(); ++i)
      f(pIter->m_value[i].m_featureId);
    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
        trieQueue.push(pIter->GoToEdge(i));
  }
}

template <class FilterT> struct OffsetIntersecter
{
  typedef unordered_set<uint32_t> SetType;

  FilterT const & m_filter;
  SetType m_prevSet;
  SetType m_set;
  bool m_bFirstStep;

  explicit OffsetIntersecter(FilterT const & filter)
    : m_filter(filter), m_bFirstStep(true) {}

  void operator() (uint32_t offset)
  {
    if (!m_filter(offset))
      return;

    m_set.insert(offset);
  }

  void NextStep()
  {
    m_prevSet.swap(m_set);
    m_set.clear();
    m_bFirstStep = false;
  }
};

}  // namespace search::impl

template <class ToDo, class FilterT>
void MatchFeaturesInTrie(vector<vector<strings::UniString> > const & tokens,
                         strings::UniString const & prefix,
                         TrieIterator const & trieRoot,
                         strings::UniChar const * commonPrefix,
                         size_t commonPrefixSize,
                         FilterT const & filter,
                         ToDo & toDo,
                         size_t resultsNeeded)
{
  impl::OffsetIntersecter<FilterT> intersecter(filter);

  // Match tokens.
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    for (size_t j = 0; j < tokens[i].size(); ++j)
      impl::FullMatchInTrie(trieRoot, commonPrefix, commonPrefixSize, tokens[i][j], intersecter);

    intersecter.NextStep();
  }

  // Match prefix.
  if (prefix.size() > 0)
  {
    impl::PrefixMatchInTrie(trieRoot, commonPrefix, commonPrefixSize, prefix, intersecter);

    intersecter.NextStep();
  }

  typedef typename impl::OffsetIntersecter<FilterT>::SetType::const_iterator IT;
  for (IT it = intersecter.m_prevSet.begin(); it != intersecter.m_prevSet.end(); ++it)
    toDo(*it);
}


}  // namespace search
