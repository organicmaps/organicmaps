#pragma once

#include "search/query_params.hpp"
#include "search/search_index_values.hpp"
#include "search/search_trie.hpp"
#include "search/token_slice.hpp"

#include "indexer/trie.hpp"

#include "base/assert.hpp"
#include "base/dfa_helpers.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/uni_string_dfa.hpp"

#include <limits>
#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

namespace search
{
namespace impl
{
template <typename ValueList>
bool FindLangIndex(trie::Iterator<ValueList> const & trieRoot, uint8_t lang, uint32_t & langIx)
{
  ASSERT_LESS(trieRoot.m_edges.size(), std::numeric_limits<uint32_t>::max(), ());

  uint32_t const numLangs = static_cast<uint32_t>(trieRoot.m_edges.size());
  for (uint32_t i = 0; i < numLangs; ++i)
  {
    auto const & edge = trieRoot.m_edges[i].m_label;
    ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());
    if (edge[0] == lang)
    {
      langIx = i;
      return true;
    }
  }
  return false;
}

template <typename ValueList, typename DFA, typename ToDo>
bool MatchInTrie(trie::Iterator<ValueList> const & trieRoot, strings::UniChar const * rootPrefix, size_t rootPrefixSize,
                 DFA const & dfa, ToDo && toDo)
{
  using TrieDFAIt = std::shared_ptr<trie::Iterator<ValueList>>;
  using DFAIt = typename DFA::Iterator;
  using State = std::pair<TrieDFAIt, DFAIt>;

  std::queue<State> q;

  {
    auto it = dfa.Begin();
    DFAMove(it, rootPrefix, rootPrefix + rootPrefixSize);
    if (it.Rejects())
      return false;
    q.emplace(trieRoot.Clone(), it);
  }

  bool found = false;

  while (!q.empty())
  {
    auto const p = q.front();
    q.pop();

    auto const & trieIt = p.first;
    auto const & dfaIt = p.second;

    if (dfaIt.Accepts())
    {
      trieIt->m_values.ForEach([&dfaIt, &toDo](auto const & v) { toDo(v, dfaIt.ErrorsMade() == 0); });
      found = true;
    }

    size_t const numEdges = trieIt->m_edges.size();
    for (size_t i = 0; i < numEdges; ++i)
    {
      auto const & edge = trieIt->m_edges[i];

      auto curIt = dfaIt;
      strings::DFAMove(curIt, edge.m_label.begin(), edge.m_label.end());
      if (!curIt.Rejects())
        q.emplace(trieIt->GoToEdge(i), curIt);
    }
  }

  return found;
}

template <typename Filter, typename Value>
class OffsetIntersector
{
  using Values = std::unordered_map<Value, bool>;

  Filter const & m_filter;
  std::unique_ptr<Values> m_prevValues;
  std::unique_ptr<Values> m_values;

public:
  explicit OffsetIntersector(Filter const & filter) : m_filter(filter), m_values(std::make_unique<Values>()) {}

  void operator()(Value const & v, bool exactMatch)
  {
    if (m_prevValues && !m_prevValues->count(v))
      return;

    if (m_filter(v))
    {
      auto res = m_values->emplace(v, exactMatch);
      if (!res.second)
        res.first->second = res.first->second || exactMatch;
    }
  }

  void NextStep()
  {
    if (!m_prevValues)
      m_prevValues = std::make_unique<Values>();

    m_prevValues.swap(m_values);
    m_values->clear();
  }

  template <class ToDo>
  void ForEachResult(ToDo && toDo) const
  {
    if (!m_prevValues)
      return;
    for (auto const & value : *m_prevValues)
      toDo(value.first, value.second);
  }
};
}  // namespace impl

template <typename ValueList>
struct TrieRootPrefix
{
  using Value = typename ValueList::Value;
  using Iterator = trie::Iterator<ValueList>;

  Iterator const & m_root;
  strings::UniChar const * m_prefix;
  size_t m_prefixSize;

  TrieRootPrefix(Iterator const & root, typename Iterator::Edge::EdgeLabel const & edge) : m_root(root)
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

template <typename Filter, typename Value>
class TrieValuesHolder
{
public:
  TrieValuesHolder(Filter const & filter) : m_filter(filter) {}

  void operator()(Value const & v, bool exactMatch)
  {
    if (m_filter(v))
      m_values.emplace_back(v, exactMatch);
  }

  template <class ToDo>
  void ForEachValue(ToDo && toDo) const
  {
    for (auto const & value : m_values)
      toDo(value.first, value.second);
  }

private:
  std::vector<std::pair<Value, bool>> m_values;
  Filter const & m_filter;
};

template <typename DFA>
struct SearchTrieRequest
{
  SearchTrieRequest() = default;

  SearchTrieRequest(SearchTrieRequest &&) = default;
  SearchTrieRequest & operator=(SearchTrieRequest &&) = default;

  template <typename Langs>
  void SetLangs(Langs const & langs)
  {
    m_langs.clear();
    for (auto const lang : langs)
      if (lang >= 0 && lang <= std::numeric_limits<int8_t>::max())
        m_langs.insert(static_cast<int8_t>(lang));
  }

  bool HasLang(int8_t lang) const { return m_langs.find(lang) != m_langs.cend(); }

  void Clear()
  {
    m_names.clear();
    m_categories.clear();
    m_langs.clear();
  }

  std::vector<DFA> m_names;
  std::vector<strings::UniStringDFA> m_categories;

  // Set of languages, will be prepended to all DFAs in |m_names|
  // during retrieval from a search index.  Semantics of this field
  // depends on the search index, for example this can be a set of
  // langs from StringUtf8Multilang, or a set of locale indices.
  std::unordered_set<int8_t> m_langs;
};

// Calls |toDo| for each feature accepted by at least one DFA.
//
// *NOTE* |toDo| may be called several times for the same feature.
template <typename DFA, typename ValueList, typename ToDo>
void MatchInTrie(std::vector<DFA> const & dfas, TrieRootPrefix<ValueList> const & trieRoot, ToDo && toDo)
{
  for (auto const & dfa : dfas)
    impl::MatchInTrie(trieRoot.m_root, trieRoot.m_prefix, trieRoot.m_prefixSize, dfa, toDo);
}

// Calls |toDo| for each feature in categories branch matching to |request|.
//
// *NOTE* |toDo| may be called several times for the same feature.
template <typename DFA, typename ValueList, typename ToDo>
bool MatchCategoriesInTrie(SearchTrieRequest<DFA> const & request, trie::Iterator<ValueList> const & trieRoot,
                           ToDo && toDo)
{
  uint32_t langIx = 0;
  if (!impl::FindLangIndex(trieRoot, search::kCategoriesLang, langIx))
    return false;

  auto const & edge = trieRoot.m_edges[langIx].m_label;
  ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());

  auto const catRoot = trieRoot.GoToEdge(langIx);
  MatchInTrie(request.m_categories, TrieRootPrefix<ValueList>(*catRoot, edge), toDo);

  return true;
}

// Calls |toDo| with trie root prefix and language code on each
// language allowed by |request|.
template <typename DFA, typename ValueList, typename ToDo>
void ForEachLangPrefix(SearchTrieRequest<DFA> const & request, trie::Iterator<ValueList> const & trieRoot, ToDo && toDo)
{
  ASSERT_LESS(trieRoot.m_edges.size(), std::numeric_limits<uint32_t>::max(), ());

  uint32_t const numLangs = static_cast<uint32_t>(trieRoot.m_edges.size());
  for (uint32_t langIx = 0; langIx < numLangs; ++langIx)
  {
    auto const & edge = trieRoot.m_edges[langIx].m_label;
    ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());
    int8_t const lang = static_cast<int8_t>(edge[0]);
    if (edge[0] < search::kCategoriesLang && request.HasLang(lang))
    {
      auto const langRoot = trieRoot.GoToEdge(langIx);
      TrieRootPrefix<ValueList> langPrefix(*langRoot, edge);
      toDo(langPrefix, lang);
    }
  }
}

// Calls |toDo| for each feature whose description matches to
// |request|.  Each feature will be passed to |toDo| only once.
template <typename DFA, typename ValueList, typename Filter, typename ToDo>
void MatchFeaturesInTrie(SearchTrieRequest<DFA> const & request, trie::Iterator<ValueList> const & trieRoot,
                         Filter const & filter, ToDo && toDo)
{
  using Value = typename ValueList::Value;

  TrieValuesHolder<Filter, Value> categoriesHolder(filter);
  bool const categoriesExist = MatchCategoriesInTrie(request, trieRoot, categoriesHolder);

  /// @todo Not sure why do we have OffsetIntersector here? We are doing aggregation only.
  impl::OffsetIntersector<Filter, Value> intersector(filter);

  ForEachLangPrefix(request, trieRoot, [&request, &intersector](TrieRootPrefix<ValueList> & langRoot, int8_t /* lang */)
  {
    // Aggregate for all languages.
    MatchInTrie(request.m_names, langRoot, intersector);
  });

  if (categoriesExist)
  {
    // Aggregate categories.
    categoriesHolder.ForEachValue(intersector);
  }

  intersector.NextStep();
  intersector.ForEachResult(toDo);
}

template <typename ValueList, typename Filter, typename ToDo>
void MatchPostcodesInTrie(TokenSlice const & slice, trie::Iterator<ValueList> const & trieRoot, Filter const & filter,
                          ToDo && toDo)
{
  using namespace strings;
  using Value = typename ValueList::Value;

  uint32_t langIx = 0;
  if (!impl::FindLangIndex(trieRoot, search::kPostcodesLang, langIx))
    return;

  auto const & edge = trieRoot.m_edges[langIx].m_label;
  auto const postcodesRoot = trieRoot.GoToEdge(langIx);

  impl::OffsetIntersector<Filter, Value> intersector(filter);
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    // Full match required even for prefix token. Reasons:
    // 1. For postcode every symbol is important, partial matching can lead to wrong results.
    // 2. For prefix match query like "streetname 40" where |streetname| is located in 40xxx
    // postcode zone will give all street vicinity as the result which is wrong.
    std::vector<UniStringDFA> dfas;
    slice.Get(i).ForOriginalAndSynonyms([&dfas](UniString const & s) { dfas.emplace_back(s); });
    MatchInTrie(dfas, TrieRootPrefix<ValueList>(*postcodesRoot, edge), intersector);

    intersector.NextStep();
  }

  intersector.ForEachResult(toDo);
}
}  // namespace search
