#pragma once
#include "search_common.hpp"

#include "../indexer/search_trie.hpp"

#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/stack.hpp"
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

template <class FilterT> class OffsetIntersecter
{
  typedef unordered_set<uint32_t> SetType;

  FilterT const & m_filter;
  scoped_ptr<SetType> m_prevSet;
  scoped_ptr<SetType> m_set;

public:
  explicit OffsetIntersecter(FilterT const & filter)
    : m_filter(filter), m_set(new SetType()) {}

  void operator() (uint32_t offset)
  {
    if (m_prevSet && !m_prevSet->count(offset))
      return;

    if (!m_filter(offset))
      return;

    m_set->insert(offset);
  }

  void NextStep()
  {
    if (!m_prevSet)
      m_prevSet.reset(new SetType());

    m_prevSet.swap(m_set);
    m_set->clear();
  }

  template <class ToDo> void ForEachResult(ToDo & toDo) const
  {
    for (SetType::const_iterator i = m_prevSet->begin(); i != m_prevSet->end(); ++i)
      toDo(*i);
  }
};

}  // namespace search::impl

struct TrieRootPrefix
{
  TrieIterator const & m_root;
  strings::UniChar const * m_prefix;
  size_t m_prefixSize;

  TrieRootPrefix(TrieIterator const & root, TrieIterator::Edge::EdgeStrT const & edge)
    : m_root(root)
  {
    if (edge.size() == 1)
    {
      m_prefix = 0;
      m_prefixSize = 0;
    }
    else
    {
      m_prefix = &edge[1];
      m_prefixSize = edge.size() - 1;
    }
  }
};

template <class ToDo, class FilterT>
void MatchFeaturesInTrie(vector<vector<strings::UniString> > const & tokens,
                         strings::UniString const & prefix,
                         TrieRootPrefix const & trieRoot,
                         TrieRootPrefix const & catRoot,
                         FilterT const & filter,
                         ToDo & toDo)
{
  impl::OffsetIntersecter<FilterT> intersecter(filter);

  // Match tokens.
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    for (size_t j = 0; j < tokens[i].size(); ++j)
    {
      impl::FullMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize,
                            tokens[i][j], intersecter);
      impl::FullMatchInTrie(catRoot.m_root, catRoot.m_prefix, catRoot.m_prefixSize,
                            tokens[i][j], intersecter);
    }

    intersecter.NextStep();
  }

  // Match prefix.
  if (prefix.size() > 0)
  {
    impl::PrefixMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize,
                            prefix, intersecter);

    intersecter.NextStep();
  }

  intersecter.ForEachResult(toDo);
}


}  // namespace search
