#pragma once

#include "search/base/inverted_list.hpp"

#include "indexer/trie.hpp"

#include "base/assert.hpp"
#include "base/mem_trie.hpp"
#include "base/stl_add.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace search
{
namespace base
{
template <typename Id, typename Doc>
class MemSearchIndex
{
public:
  using Token = strings::UniString;
  using Char = Token::value_type;
  using List = InvertedList<Id>;
  using Trie = ::base::MemTrie<Token, List>;
  using Iterator = trie::MemTrieIterator<Token, List>;

  void Add(Id const & id, Doc const & doc)
  {
    ForEachToken(id, doc, [&](Token const & token) { m_trie.Add(token, id); });
  }

  void Erase(Id const & id, Doc const & doc)
  {
    ForEachToken(id, doc, [&](Token const & token) { m_trie.Erase(token, id); });
  }

  Iterator GetRootIterator() const { return Iterator(m_trie.GetRootIterator()); }

  std::vector<Id> GetAllIds() const
  {
    std::vector<Id> ids;
    m_trie.ForEachInTrie([&](Token const & /* token */, Id const & id) { ids.push_back(id); });
    my::SortUnique(ids);
    return ids;
  }

private:
  static Token AddLang(int8_t lang, Token const & token)
  {
    Token r(1 + token.size());
    r[0] = static_cast<Char>(lang);
    std::copy(token.begin(), token.end(), r.begin() + 1);
    return r;
  }

  template <typename Fn>
  void ForEachToken(Id const & id, Doc const & doc, Fn && fn)
  {
    doc.ForEachToken([&](int8_t lang, Token const & token) {
      if (lang >= 0)
        fn(AddLang(lang, token));
    });
  }

  Trie m_trie;
};
}  // namespace base
}  // namespace search
