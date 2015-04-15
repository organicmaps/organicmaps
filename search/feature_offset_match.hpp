#pragma once
#include "search/search_common.hpp"
#include "search/search_query.hpp"
#include "search/search_query_params.hpp"

#include "indexer/search_trie.hpp"

#include "base/mutex.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/target_os.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_set.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace search
{
namespace impl
{
template <class TSrcIter, class TCompIter>
size_t CalcEqualLength(TSrcIter b, TSrcIter e, TCompIter bC, TCompIter eC)
{
  size_t count = 0;
  while ((b != e) && (bC != eC) && (*b++ == *bC++))
    ++count;
  return count;
}

inline trie::DefaultIterator * MoveTrieIteratorToString(trie::DefaultIterator const & trieRoot,
                                                        strings::UniString const & queryS,
                                                        size_t & symbolsMatched,
                                                        bool & bFullEdgeMatched)
{
  symbolsMatched = 0;
  bFullEdgeMatched = false;

  unique_ptr<trie::DefaultIterator> pIter(trieRoot.Clone());

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
void FullMatchInTrie(trie::DefaultIterator const & trieRoot, strings::UniChar const * rootPrefix,
                     size_t rootPrefixSize, strings::UniString s, F & f)
{
  if (!CheckMatchString(rootPrefix, rootPrefixSize, s))
      return;

  size_t symbolsMatched = 0;
  bool bFullEdgeMatched;
  unique_ptr<trie::DefaultIterator> const pIter(
      MoveTrieIteratorToString(trieRoot, s, symbolsMatched, bFullEdgeMatched));

  if (!pIter || (!s.empty() && !bFullEdgeMatched) || symbolsMatched != s.size())
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
void PrefixMatchInTrie(trie::DefaultIterator const & trieRoot, strings::UniChar const * rootPrefix,
                       size_t rootPrefixSize, strings::UniString s, F & f)
{
  if (!CheckMatchString(rootPrefix, rootPrefixSize, s))
      return;

  using TQueue = vector<trie::DefaultIterator *>;
  TQueue trieQueue;
  {
    size_t symbolsMatched = 0;
    bool bFullEdgeMatched;
    trie::DefaultIterator * pRootIter =
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
    unique_ptr<trie::DefaultIterator> const pIter(trieQueue.back());
    trieQueue.pop_back();

    for (size_t i = 0; i < pIter->m_value.size(); ++i)
      f(pIter->m_value[i]);

    for (size_t i = 0; i < pIter->m_edge.size(); ++i)
      trieQueue.push_back(pIter->GoToEdge(i));
  }
}

template <class TFilter>
class OffsetIntersecter
{
  using ValueT = trie::ValueReader::ValueType;

  struct HashFn
  {
    size_t operator() (ValueT const & v) const
    {
      return v.m_featureId;
    }
  };
  struct EqualFn
  {
    bool operator() (ValueT const & v1, ValueT const & v2) const
    {
      return (v1.m_featureId == v2.m_featureId);
    }
  };

  using TSet = unordered_set<ValueT, HashFn, EqualFn>;

  TFilter const & m_filter;
  unique_ptr<TSet> m_prevSet;
  unique_ptr<TSet> m_set;

public:
  explicit OffsetIntersecter(TFilter const & filter) : m_filter(filter), m_set(new TSet) {}

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
      m_prevSet.reset(new TSet);

    m_prevSet.swap(m_set);
    m_set->clear();
  }

  template <class ToDo>
  void ForEachResult(ToDo && toDo) const
  {
    if (!m_prevSet)
      return;
    for (auto const & value : *m_prevSet)
      toDo(value);
  }
};
}  // namespace search::impl

struct TrieRootPrefix
{
  trie::DefaultIterator const & m_root;
  strings::UniChar const * m_prefix;
  size_t m_prefixSize;

  TrieRootPrefix(trie::DefaultIterator const & root,
                 trie::DefaultIterator::Edge::EdgeStrT const & edge)
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

template <class TFilter>
class TrieValuesHolder
{
public:
  TrieValuesHolder(TFilter const & filter) : m_filter(filter) {}

  void Resize(size_t count) { m_holder.resize(count); }

  void SwitchTo(size_t index)
  {
    ASSERT_LESS(index, m_holder.size(), ());
    m_index = index;
  }

  void operator()(Query::TTrieValue const & v)
  {
    if (m_filter(v.m_featureId))
      m_holder[m_index].push_back(v);
  }

  template <class ToDo>
  void ForEachValue(size_t index, ToDo && toDo) const
  {
    for (auto const & value : m_holder[index])
      toDo(value);
  }

private:
  vector<vector<Query::TTrieValue>> m_holder;
  size_t m_index;
  TFilter const & m_filter;
};

// Calls toDo for each feature corresponding to at least one synonym.
// *NOTE* toDo may be called several times for the same feature.
template <typename ToDo>
void MatchTokenInTrie(SearchQueryParams::TSynonymsVector const & syns,
                      TrieRootPrefix const & trieRoot, ToDo && toDo)
{
  for (auto const & syn : syns)
  {
    ASSERT(!syn.empty(), ());
    impl::FullMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize, syn, toDo);
  }
}

// Calls toDo for each feature whose tokens contains at least one
// synonym as a prefix.
// *NOTE* toDo may be called serveral times for the same feature.
template <typename ToDo>
void MatchTokenPrefixInTrie(SearchQueryParams::TSynonymsVector const & syns,
                            TrieRootPrefix const & trieRoot, ToDo && toDo)
{
  for (auto const & syn : syns)
  {
    ASSERT(!syn.empty(), ());
    impl::PrefixMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize, syn, toDo);
  }
}

// Fills holder with features whose names correspond to tokens list up to synonyms.
// *NOTE* the same feature may be put in the same holder's slot several times.
template <typename THolder>
void MatchTokensInTrie(vector<SearchQueryParams::TSynonymsVector> const & tokens,
                       TrieRootPrefix const & trieRoot, THolder && holder)
{
  holder.Resize(tokens.size());
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    holder.SwitchTo(i);
    MatchTokenInTrie(tokens[i], trieRoot, holder);
  }
}

// Fills holder with features whose names correspond to tokens list up to synonyms,
// also, last holder's slot will be filled with features corresponding to prefixTokens.
// *NOTE* the same feature may be put in the same holder's slot several times.
template <typename THolder>
void MatchTokensAndPrefixInTrie(vector<SearchQueryParams::TSynonymsVector> const & tokens,
                                SearchQueryParams::TSynonymsVector const & prefixTokens,
                                TrieRootPrefix const & trieRoot, THolder && holder)
{
  MatchTokensInTrie(tokens, trieRoot, holder);

  holder.Resize(tokens.size() + 1);
  holder.SwitchTo(tokens.size());
  MatchTokenPrefixInTrie(prefixTokens, trieRoot, holder);
}

// Fills holder with categories whose description matches to at least one
// token from a search query.
// *NOTE* query prefix will be treated as a complete token in the function.
template <typename THolder>
bool MatchCategoriesInTrie(SearchQueryParams const & params, trie::DefaultIterator const & trieRoot,
                           THolder && holder)
{
  ASSERT_LESS(trieRoot.m_edge.size(), numeric_limits<uint32_t>::max(), ());
  uint32_t const numLangs = static_cast<uint32_t>(trieRoot.m_edge.size());
  for (uint32_t langIx = 0; langIx < numLangs; ++langIx)
  {
    auto const & edge = trieRoot.m_edge[langIx].m_str;
    ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());
    if (edge[0] == search::kCategoriesLang)
    {
      unique_ptr<trie::DefaultIterator> const catRoot(trieRoot.GoToEdge(langIx));
      MatchTokensInTrie(params.m_tokens, TrieRootPrefix(*catRoot, edge), holder);

      // Last token's prefix is used as a complete token here, to
      // limit the number of features in the last bucket of a
      // holder. Probably, this is a false optimization.
      holder.Resize(params.m_tokens.size() + 1);
      holder.SwitchTo(params.m_tokens.size());
      MatchTokenInTrie(params.m_prefixTokens, TrieRootPrefix(*catRoot, edge), holder);
      return true;
    }
  }
  return false;
}

// Calls toDo with trie root prefix and language code on each language
// allowed by params.
template <typename ToDo>
void ForEachLangPrefix(SearchQueryParams const & params, trie::DefaultIterator const & trieRoot,
                       ToDo && toDo)
{
  ASSERT_LESS(trieRoot.m_edge.size(), numeric_limits<uint32_t>::max(), ());
  uint32_t const numLangs = static_cast<uint32_t>(trieRoot.m_edge.size());
  for (uint32_t langIx = 0; langIx < numLangs; ++langIx)
  {
    auto const & edge = trieRoot.m_edge[langIx].m_str;
    ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());
    int8_t const lang = static_cast<int8_t>(edge[0]);
    if (edge[0] < search::kCategoriesLang && params.IsLangExist(lang))
    {
      unique_ptr<trie::DefaultIterator> const langRoot(trieRoot.GoToEdge(langIx));
      TrieRootPrefix langPrefix(*langRoot, edge);
      toDo(langPrefix, lang);
    }
  }
}

// Calls toDo for each feature whose description contains *ALL* tokens from a search query.
// Each feature will be passed to toDo only once.
template <typename TFilter, typename ToDo>
void MatchFeaturesInTrie(SearchQueryParams const & params, trie::DefaultIterator const & trieRoot,
                         TFilter const & filter, ToDo && toDo)
{
  TrieValuesHolder<TFilter> categoriesHolder(filter);
  CHECK(MatchCategoriesInTrie(params, trieRoot, categoriesHolder), ("Can't find categories."));

  impl::OffsetIntersecter<TFilter> intersecter(filter);
  for (size_t i = 0; i < params.m_tokens.size(); ++i)
  {
    ForEachLangPrefix(params, trieRoot, [&](TrieRootPrefix & langRoot, int8_t lang)
    {
      MatchTokenInTrie(params.m_tokens[i], langRoot, intersecter);
    });
    categoriesHolder.ForEachValue(i, intersecter);
    intersecter.NextStep();
  }

  if (!params.m_prefixTokens.empty())
  {
    ForEachLangPrefix(params, trieRoot, [&](TrieRootPrefix & langRoot, int8_t /* lang */)
    {
      MatchTokenPrefixInTrie(params.m_prefixTokens, langRoot, intersecter);
    });
    categoriesHolder.ForEachValue(params.m_tokens.size(), intersecter);
    intersecter.NextStep();
  }

  intersecter.ForEachResult(forward<ToDo>(toDo));
}
}  // namespace search
