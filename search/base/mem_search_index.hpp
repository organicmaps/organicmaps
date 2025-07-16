#pragma once

#include "search/base/inverted_list.hpp"

#include "indexer/trie.hpp"

#include "base/assert.hpp"
#include "base/mem_trie.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace search_base
{
template <typename Id>
class MemSearchIndex
{
public:
  using Token = strings::UniString;
  using Char = Token::value_type;
  using List = InvertedList<Id>;
  using Trie = base::MemTrie<Token, List>;
  using Iterator = trie::MemTrieIterator<Token, List>;

  template <typename Doc>
  void Add(Id const & id, Doc const & doc)
  {
    ForEachToken(id, doc, [&](Token const & token) { m_trie.Add(token, id); });
  }

  template <typename Doc>
  void Erase(Id const & id, Doc const & doc)
  {
    ForEachToken(id, doc, [&](Token const & token) { m_trie.Erase(token, id); });
  }

  Iterator GetRootIterator() const { return Iterator(m_trie.GetRootIterator()); }

  std::vector<Id> GetAllIds() const
  {
    return WithIds([&](std::vector<Id> & ids)
    { m_trie.ForEachInTrie([&](Token const & /* token */, Id const & id) { ids.push_back(id); }); });
  }

  size_t GetNumDocs(int8_t lang, strings::UniString const & token, bool prefix) const
  {
    auto const key = AddLang(lang, token);

    if (!prefix)
    {
      size_t numDocs = 0;
      m_trie.WithValuesHolder(key, [&](List const & list) { numDocs = list.Size(); });
      return numDocs;
    }

    return WithIds([&](std::vector<Id> & ids)
    {
      m_trie.ForEachInSubtree(key, [&](Token const & /* token */, Id const & id) { ids.push_back(id); });
    }).size();
  }

private:
  static Token AddLang(int8_t lang, Token const & token)
  {
    Token r(1 + token.size());
    r[0] = static_cast<Char>(lang);
    std::copy(token.begin(), token.end(), r.begin() + 1);
    return r;
  }

  template <typename Doc, typename Fn>
  void ForEachToken(Id const & /*id*/, Doc const & doc, Fn && fn)
  {
    doc.ForEachToken([&](int8_t lang, Token const & token)
    {
      if (lang >= 0)
        fn(AddLang(lang, token));
    });
  }

  template <typename Fn>
  static std::vector<Id> WithIds(Fn && fn)
  {
    std::vector<Id> ids;
    fn(ids);
    base::SortUnique(ids);
    return ids;
  }

  Trie m_trie;
};
}  // namespace search_base
