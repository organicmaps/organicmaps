#pragma once
#include "search/search_common.hpp"

#include "indexer/search_trie.hpp"

#include "base/string_utils.hpp"
#include "base/stl_add.hpp"
#include "base/scope_guard.hpp"
#include "base/mutex.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_set.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"
#include "std/target_os.hpp"

//#include "../sparsehash/dense_hash_set.hpp"


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

  unique_ptr<search::TrieIterator> pIter(trieRoot.Clone());

  size_t const szQuery = queryS.size();
  while (symbolsMatched < szQuery)
  {
    bool bMatched = false;

    ASSERT_LESS(pIter->m_edge.size(), std::numeric_limits<uint32_t>::max(), ());
    uint32_t const edgeCount = static_cast<uint32_t>(pIter->m_edge.size());

    for (uint32_t i = 0; i < edgeCount; ++i)
    {
      size_t const szEdge = pIter->m_edge[i].m_str.size();

      size_t const count = CalcEqualLength(
                                        pIter->m_edge[i].m_str.begin(),
                                        pIter->m_edge[i].m_str.end(),
                                        queryS.begin() + symbolsMatched,
                                        queryS.end());

      if ((count > 0) && (count == szEdge || szQuery == count + symbolsMatched))
      {
        pIter.reset(pIter->GoToEdge(i));

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
  unique_ptr<search::TrieIterator> const pIter(
        MoveTrieIteratorToString(trieRoot, s, symbolsMatched, bFullEdgeMatched));

  if (!pIter || !bFullEdgeMatched || symbolsMatched != s.size())
    return;

#if defined(OMIM_OS_IPHONE) && !defined(__clang__)
  // Here is the dummy mutex to avoid mysterious iOS GCC-LLVM bug here.
  static threads::Mutex dummyM;
  threads::MutexGuard dummyG(dummyM);
#endif

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

  typedef vector<search::TrieIterator *> QueueT;
  QueueT trieQueue;
  {
    size_t symbolsMatched = 0;
    bool bFullEdgeMatched;
    search::TrieIterator * pRootIter =
        MoveTrieIteratorToString(trieRoot, s, symbolsMatched, bFullEdgeMatched);

    UNUSED_VALUE(symbolsMatched);
    UNUSED_VALUE(bFullEdgeMatched);

    if (!pRootIter)
      return;

    trieQueue.push_back(pRootIter);
  }

  // 'f' can throw an exception. So be prepared to delete unprocessed elements.
  MY_SCOPE_GUARD(doDelete, GetRangeDeletor(trieQueue, DeleteFunctor()));

  while (!trieQueue.empty())
  {
    // Next 2 lines don't throw any exceptions while moving
    // ownership from container to smart pointer.
    unique_ptr<search::TrieIterator> const pIter(trieQueue.back());
    trieQueue.pop_back();

    for (size_t i = 0; i < pIter->m_value.size(); ++i)
      f(pIter->m_value[i]);

    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
      trieQueue.push_back(pIter->GoToEdge(i));
  }
}

template <class FilterT> class OffsetIntersecter
{
  typedef trie::ValueReader::ValueType ValueT;

  struct HashFn
  {
    size_t operator() (ValueT const & v) const
    {
      return static_cast<size_t>(v.m_featureId);
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
  unique_ptr<SetType> m_prevSet;
  unique_ptr<SetType> m_set;

public:
  explicit OffsetIntersecter(FilterT const & filter)
    : m_filter(filter), m_set(new SetType)
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
      m_prevSet.reset(new SetType);

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

/// Return features set for each token.
template <class HolderT>
void GetFeaturesInTrie(vector<vector<strings::UniString> > const & tokens,
                       vector<strings::UniString> const & prefixTokens,
                       TrieRootPrefix const & trieRoot,
                       HolderT & holder)
{
  // Match tokens.
  size_t const count = tokens.size();
  holder.Resize(count + 1);

  for (size_t i = 0; i < count; ++i)
  {
    holder.StartNew(i);

    for (size_t j = 0; j < tokens[i].size(); ++j)
    {
      ASSERT ( !tokens[i][j].empty(), () );

      impl::FullMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize,
                            tokens[i][j], holder);
    }
  }

  // Match prefix.
  holder.StartNew(count);
  for (size_t i = 0; i < prefixTokens.size(); ++i)
  {
    ASSERT ( !prefixTokens[i].empty(), () );

    impl::FullMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize,
                          prefixTokens[i], holder);
  }
}

/// Do set intersection of features for each token.
template <class ToDo, class FilterT, class HolderT>
void MatchFeaturesInTrie(vector<vector<strings::UniString> > const & tokens,
                         vector<strings::UniString> const & prefixTokens,
                         TrieRootPrefix const & trieRoot,
                         FilterT const & filter,
                         HolderT const & addHolder,
                         ToDo & toDo)
{
  impl::OffsetIntersecter<FilterT> intersecter(filter);

  // Match tokens.
  size_t const count = tokens.size();
  for (size_t i = 0; i < count; ++i)
  {
    for (size_t j = 0; j < tokens[i].size(); ++j)
    {
      ASSERT ( !tokens[i][j].empty(), () );

      // match in trie
      impl::FullMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize,
                            tokens[i][j], intersecter);
    }

    // get additional features for 'i' token
    addHolder.GetValues(i, intersecter);

    intersecter.NextStep();
  }

  // Match prefix.
  size_t const prefixCount = prefixTokens.size();
  for (size_t i = 0; i < prefixCount; ++i)
  {
    ASSERT ( !prefixTokens[i].empty(), () );

    // match in trie
    impl::PrefixMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize,
                            prefixTokens[i], intersecter);
  }

  if (prefixCount > 0)
  {
    // get additional features for prefix token
    addHolder.GetValues(count, intersecter);

    intersecter.NextStep();
  }

  intersecter.ForEachResult(toDo);
}

}  // namespace search
