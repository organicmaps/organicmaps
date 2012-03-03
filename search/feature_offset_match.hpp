#pragma once
#include "search_common.hpp"

#include "../indexer/search_trie.hpp"

#include "../base/string_utils.hpp"
//#include "../base/logging.hpp"

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
                                        size_t & symbolsMatched,
                                        bool & bFullEdgeMatched)
{
  symbolsMatched = 0;
  bFullEdgeMatched = false;

  scoped_ptr<search::TrieIterator> pIter(trieRoot.Clone());

  size_t const szQuery = queryS.size();
  while (symbolsMatched < szQuery)
  {
    bool bMatched = false;

    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
    {
      size_t const szEdge = pIter->m_edge[i].m_str.size();

      size_t const count = CalcEqualLength(
                                        pIter->m_edge[i].m_str.begin(),
                                        pIter->m_edge[i].m_str.end(),
                                        queryS.begin() + symbolsMatched,
                                        queryS.end());

      if ((count > 0) && (count == szEdge || szQuery == count + symbolsMatched))
      {
        scoped_ptr<search::TrieIterator>(pIter->GoToEdge(i)).swap(pIter);

        bFullEdgeMatched = (count == szEdge);
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

namespace
{
  bool CheckMatchString(strings::UniChar const * rootPrefix,
                        size_t rootPrefixSize,
                        strings::UniString & s)
  {
    if (rootPrefixSize > 0)
    {
      if (s.size() < rootPrefixSize ||
          !StartsWith(s.begin(), s.end(), rootPrefix, rootPrefix + rootPrefixSize))
        return false;

      s = strings::UniString(s.begin() + rootPrefixSize, s.end());
    }

    return true;
  }
}

template <typename F>
void FullMatchInTrie(TrieIterator const & trieRoot,
                     strings::UniChar const * rootPrefix,
                     size_t rootPrefixSize,
                     strings::UniString s,
                     F & f)
{
  if (!CheckMatchString(rootPrefix, rootPrefixSize, s))
      return;

  size_t symbolsMatched = 0;
  bool bFullEdgeMatched;
  scoped_ptr<search::TrieIterator> pIter(
        MoveTrieIteratorToString(trieRoot, s, symbolsMatched, bFullEdgeMatched));

  if (!pIter || !bFullEdgeMatched || symbolsMatched != s.size())
    return;

  ASSERT_EQUAL ( symbolsMatched, s.size(), () );
  for (size_t i = 0; i < pIter->m_value.size(); ++i)
    f(pIter->m_value[i]);
}

template <typename F>
void PrefixMatchInTrie(TrieIterator const & trieRoot,
                       strings::UniChar const * rootPrefix,
                       size_t rootPrefixSize,
                       strings::UniString s,
                       F & f)
{
  if (!CheckMatchString(rootPrefix, rootPrefixSize, s))
      return;

  stack<search::TrieIterator *> trieQueue;
  {
    size_t symbolsMatched = 0;
    bool bFullEdgeMatched;
    search::TrieIterator * const pRootIter =
        MoveTrieIteratorToString(trieRoot, s, symbolsMatched, bFullEdgeMatched);

    UNUSED_VALUE(symbolsMatched);
    UNUSED_VALUE(bFullEdgeMatched);

    if (!pRootIter)
      return;

    trieQueue.push(pRootIter);
  }

  while (!trieQueue.empty())
  {
    scoped_ptr<search::TrieIterator> pIter(trieQueue.top());
    trieQueue.pop();

    for (size_t i = 0; i < pIter->m_value.size(); ++i)
      f(pIter->m_value[i]);

    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
        trieQueue.push(pIter->GoToEdge(i));
  }
}

template <class FilterT> class OffsetIntersecter
{
  typedef trie::ValueReader::ValueType ValueT;

  struct HashFn
  {
    size_t operator() (ValueT const & v) const
    {
      return (boost::hash_value(v.m_featureId));
    }
  };
  struct EqualFn
  {
    bool operator() (ValueT const & v1, ValueT const & v2) const
    {
      return (v1.m_featureId == v2.m_featureId);
    }
  };

  typedef unordered_set<ValueT, HashFn, EqualFn> SetType;

  FilterT const & m_filter;
  scoped_ptr<SetType> m_prevSet;
  scoped_ptr<SetType> m_set;

public:
  explicit OffsetIntersecter(FilterT const & filter)
    : m_filter(filter), m_set(new SetType())
  {
  }

  void operator() (ValueT const & v)
  {
    if (m_prevSet && !m_prevSet->count(v))
      return;

    if (!m_filter(v.m_featureId))
      return;

    m_set->insert(v);
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
    if (m_prevSet)
    {
      for (typename SetType::const_iterator i = m_prevSet->begin(); i != m_prevSet->end(); ++i)
        toDo(*i);
    }
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
                         vector<strings::UniString> const & prefixTokens,
                         TrieRootPrefix const & trieRoot,
                         TrieRootPrefix const & catRoot,
                         FilterT const & filter,
                         ToDo & toDo)
{
  //LOG(LDEBUG, ("Tokens: ", tokens));
  //LOG(LDEBUG, ("Prefix: ", prefixTokens));

  impl::OffsetIntersecter<FilterT> intersecter(filter);

  // Match tokens.
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    for (size_t j = 0; j < tokens[i].size(); ++j)
    {
      ASSERT ( !tokens[i][j].empty(), () );

      impl::FullMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize,
                            tokens[i][j], intersecter);

      impl::FullMatchInTrie(catRoot.m_root, catRoot.m_prefix, catRoot.m_prefixSize,
                            tokens[i][j], intersecter);
    }

    intersecter.NextStep();
  }

  // Match prefix.
  size_t const prefixCount = prefixTokens.size();
  for (size_t i = 0; i < prefixCount; ++i)
  {
    ASSERT ( !prefixTokens[i].empty(), () );

    impl::PrefixMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize,
                            prefixTokens[i], intersecter);

    impl::FullMatchInTrie(catRoot.m_root, catRoot.m_prefix, catRoot.m_prefixSize,
                          prefixTokens[i], intersecter);
  }

  if (prefixCount > 0)
    intersecter.NextStep();

  intersecter.ForEachResult(toDo);
}

}  // namespace search
