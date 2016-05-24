#pragma once
#include "search/processor.hpp"
#include "search/query_params.hpp"
#include "search/search_common.hpp"
#include "search/search_index_values.hpp"
#include "search/v2/token_slice.hpp"

#include "indexer/trie.hpp"

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

template <typename TValue>
inline shared_ptr<trie::Iterator<ValueList<TValue>>> MoveTrieIteratorToString(
    trie::Iterator<ValueList<TValue>> const & trieRoot, strings::UniString const & queryS,
    size_t & symbolsMatched, bool & bFullEdgeMatched)
{
  symbolsMatched = 0;
  bFullEdgeMatched = false;

  auto it = trieRoot.Clone();

  size_t const szQuery = queryS.size();

  while (symbolsMatched < szQuery)
  {
    bool bMatched = false;

    ASSERT_LESS(it->m_edge.size(), std::numeric_limits<uint32_t>::max(), ());
    uint32_t const edgeCount = static_cast<uint32_t>(it->m_edge.size());

    for (uint32_t i = 0; i < edgeCount; ++i)
    {
      size_t const szEdge = it->m_edge[i].m_label.size();

      size_t const count =
          CalcEqualLength(it->m_edge[i].m_label.begin(), it->m_edge[i].m_label.end(),
                          queryS.begin() + symbolsMatched, queryS.end());

      if ((count > 0) && (count == szEdge || szQuery == count + symbolsMatched))
      {
        it = it->GoToEdge(i);

        bFullEdgeMatched = (count == szEdge);
        symbolsMatched += count;
        bMatched = true;
        break;
      }
    }

    if (!bMatched)
      return NULL;
  }
  return it->Clone();
}

namespace
{
bool CheckMatchString(strings::UniChar const * rootPrefix, size_t rootPrefixSize,
                      strings::UniString & s, bool prefix)
{
  if (rootPrefixSize == 0)
    return true;

  if (prefix && s.size() < rootPrefixSize &&
      StartsWith(rootPrefix, rootPrefix + rootPrefixSize, s.begin(), s.end()))
  {
    // In the case of prefix match query may be a prefix of the root
    // label string.  In this case we continue processing as if the
    // string is equal to root label.
    s.clear();
    return true;
  }
  if (s.size() >= rootPrefixSize &&
      StartsWith(s.begin(), s.end(), rootPrefix, rootPrefix + rootPrefixSize))
  {
    // In both (prefix and not-prefix) cases when string has root label
    // as a prefix, we continue processing.
    s = strings::UniString(s.begin() + rootPrefixSize, s.end());
    return true;
  }

  return false;
}

template <typename TValue>
bool FindLangIndex(trie::Iterator<ValueList<TValue>> const & trieRoot, uint8_t lang, uint32_t & langIx)
{
  ASSERT_LESS(trieRoot.m_edge.size(), numeric_limits<uint32_t>::max(), ());

  uint32_t const numLangs = static_cast<uint32_t>(trieRoot.m_edge.size());
  for (uint32_t i = 0; i < numLangs; ++i)
  {
    auto const & edge = trieRoot.m_edge[i].m_label;
    ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());
    if (edge[0] == lang)
    {
      langIx = i;
      return true;
    }
  }
  return false;
}
}  // namespace

template <typename TValue, typename TF>
void FullMatchInTrie(trie::Iterator<ValueList<TValue>> const & trieRoot,
                     strings::UniChar const * rootPrefix, size_t rootPrefixSize,
                     strings::UniString s, TF & f)
{
  if (!CheckMatchString(rootPrefix, rootPrefixSize, s, false /* prefix */))
    return;

  size_t symbolsMatched = 0;
  bool bFullEdgeMatched;
  auto const it = MoveTrieIteratorToString(trieRoot, s, symbolsMatched, bFullEdgeMatched);

  if (!it || (!s.empty() && !bFullEdgeMatched) || symbolsMatched != s.size())
    return;

#if defined(OMIM_OS_IPHONE) && !defined(__clang__)
  // Here is the dummy mutex to avoid mysterious iOS GCC-LLVM bug here.
  static threads::Mutex dummyM;
  threads::MutexGuard dummyG(dummyM);
#endif

  ASSERT_EQUAL(symbolsMatched, s.size(), ());

  it->m_valueList.ForEach(f);
}

template <typename TValue, typename TF>
void PrefixMatchInTrie(trie::Iterator<ValueList<TValue>> const & trieRoot,
                       strings::UniChar const * rootPrefix, size_t rootPrefixSize,
                       strings::UniString s, TF & f)
{
  if (!CheckMatchString(rootPrefix, rootPrefixSize, s, true /* prefix */))
    return;

  using TIterator = trie::Iterator<ValueList<TValue>>;

  using TQueue = vector<shared_ptr<TIterator>>;
  TQueue trieQueue;
  {
    size_t symbolsMatched = 0;
    bool bFullEdgeMatched;
    auto const it = MoveTrieIteratorToString(trieRoot, s, symbolsMatched, bFullEdgeMatched);

    UNUSED_VALUE(symbolsMatched);
    UNUSED_VALUE(bFullEdgeMatched);

    if (!it)
      return;

    trieQueue.push_back(it);
  }

  while (!trieQueue.empty())
  {
    auto const it = trieQueue.back();
    trieQueue.pop_back();

    it->m_valueList.ForEach(f);

    for (size_t i = 0; i < it->m_edge.size(); ++i)
      trieQueue.push_back(it->GoToEdge(i));
  }
}

template <typename TFilter, typename TValue>
class OffsetIntersecter
{
  struct HashFn
  {
    size_t operator()(TValue const & v) const { return v.m_featureId; }
  };
  struct EqualFn
  {
    bool operator()(TValue const & v1, TValue const & v2) const
    {
      return (v1.m_featureId == v2.m_featureId);
    }
  };

  using TSet = unordered_set<TValue, HashFn, EqualFn>;

  TFilter const & m_filter;
  unique_ptr<TSet> m_prevSet;
  unique_ptr<TSet> m_set;

public:
  explicit OffsetIntersecter(TFilter const & filter) : m_filter(filter), m_set(new TSet) {}

  void operator()(TValue const & v)
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
}  // namespace impl

template <typename TValue>
struct TrieRootPrefix
{
  using TIterator = trie::Iterator<ValueList<TValue>>;
  TIterator const & m_root;
  strings::UniChar const * m_prefix;
  size_t m_prefixSize;

  TrieRootPrefix(TIterator const & root, typename TIterator::Edge::TEdgeLabel const & edge)
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

template <typename TFilter, typename TValue>
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

  void operator()(TValue const & v)
  {
    if (m_filter(v.m_featureId))
      m_holder[m_index].push_back(v);
  }

  template <class ToDo>
  void ForEachValue(size_t index, ToDo && toDo) const
  {
    ASSERT_LESS(index, m_holder.size(), ());
    for (auto const & value : m_holder[index])
      toDo(value);
  }

private:
  vector<vector<TValue>> m_holder;
  size_t m_index;
  TFilter const & m_filter;
};

// Calls toDo for each feature corresponding to at least one synonym.
// *NOTE* toDo may be called several times for the same feature.
template <typename TValue, typename ToDo>
void MatchTokenInTrie(QueryParams::TSynonymsVector const & syns,
                      TrieRootPrefix<TValue> const & trieRoot, ToDo && toDo)
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
template <typename TValue, typename ToDo>
void MatchTokenPrefixInTrie(QueryParams::TSynonymsVector const & syns,
                            TrieRootPrefix<TValue> const & trieRoot, ToDo && toDo)
{
  for (auto const & syn : syns)
  {
    ASSERT(!syn.empty(), ());
    impl::PrefixMatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize, syn, toDo);
  }
}

// Fills holder with features whose names correspond to tokens list up to synonyms.
// *NOTE* the same feature may be put in the same holder's slot several times.
template <typename TValue, typename THolder>
void MatchTokensInTrie(vector<QueryParams::TSynonymsVector> const & tokens,
                       TrieRootPrefix<TValue> const & trieRoot, THolder && holder)
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
template <typename TValue, typename THolder>
void MatchTokensAndPrefixInTrie(vector<QueryParams::TSynonymsVector> const & tokens,
                                QueryParams::TSynonymsVector const & prefixTokens,
                                TrieRootPrefix<TValue> const & trieRoot, THolder && holder)
{
  MatchTokensInTrie(tokens, trieRoot, holder);

  holder.Resize(tokens.size() + 1);
  holder.SwitchTo(tokens.size());
  MatchTokenPrefixInTrie(prefixTokens, trieRoot, holder);
}

// Fills holder with categories whose description matches to at least one
// token from a search query.
// *NOTE* query prefix will be treated as a complete token in the function.
template <typename TValue, typename THolder>
bool MatchCategoriesInTrie(QueryParams const & params,
                           trie::Iterator<ValueList<TValue>> const & trieRoot, THolder && holder)
{
  uint32_t langIx = 0;
  if (!impl::FindLangIndex(trieRoot, search::kCategoriesLang, langIx))
    return false;

  auto const & edge = trieRoot.m_edge[langIx].m_label;
  ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());

  auto const catRoot = trieRoot.GoToEdge(langIx);
  MatchTokensInTrie(params.m_tokens, TrieRootPrefix<TValue>(*catRoot, edge), holder);

  // Last token's prefix is used as a complete token here, to limit
  // the number of features in the last bucket of a holder. Probably,
  // this is a false optimization.
  holder.Resize(params.m_tokens.size() + 1);
  holder.SwitchTo(params.m_tokens.size());
  MatchTokenInTrie(params.m_prefixTokens, TrieRootPrefix<TValue>(*catRoot, edge), holder);
  return true;
}

// Calls toDo with trie root prefix and language code on each language
// allowed by params.
template <typename TValue, typename ToDo>
void ForEachLangPrefix(QueryParams const & params,
                       trie::Iterator<ValueList<TValue>> const & trieRoot, ToDo && toDo)
{
  ASSERT_LESS(trieRoot.m_edge.size(), numeric_limits<uint32_t>::max(), ());
  uint32_t const numLangs = static_cast<uint32_t>(trieRoot.m_edge.size());
  for (uint32_t langIx = 0; langIx < numLangs; ++langIx)
  {
    auto const & edge = trieRoot.m_edge[langIx].m_label;
    ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());
    int8_t const lang = static_cast<int8_t>(edge[0]);
    if (edge[0] < search::kCategoriesLang && params.IsLangExist(lang))
    {
      auto const langRoot = trieRoot.GoToEdge(langIx);
      TrieRootPrefix<TValue> langPrefix(*langRoot, edge);
      toDo(langPrefix, lang);
    }
  }
}

// Calls toDo for each feature whose description contains *ALL* tokens from a search query.
// Each feature will be passed to toDo only once.
template <typename TValue, typename TFilter, typename ToDo>
void MatchFeaturesInTrie(QueryParams const & params,
                         trie::Iterator<ValueList<TValue>> const & trieRoot, TFilter const & filter,
                         ToDo && toDo)
{
  using TIterator = trie::Iterator<ValueList<TValue>>;

  TrieValuesHolder<TFilter, TValue> categoriesHolder(filter);
  bool const categoriesMatched = MatchCategoriesInTrie(params, trieRoot, categoriesHolder);

  impl::OffsetIntersecter<TFilter, TValue> intersecter(filter);
  for (size_t i = 0; i < params.m_tokens.size(); ++i)
  {
    ForEachLangPrefix(params, trieRoot, [&](TrieRootPrefix<TValue> & langRoot, int8_t lang)
                      {
                        MatchTokenInTrie(params.m_tokens[i], langRoot, intersecter);
                      });
    if (categoriesMatched)
      categoriesHolder.ForEachValue(i, intersecter);
    intersecter.NextStep();
  }

  if (!params.m_prefixTokens.empty())
  {
    ForEachLangPrefix(params, trieRoot, [&](TrieRootPrefix<TValue> & langRoot, int8_t /* lang */)
                      {
                        MatchTokenPrefixInTrie(params.m_prefixTokens, langRoot, intersecter);
                      });
    if (categoriesMatched)
      categoriesHolder.ForEachValue(params.m_tokens.size(), intersecter);
    intersecter.NextStep();
  }

  intersecter.ForEachResult(forward<ToDo>(toDo));
}

template <typename TValue, typename TFilter, typename ToDo>
void MatchPostcodesInTrie(v2::TokenSlice const & slice,
                          trie::Iterator<ValueList<TValue>> const & trieRoot,
                          TFilter const & filter, ToDo && toDo)
{
  uint32_t langIx = 0;
  if (!impl::FindLangIndex(trieRoot, search::kPostcodesLang, langIx))
    return;

  auto const & edge = trieRoot.m_edge[langIx].m_label;
  auto const postcodesRoot = trieRoot.GoToEdge(langIx);

  impl::OffsetIntersecter<TFilter, TValue> intersecter(filter);
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    if (slice.IsPrefix(i))
      MatchTokenPrefixInTrie(slice.Get(i), TrieRootPrefix<TValue>(*postcodesRoot, edge), intersecter);
    else
      MatchTokenInTrie(slice.Get(i), TrieRootPrefix<TValue>(*postcodesRoot, edge), intersecter);
    intersecter.NextStep();
  }

  intersecter.ForEachResult(forward<ToDo>(toDo));
}
}  // namespace search
